"""Integration checks for release integrity, reproducibility and publication gates."""
import contextlib
import io
import json
import os
import sys
from unittest.mock import patch

import draft_release
import subprocess
import tarfile
import tempfile
import unittest
from pathlib import Path

from source_release import digest, extract, git, package, verify


class SourceReleaseTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.repo = self.root / 'repo'
        self.repo.mkdir()
        self.init(self.repo)
        lvgl = self.repo / 'lvgl'
        lvgl.mkdir()
        self.init(lvgl)
        for name in ('LICENCE.txt', 'COPYRIGHTS.md'):
            (lvgl / name).write_text('MIT notice\n')
        (lvgl / 'lv_version.h').write_text('''#define LVGL_VERSION_MAJOR 9
#define LVGL_VERSION_MINOR 6
#define LVGL_VERSION_PATCH 0
#define LVGL_VERSION_INFO "dev"
''')
        self.commit(lvgl)
        self.lvgl_sha = git(lvgl, 'rev-parse', 'HEAD').decode().strip()
        (self.repo / 'LICENSE').write_text('MIT project license\n')
        (self.repo / 'CMakeLists.txt').write_text('project(lvglpp VERSION 0.1.0)\n')
        (self.repo / 'release').mkdir()
        self.policy = {'lvgl_tag': None, 'lvgl_commit': self.lvgl_sha,
                       'lvgl_version': '9.6.0-dev'}
        (self.repo / 'release/source.json').write_text(json.dumps(self.policy))
        git(self.repo, 'add', 'LICENSE', 'CMakeLists.txt', 'release')
        git(self.repo, 'update-index', '--add', '--cacheinfo', '160000,' + self.lvgl_sha + ',lvgl')
        git(self.repo, 'commit', '-qm', 'fixture')
        git(self.repo, 'tag', '-a', 'v0.1.0', '-m', 'fixture release')
        git(self.repo, 'update-ref', 'refs/remotes/origin/main', 'HEAD')
        self.out = self.root / 'assets'

    def tearDown(self):
        self.temp.cleanup()

    def init(self, repo):
        git(repo, 'init', '-q')
        git(repo, 'config', 'user.name', 'Release test')
        git(repo, 'config', 'user.email', 'release-test@example.invalid')

    def commit(self, repo):
        git(repo, 'add', '.')
        git(repo, 'commit', '-qm', 'fixture')

    def make(self, out=None, tag=''):
        with contextlib.redirect_stdout(io.StringIO()):
            package(self.repo, out or self.out, tag)

    def update_policy(self):
        (self.repo / 'release/source.json').write_text(json.dumps(self.policy))
        git(self.repo, 'add', 'release/source.json')
        git(self.repo, 'commit', '-qm', 'policy')

    def refresh_sums(self):
        (self.out / 'SHA256SUMS').write_text(''.join(digest(p) + '  ' + p.name + '\n'
            for p in sorted(self.out.iterdir()) if p.name != 'SHA256SUMS'))

    def test_reproducible_committed_trees_and_no_git_or_rust(self):
        self.make()
        # Untracked and modified files are never silently included in a release.
        (self.repo / 'private.txt').write_text('untracked')
        (self.repo / 'LICENSE').write_text('working copy')
        other = self.root / 'second'
        self.make(other)
        self.assertEqual((self.out / 'SHA256SUMS').read_bytes(), (other / 'SHA256SUMS').read_bytes())
        with contextlib.redirect_stdout(io.StringIO()):
            source = extract(self.out, self.root / 'extracted')
        self.assertTrue((source / 'lvgl/lv_version.h').is_file())
        self.assertFalse((source / 'private.txt').exists())
        self.assertFalse(any(source.rglob('.git')))
        self.assertFalse(any((source / 'rlvgl').glob('**/*')))

    def test_existing_tag_and_version_and_main_ancestry(self):
        self.make(tag='v0.1.0')
        self.assertEqual(verify(self.out)['release_tag'], 'v0.1.0')
        with self.assertRaisesRegex(ValueError, 'project version'):
            self.make(self.root / 'wrong', 'v0.2.0')
        (self.repo / 'another').write_text('new commit')
        self.commit(self.repo)
        with self.assertRaisesRegex(ValueError, 'packaged commit'):
            self.make(self.root / 'moved', 'v0.1.0')
        git(self.repo, 'tag', '-f', 'v0.1.0')
        with self.assertRaises(subprocess.CalledProcessError):
            self.make(self.root / 'off-main', 'v0.1.0')

    def test_gitlink_and_declared_tag_mismatch(self):
        self.policy['lvgl_commit'] = '0' * 40
        self.update_policy()
        with self.assertRaisesRegex(ValueError, 'gitlink'):
            self.make()
        self.policy['lvgl_commit'] = self.lvgl_sha
        self.policy['lvgl_tag'] = 'v9.6.0'
        self.update_policy()
        git(self.repo / 'lvgl', 'tag', 'v9.6.0')
        with self.assertRaisesRegex(ValueError, 'version macros'):
            self.make()

    def test_notice_required_and_no_overwrite(self):
        git(self.repo, 'rm', 'LICENSE')
        git(self.repo, 'commit', '-qm', 'missing notice')
        with self.assertRaises(subprocess.CalledProcessError):
            self.make()
        git(self.repo, 'checkout', 'HEAD~1', '--', 'LICENSE')
        git(self.repo, 'commit', '-qm', 'restore notice')
        self.make(self.root / 'new')
        with self.assertRaisesRegex(ValueError, 'empty'):
            self.make(self.root / 'new')

    def test_corrupted_missing_and_unexpected_assets(self):
        self.make()
        (self.out / 'manifest.json').write_text('{}')
        with self.assertRaisesRegex(ValueError, 'Checksum mismatch'):
            verify(self.out)
        (self.out / 'manifest.json').unlink()
        with self.assertRaises(FileNotFoundError):
            verify(self.out)
        (self.out / 'unexpected').write_text('extra')
        # Restore a new valid set to test extra assets separately.
        self.make(self.root / 'valid')
        (self.root / 'valid/extra').write_text('extra')
        with self.assertRaisesRegex(ValueError, 'Unexpected or missing'):
            verify(self.root / 'valid')

    def test_buildroot_hash_consistency(self):
        self.make()
        (self.out / 'lvglpp.hash').write_text('sha256  ' + '0' * 64 + '  LICENSE\n')
        self.refresh_sums()
        with self.assertRaisesRegex(ValueError, 'Buildroot hashes'):
            verify(self.out)

    def test_draft_publication_is_bounded_and_retry_safe(self):
        self.make(tag='v0.1.0')
        manifest = verify(self.out)
        commit = manifest['lvglpp']['commit']
        existing = {'draft': True, 'target_commitish': commit,
                    'html_url': 'https://example.invalid/draft',
                    'assets': [{'name': p.name, 'digest': 'sha256:' + digest(p)}
                               for p in self.out.iterdir()]}
        def invoke(release, remote_commit=commit, status='ahead'):
            def response(path, missing_ok=False):
                if '/git/ref/' in path:
                    return {'object': {'type': 'commit', 'sha': remote_commit}}
                if '/compare/' in path:
                    return {'status': status}
                return release
            with patch.object(sys, 'argv', ['draft_release.py', 'v0.1.0', str(self.out)]), \
                 patch.dict(os.environ, {'GITHUB_REPOSITORY': 'SoftOboros/lvglpp'}), \
                 patch.object(draft_release, 'api', side_effect=response), \
                 patch.object(draft_release.subprocess, 'run') as run, \
                 contextlib.redirect_stdout(io.StringIO()):
                draft_release.main()
                return run
        created = invoke(None)
        command = created.call_args.args[0]
        self.assertIn('--draft', command)
        self.assertIn('--verify-tag', command)
        self.assertNotIn('--clobber', command)
        self.assertFalse(invoke(existing).called)
        with self.assertRaisesRegex(ValueError, 'Remote release tag moved'):
            invoke(None, remote_commit='0' * 40)
        with self.assertRaisesRegex(ValueError, 'no longer on main'):
            invoke(None, status='diverged')
        existing['draft'] = False
        with self.assertRaisesRegex(ValueError, 'refusing to overwrite'):
            invoke(existing)
        existing['draft'] = True
        existing['assets'].pop()
        with self.assertRaisesRegex(ValueError, 'refusing to overwrite'):
            invoke(existing)

    def test_unsafe_archive_is_rejected_before_extraction(self):
        self.make()
        manifest = verify(self.out)
        asset = self.out / manifest['lvglpp']['archive']
        with tarfile.open(asset, 'w:gz') as target:
            member = tarfile.TarInfo(manifest['lvglpp']['root'] + '/../../escape')
            member.size = 3
            target.addfile(member, io.BytesIO(b'bad'))
        manifest['lvglpp']['sha256'] = digest(asset)
        (self.out / 'manifest.json').write_text(json.dumps(manifest))
        self.refresh_sums()
        with self.assertRaisesRegex(ValueError, 'Unsafe archive'):
            extract(self.out, self.root / 'unsafe')
        self.assertFalse((self.root / 'unsafe').exists())


if __name__ == '__main__':
    unittest.main()
