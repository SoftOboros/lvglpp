"""Exercise the real formatter gate in isolated Git checkouts."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / 'scripts/format.py'


class FormattingGateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.formatter = (os.environ.get('CLANG_FORMAT') or
                         shutil.which('clang-format-14') or shutil.which('clang-format'))
        if not cls.formatter:
            raise RuntimeError('Set CLANG_FORMAT to LLVM clang-format 14.0.6')

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='lvglpp format ')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / 'checkout with spaces'
        self.root.mkdir()
        self.git('init', '-q')
        (self.root / '.clang-format').write_bytes((ROOT / '.clang-format').read_bytes())
        (self.root / '.gitignore').write_text('build/\n')
        (self.root / 'tracked.cpp').write_text('int main() { return 0; }\n')
        self.git('add', '.clang-format', '.gitignore', 'tracked.cpp')

    def git(self, *args):
        return subprocess.run(['git', '-C', str(self.root), *args],
                              capture_output=True, text=True, check=True)

    def gate(self, mode, formatter=None):
        return subprocess.run([sys.executable, str(SCRIPT), '--root', str(self.root),
                               mode, '--clang-format', formatter or self.formatter],
                              capture_output=True, text=True)

    def test_drift_fails_read_only_then_fix_is_idempotent(self):
        source = self.root / 'tracked.cpp'
        source.write_text('int main(){return 0;}\n')
        before = source.read_bytes()
        self.assertNotEqual(self.gate('--check').returncode, 0)
        self.assertEqual(source.read_bytes(), before)
        result = self.gate('--fix')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.gate('--check').returncode, 0)
        formatted = source.read_bytes()
        self.assertEqual(self.gate('--fix').returncode, 0)
        self.assertEqual(source.read_bytes(), formatted)

    def test_inventory_includes_untracked_spaces_excludes_build_and_submodules(self):
        untracked = self.root / 'new header with spaces.hpp'
        untracked.write_text('inline int f(){return 1;}\n')
        excluded = []
        for directory in ['build', 'lvgl', 'rlvgl']:
            folder = self.root / directory
            folder.mkdir()
            source = folder / 'external.cpp'
            source.write_text('int f(){return 0;}\n')
            excluded.append(source)
            if directory != 'build':
                self.git('update-index', '--add', '--cacheinfo',
                         '160000,' + '1' * 40 + ',' + directory)
        before = [p.read_bytes() for p in excluded]
        result = self.gate('--fix')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('2 owned C/C++ files', result.stdout)
        self.assertEqual([p.read_bytes() for p in excluded], before)
        self.assertEqual(self.gate('--check').returncode, 0)

    def test_wrong_formatter_version_fails_before_mutation(self):
        fake = Path(self.temp.name) / 'wrong formatter'
        fake.write_text('#!' + sys.executable + '\nprint("clang-format version 22.0.0")\n')
        fake.chmod(0o755)
        before = (self.root / 'tracked.cpp').read_bytes()
        result = self.gate('--fix', str(fake))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Expected LLVM clang-format 14.0.6', result.stderr)
        self.assertEqual((self.root / 'tracked.cpp').read_bytes(), before)

    def test_missing_formatter_fails_closed(self):
        result = self.gate('--check', str(self.root / 'missing-formatter'))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Install LLVM clang-format 14.0.6', result.stderr)

    def test_symlink_cannot_format_external_source(self):
        external = Path(self.temp.name) / 'outside.cpp'
        external.write_text('int f(){return 0;}\n')
        (self.root / 'linked.cpp').symlink_to(external)
        before = external.read_bytes()
        result = self.gate('--fix')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Refusing to format a symlink', result.stderr)
        self.assertEqual(external.read_bytes(), before)


if __name__ == '__main__':
    unittest.main()
