#!/usr/bin/env python3
"""Fix/check owned C/C++ sources with the same LLVM formatter as release CI."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

FORMAT_VERSION = '14.0.6'
ROOT = Path(__file__).resolve().parents[1]


def source_files(root):
    top = subprocess.check_output(
        ['git', '-C', str(root), 'rev-parse', '--show-toplevel'], text=True)
    if Path(top.strip()).resolve() != root:
        raise ValueError('Formatting requires the lvglpp checkout root')
    if not (root / '.clang-format').is_file():
        raise ValueError('Missing .clang-format policy')
    entries = subprocess.check_output([
        'git', '-C', str(root), 'ls-files', '--cached', '--others',
        '--exclude-standard', '-z', '--', '*.cpp', '*.hpp', '*.h'])
    files = []
    for name in sorted(set(os.fsdecode(p) for p in entries.split(b'\0') if p)):
        path = root / name
        if path.is_symlink():
            raise ValueError('Refusing to format a symlink: ' + name)
        # Locally deleted tracked files are absent; Git excludes ignored
        # build/generated output and does not recurse into submodules.
        if path.is_file():
            files.append(path)
    if not files:
        raise ValueError('No owned C/C++ files found')
    return files


def formatter_path(requested):
    names = [requested] if requested else ['clang-format-14', 'clang-format']
    executable = next((shutil.which(name) for name in names if shutil.which(name)), None)
    if not executable:
        raise ValueError('Install LLVM clang-format ' + FORMAT_VERSION +
                         ' or set CLANG_FORMAT to its executable')
    version = subprocess.check_output([executable, '--version'], text=True)
    found = re.search(r'\bversion\s+(\d+\.\d+\.\d+)\b', version)
    if not found or found.group(1) != FORMAT_VERSION:
        raise ValueError('Expected LLVM clang-format ' + FORMAT_VERSION +
                         '; found ' + version.strip())
    return executable


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--fix', action='store_true')
    mode.add_argument('--check', action='store_true')
    parser.add_argument('--clang-format', default=os.environ.get('CLANG_FORMAT', ''))
    parser.add_argument('--root', type=Path, default=ROOT)
    args = parser.parse_args(argv)
    root = args.root.resolve()
    executable = formatter_path(args.clang_format or os.environ.get('CLANG_FORMAT', ''))
    files = source_files(root)
    flags = ['-i'] if args.fix else ['--dry-run', '--Werror']
    for start in range(0, len(files), 50):
        subprocess.run([executable, '--style=file', *flags,
                        *[str(p) for p in files[start:start + 50]]], cwd=root, check=True)
    print(('Formatted' if args.fix else 'Checked') + ' ' + str(len(files)) +
          ' owned C/C++ files with LLVM clang-format ' + FORMAT_VERSION)


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        print('Formatting failed: ' + str(error), file=sys.stderr)
        sys.exit(1)
