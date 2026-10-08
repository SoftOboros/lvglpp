#!/usr/bin/env python3
"""Build and check fixed source-release assets using only committed Git trees."""
import argparse
import gzip
import hashlib
import io
import json
import re
import subprocess
import tarfile
from pathlib import Path, PurePosixPath


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args])


def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as source:
        for block in iter(lambda: source.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def blob(repo, revision, path):
    return git(repo, 'show', revision + ':' + path)


def safe_name(name):
    p = PurePosixPath(name)
    if p.is_absolute() or '..' in p.parts or '\\' in name or not p.parts:
        raise ValueError('Unsafe archive path: ' + name)
    return p


def archive(repo, revision, root, destination):
    # Git archive omits submodule contents; LVGL is a separate, pinned asset.
    raw = git(repo, 'archive', '--format=tar', revision)
    with tarfile.open(fileobj=io.BytesIO(raw)) as source:
        with destination.open('wb') as output:
            with gzip.GzipFile(filename='', fileobj=output, mode='wb', mtime=0) as compressed:
                with tarfile.open(fileobj=compressed, mode='w', format=tarfile.PAX_FORMAT) as target:
                    for member in sorted(source.getmembers(), key=lambda item: item.name):
                        safe_name(member.name)
                        if not (member.isfile() or member.isdir()):
                            raise ValueError('Release archives require regular files/directories: ' + member.name)
                        member.name = root + '/' + member.name
                        member.uid = member.gid = member.mtime = 0
                        member.uname = member.gname = ''
                        member.pax_headers = {}
                        member.mode = 0o755 if member.isdir() or member.mode & 0o111 else 0o644
                        target.addfile(member, source.extractfile(member) if member.isfile() else None)


def package(repo, out, release_tag=''):
    revision = git(repo, 'rev-parse', 'HEAD').decode().strip()
    cmake = blob(repo, revision, 'CMakeLists.txt').decode()
    version = re.search(r'\bVERSION\s+(\d+\.\d+\.\d+)', cmake).group(1)
    policy = json.loads(blob(repo, revision, 'release/source.json'))
    lvgl_sha = git(repo, 'rev-parse', revision + ':lvgl').decode().strip()
    if lvgl_sha != policy['lvgl_commit']:
        raise ValueError('LVGL gitlink differs from the qualified release policy')
    lvgl = repo / 'lvgl'
    tag = policy['lvgl_tag']
    if tag is not None:
        if not re.fullmatch(r'v\d+\.\d+\.\d+', tag):
            raise ValueError('Invalid LVGL release tag')
        if git(lvgl, 'rev-parse', 'refs/tags/' + tag + '^{commit}').decode().strip() != lvgl_sha:
            raise ValueError('LVGL release tag does not resolve to its pinned commit')
    macros = blob(lvgl, lvgl_sha, 'lv_version.h').decode()
    numbers = [re.search(r'#define\s+LVGL_VERSION_' + part + r'\s+(\d+)', macros).group(1)
               for part in ('MAJOR', 'MINOR', 'PATCH')]
    info = re.search(r'#define\s+LVGL_VERSION_INFO\s+"([^"]*)"', macros).group(1)
    lvgl_version = '.'.join(numbers) + ('-' + info if info else '')
    if lvgl_version != policy['lvgl_version']:
        raise ValueError('LVGL version differs from the qualified release policy')
    if tag and ('v' + '.'.join(numbers) != tag or info):
        raise ValueError('LVGL version macros do not describe the declared release tag')
    if release_tag:
        if release_tag != 'v' + version:
            raise ValueError('Release tag must match the CMake project version')
        tagged = git(repo, 'rev-parse', 'refs/tags/' + release_tag + '^{commit}').decode().strip()
        if tagged != revision:
            raise ValueError('Release tag must already exist at the packaged commit')
        subprocess.run(['git', '-C', str(repo), 'merge-base', '--is-ancestor', revision,
                        'refs/remotes/origin/main'], check=True)
    if out.exists() and any(out.iterdir()):
        raise ValueError('Output directory must be empty; never replace existing assets')
    out.mkdir(parents=True, exist_ok=True)
    label = release_tag or ('candidate-' + revision[:12])
    parent_root = 'lvglpp-' + label
    lvgl_root = 'lvgl-' + (tag or lvgl_sha)
    parent_asset = parent_root + '.tar.gz'
    lvgl_asset = lvgl_root + '.tar.gz'
    licenses = {}
    for path, owner, commit, original in [('LICENSE', repo, revision, 'LICENSE'),
                                         ('lvgl/LICENCE.txt', lvgl, lvgl_sha, 'LICENCE.txt'),
                                         ('lvgl/COPYRIGHTS.md', lvgl, lvgl_sha, 'COPYRIGHTS.md')]:
        content = blob(owner, commit, original)
        if not content.strip():
            raise ValueError('Missing license notice: ' + path)
        licenses[path] = hashlib.sha256(content).hexdigest()
    archive(repo, revision, parent_root, out / parent_asset)
    archive(lvgl, lvgl_sha, lvgl_root, out / lvgl_asset)
    manifest = {
        'schema_version': 1,
        'release_tag': release_tag or None,
        'project_version': version,
        'lvglpp': {'repository': 'https://github.com/SoftOboros/lvglpp',
                   'commit': revision, 'archive': parent_asset, 'root': parent_root,
                   'sha256': digest(out / parent_asset)},
        'lvgl': {'repository': 'https://github.com/lvgl/lvgl', 'tag': tag,
                 'version': lvgl_version, 'commit': lvgl_sha, 'archive': lvgl_asset, 'root': lvgl_root,
                 'sha256': digest(out / lvgl_asset)},
        'layout': 'Extract lvglpp, then place the extracted LVGL tree at lvglpp/lvgl.',
        'rlvgl_included': False,
        'license_sha256': licenses,
        'requirements': {'cmake_minimum': '3.20', 'cxx_standard': 20,
                         'cross_build': 'LVGLPP_EMBEDDED_POSTURE=ON',
                         'configuration': 'LV_BUILD_CONF_PATH or LV_BUILD_CONF_DIR',
                         'dependencies': 'Supply enabled LVGL backend libraries from the target sysroot.'},
    }
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
    hash_lines = ['# Fixed release assets and extracted license files; SHA-256.']
    for name in (parent_asset, lvgl_asset, 'manifest.json'):
        hash_lines.append('sha256  ' + digest(out / name) + '  ' + name)
    for name, value in sorted(licenses.items()):
        hash_lines.append('sha256  ' + value + '  ' + name)
    (out / 'lvglpp.hash').write_text('\n'.join(hash_lines) + '\n')
    (out / 'SHA256SUMS').write_text(''.join(digest(path) + '  ' + path.name + '\n'
                                          for path in sorted(out.iterdir())))
    verify(out)
    print(json.dumps({'commit': revision, 'lvgl_commit': lvgl_sha, 'output': str(out)}))


def verify(out):
    listed = set()
    for line in (out / 'SHA256SUMS').read_text().splitlines():
        checksum, name = line.split('  ', 1)
        if not re.fullmatch(r'[a-f0-9]{64}', checksum) or Path(name).name != name:
            raise ValueError('Invalid checksum entry')
        if name in listed or name == 'SHA256SUMS':
            raise ValueError('Duplicate or circular checksum entry')
        listed.add(name)
        if digest(out / name) != checksum:
            raise ValueError('Checksum mismatch: ' + name)
    manifest = json.loads((out / 'manifest.json').read_text())
    if manifest['schema_version'] != 1:
        raise ValueError('Unsupported manifest schema')
    if set(manifest['license_sha256']) != {'LICENSE', 'lvgl/LICENCE.txt', 'lvgl/COPYRIGHTS.md'}:
        raise ValueError('Missing or unexpected license hashes')
    expected = {'manifest.json', 'lvglpp.hash', manifest['lvglpp']['archive'], manifest['lvgl']['archive']}
    if listed != expected or {p.name for p in out.iterdir()} != expected | {'SHA256SUMS'}:
        raise ValueError('Unexpected or missing release assets')
    for key in ('lvglpp', 'lvgl'):
        item = manifest[key]
        if digest(out / item['archive']) != item['sha256']:
            raise ValueError('Manifest hash mismatch: ' + key)
        safe_name(item['root'])
        with tarfile.open(out / item['archive']) as source:
            seen = set()
            for member in source.getmembers():
                p = safe_name(member.name)
                if p.parts[0] != item['root'] or not (member.isfile() or member.isdir()):
                    raise ValueError('Unexpected archive member: ' + member.name)
                if member.name in seen or '.git' in p.parts:
                    raise ValueError('Duplicate or Git metadata in archive')
                seen.add(member.name)
    expected_hashes = {name: digest(out / name) for name in expected if name != 'lvglpp.hash'}
    expected_hashes.update(manifest['license_sha256'])
    hash_rows = {}
    for line in (out / 'lvglpp.hash').read_text().splitlines():
        if line.startswith('#') or not line.strip():
            continue
        kind, checksum, name = line.split()
        if kind != 'sha256' or name in hash_rows:
            raise ValueError('Invalid Buildroot hash entry')
        hash_rows[name] = checksum
    if hash_rows != expected_hashes:
        raise ValueError('Buildroot hashes differ from the assets or license manifest')
    return manifest


def extract(out, destination):
    manifest = verify(out)
    if destination.exists():
        raise ValueError('Extraction destination must not already exist')
    destination.mkdir(parents=True)
    for key in ('lvglpp', 'lvgl'):
        with tarfile.open(out / manifest[key]['archive']) as source:
            # verify() rejected symlinks, special files, traversal and duplicates.
            source.extractall(destination, **({'filter': 'data'} if hasattr(tarfile, 'data_filter') else {}))
    parent = destination / manifest['lvglpp']['root']
    lvgl_dir = parent / 'lvgl'
    if lvgl_dir.exists():
        lvgl_dir.rmdir()  # Git's empty submodule placeholder only.
    (destination / manifest['lvgl']['root']).rename(lvgl_dir)
    for name, checksum in manifest['license_sha256'].items():
        if digest(parent / name) != checksum:
            raise ValueError('Extracted license hash mismatch: ' + name)
    if any((parent / 'rlvgl').glob('**/*')):
        raise ValueError('Rust reference unexpectedly included')
    print(parent)
    return parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    make = sub.add_parser('package')
    make.add_argument('--repo', type=Path, default=Path('.'))
    make.add_argument('--output', type=Path, required=True)
    make.add_argument('--release-tag', default='')
    check = sub.add_parser('verify')
    check.add_argument('assets', type=Path)
    unpack = sub.add_parser('extract')
    unpack.add_argument('assets', type=Path)
    unpack.add_argument('destination', type=Path)
    args = parser.parse_args()
    if args.command == 'package':
        package(args.repo.resolve(), args.output.resolve(), args.release_tag)
    elif args.command == 'verify':
        verify(args.assets)
    else:
        extract(args.assets, args.destination)


if __name__ == '__main__':
    main()
