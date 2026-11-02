#!/usr/bin/env python3
"""Create a TV1 handoff from a clean, committed tree and verify its patch."""

import io
from pathlib import Path
import subprocess
import tarfile
import tempfile
import zipfile

root = Path(__file__).resolve().parents[1]


def git(*args):
    return subprocess.check_output(['git', *args], cwd=root)


def run(*args, cwd, log=None):
    subprocess.run(args, cwd=cwd, check=True, stdout=log, stderr=log)


def main():
    if git('status', '--porcelain').strip():
        raise SystemExit('Commit the handoff changes before packaging.')
    baseline = (root / 'docs/tv1/BASELINE').read_text().strip()
    commit = git('rev-parse', 'HEAD').decode().strip()
    run('git', 'merge-base', '--is-ancestor', baseline, commit, cwd=root)

    output = root / 'submissions/tv1'
    output.mkdir(parents=True, exist_ok=True)
    patch = output / 'TV1.patch'
    patch.write_bytes(git('diff', '--binary', baseline, commit))

    with tempfile.TemporaryDirectory(prefix='xv6-tv1-') as directory:
        source = Path(directory) / 'xv6-labs-2024'
        source.mkdir()
        archive = git('archive', '--format=tar', baseline)
        with tarfile.open(fileobj=io.BytesIO(archive)) as files:
            files.extractall(source, filter='data')
        run('git', 'apply', '--check', str(patch), cwd=source)
        run('git', 'apply', str(patch), cwd=source)
        with (output / 'verify-build.txt').open('w') as log:
            run('make', '-j2', 'kernel/kernel', 'fs.img', cwd=source, log=log)
            run('make', 'clean', cwd=source, log=log)

        # Check that cleanup left exactly the committed source files.
        expected = set(git('ls-tree', '-r', '--name-only', commit).decode().splitlines())
        actual = {str(path.relative_to(source))
                  for path in source.rglob('*') if path.is_file()}
        if actual != expected:
            raise SystemExit('Clean source file list differs from the commit.')

        source_zip = output / 'xv6-labs-2024.zip'
        with zipfile.ZipFile(source_zip, 'w', zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(source.rglob('*')):
                if path.is_file():
                    archive.write(path, path.relative_to(source.parent))

    (output / 'HANDOFF.txt').write_text(
        f'TV1 handoff only; not the final group submission.\n'
        f'Baseline: {baseline}\nCommit: {commit}\n'
        'Patch applied to the clean baseline and built successfully.\n'
        'Source was cleaned with make clean before archiving.\n'
        'Pending: TV2/TV3 programs, cross-review, group PDF and student IDs.\n'
    )
    handoff = output / 'TV1-handoff.zip'
    with zipfile.ZipFile(handoff, 'w', zipfile.ZIP_DEFLATED) as archive:
        archive.write(patch, 'Realease/TV1.patch')
        archive.write(source_zip, 'Source/xv6-labs-2024.zip')
        archive.write(output / 'HANDOFF.txt', 'HANDOFF.txt')
        archive.write(output / 'verify-build.txt', 'verify-build.txt')
        for path in sorted((root / 'docs/tv1').iterdir()):
            if path.is_file():
                archive.write(path, 'docs/' + path.name)
    print(f'Patch applied and built; clean source archived: {handoff}')


if __name__ == '__main__':
    main()
