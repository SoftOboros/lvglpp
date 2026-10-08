#!/usr/bin/env python3
"""Create a draft once; accept exact retries and never replace release assets."""
import json
import os
import re
import subprocess
import sys
from pathlib import Path

from source_release import digest, verify


def api(path, missing_ok=False):
    result = subprocess.run(['gh', 'api', path], capture_output=True, text=True)
    if result.returncode:
        if missing_ok and 'HTTP 404' in result.stderr:
            return None
        raise RuntimeError(result.stderr)
    return json.loads(result.stdout)


def main():
    tag, asset_dir = sys.argv[1:]
    if not re.fullmatch(r'v\d+\.\d+\.\d+', tag):
        raise ValueError('Invalid release tag')
    repository = os.environ['GITHUB_REPOSITORY']
    if repository != 'SoftOboros/lvglpp':
        raise ValueError('Draft publication is restricted to SoftOboros/lvglpp')
    assets = Path(asset_dir)
    manifest = verify(assets)
    if manifest['release_tag'] != tag:
        raise ValueError('Manifest release tag differs from publication tag')
    commit = manifest['lvglpp']['commit']
    ref = api('repos/' + repository + '/git/ref/tags/' + tag)['object']
    while ref['type'] == 'tag':
        ref = api('repos/' + repository + '/git/tags/' + ref['sha'])['object']
    if ref['type'] != 'commit' or ref['sha'] != commit:
        raise ValueError('Remote release tag moved since packaging')
    # Check current remote main too, without trusting a stale local tracking ref.
    comparison = api('repos/' + repository + '/compare/' + commit + '...main')
    if comparison['status'] not in ('ahead', 'identical'):
        raise ValueError('Packaged commit is no longer on main')
    existing = api('repos/' + repository + '/releases/tags/' + tag, missing_ok=True)
    paths = sorted(assets.iterdir())
    if existing:
        expected = {p.name: 'sha256:' + digest(p) for p in paths}
        actual = {p['name']: p.get('digest') for p in existing['assets']}
        if not existing['draft'] or expected != actual or existing['target_commitish'] != commit:
            raise ValueError('Existing release differs; refusing to overwrite or repair it automatically')
        print('Existing draft already contains exactly these assets: ' + existing['html_url'])
        return
    lvgl = manifest['lvgl']
    notes = assets.parent / 'release-notes.md'
    notes.write_text('Source release for CMake consumers. Requires CMake >= 3.20 and C++20.\n\n'
                     'LVGL source: `' + lvgl['version'] + ' @ ' + lvgl['commit'] + '`.\n\n'
                     'Use the uploaded source assets; GitHub automatic archives omit LVGL. '
                     'Verify SHA256SUMS, then extract LVGL into lvglpp/lvgl. '
                     'manifest.json records both exact source revisions and license hashes. '
                     'lvglpp.hash provides Buildroot source/license hashes.\n\n'
                     'Validated: archive-only host tests, Linux Wayland SHM/FBDEV/EVDEV compile '
                     'and link, Cortex-M7 library cross-build, and CMake 3.20.5. '
                     'Target hardware and the consumer Buildroot sysroot require downstream validation.\n\n'
                     'Maintainer: enable release immutability in repository settings before publishing '
                     'this draft. Preserve the fixed uploaded bytes when mirroring.\n')
    subprocess.run(['gh', 'release', 'create', tag, '--repo', repository,
                    '--draft', '--verify-tag', '--target', commit, '--title', tag,
                    '--notes-file', str(notes), *[str(p) for p in paths]], check=True)


if __name__ == '__main__':
    main()
