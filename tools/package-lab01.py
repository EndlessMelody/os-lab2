#!/usr/bin/env python3
"""Verify the integrated Lab 01 patch and archive clean source for reporting."""

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
        raise SystemExit('Commit the integration changes before packaging.')
    baseline = (root / 'docs/tv1/BASELINE').read_text().strip()
    commit = git('rev-parse', 'HEAD').decode().strip()
    run('git', 'merge-base', '--is-ancestor', baseline, commit, cwd=root)
    for program in ['sleep', 'pingpong', 'primes', 'cp', 'diff', 'tree', 'du']:
        if not (root / f'user/{program}.c').is_file():
            raise SystemExit(f'Missing Lab 01 program: {program}')

    output = root / 'submissions/lab01'
    output.mkdir(parents=True, exist_ok=True)
    patch = output / 'Lab01.patch'
    patch.write_bytes(git('diff', '--binary', baseline, commit))

    with tempfile.TemporaryDirectory(prefix='xv6-lab01-') as directory:
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

        expected = set(git('ls-tree', '-r', '--name-only', commit).decode().splitlines())
        actual = {str(path.relative_to(source))
                  for path in source.rglob('*') if path.is_file()}
        if actual != expected:
            raise SystemExit('Clean source file list differs from the commit.')
        # Ensure the patch reconstructs the committed file contents as well.
        for name in expected:
            if (source / name).read_bytes() != git('show', f'{commit}:{name}'):
                raise SystemExit(f'Patched source differs from the commit: {name}')

        source_zip = output / 'xv6-labs-2024.zip'
        with zipfile.ZipFile(source_zip, 'w', zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(source.rglob('*')):
                if path.is_file():
                    archive.write(path, path.relative_to(source.parent))

    (output / 'HANDOFF.txt').write_text(
        'Integrated seven-program Lab 01 code handoff for the report phase.\n'
        f'Baseline: {baseline}\nCommit: {commit}\n'
        'Patch applied to the clean baseline and built successfully.\n'
        'All source contents match the commit; make clean ran before ZIP.\n'
        'Pending final submission: report PDF, student IDs and group approval.\n'
    )
    handoff = output / 'Lab01-code-handoff.zip'
    with zipfile.ZipFile(handoff, 'w', zipfile.ZIP_DEFLATED) as archive:
        archive.write(patch, 'Realease/Lab01.patch')
        archive.write(source_zip, 'Source/xv6-labs-2024.zip')
        archive.write(output / 'HANDOFF.txt', 'HANDOFF.txt')
        archive.write(output / 'verify-build.txt', 'verify-build.txt')
        for path in sorted((root / 'docs/lab01').iterdir()):
            if path.is_file():
                archive.write(path, 'docs/' + path.name)
    print(f'Patch verified and clean source archived: {handoff}')


if __name__ == '__main__':
    main()
