#!/usr/bin/env python3
"""Verify an immutable Lab 2 commit in an isolated directory and record evidence."""

import argparse
import hashlib
import io
import json
import platform
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
import time
from datetime import datetime
from zoneinfo import ZoneInfo

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--commit', default='HEAD')
args = parser.parse_args()


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)


def normalize(path):
    path.write_text('\n'.join(line.rstrip() for line in path.read_text().splitlines()) + '\n')


commit = git('rev-parse', args.commit + '^{commit}').decode().strip()
out = ROOT / 'docs/lab02/validation'
out.mkdir(parents=True, exist_ok=True)
context = {
    'commit': commit,
    'date': datetime.now(ZoneInfo('Asia/Bangkok')).isoformat(),
    'environment': {'host': platform.platform(), 'python': platform.python_version(),
                    'qemu_cpus': 3, 'qemu_memory_mib': 128},
    'reviewed_branches': {},
    'runs': [],
}
for name in ('lab2-trace', 'lab2-sysinfo', 'lab2-ptree'):
    context['reviewed_branches'][name] = git('rev-parse', 'origin/' + name).decode().strip()
for name, command in [('gcc', ['riscv64-linux-gnu-gcc', '--version']),
                      ('qemu', ['qemu-system-riscv64', '--version']),
                      ('gdb', ['gdb-multiarch', '--version'])]:
    context['environment'][name] = subprocess.check_output(command, text=True).splitlines()[0]

with tempfile.TemporaryDirectory(prefix='lab02-verify-') as directory:
    snapshot = Path(directory)
    with tarfile.open(fileobj=io.BytesIO(git('archive', commit)), mode='r:') as archive:
        archive.extractall(snapshot, filter='data')
    commands = [
        ('clean.txt', ['make', 'clean']),
        ('build.txt', ['make', '-j4', 'kernel/kernel', 'fs.img']),
        ('trace-tests.txt', ['make', 'check-trace']),
        ('integration-tests.txt', ['make', 'check-lab02']),
        ('lab01-tests.txt', ['make', 'check-lab01']),
        ('kernel-tests.txt', ['./grade-lab02-kernel']),
    ]
    for filename, command in commands:
        print('$ ' + ' '.join(command), flush=True)
        started = time.monotonic()
        with (out / filename).open('wb') as log:
            result = subprocess.run(command, cwd=snapshot, stdout=log, stderr=subprocess.STDOUT)
        normalize(out / filename)
        context['runs'].append({'command': command, 'log': filename,
                                'returncode': result.returncode,
                                'elapsed_seconds': round(time.monotonic() - started, 2)})
        print(f'  exit={result.returncode}', flush=True)
        if result.returncode:
            (out / 'run-context.json').write_text(json.dumps(context, ensure_ascii=False, indent=2) + '\n')
            raise SystemExit(f'Verification failed; see {out / filename}')
        if filename == 'build.txt':
            # Some WSL filesystems briefly return mtimes ahead of the clock.
            # Wait for make's inputs to settle before running incremental builds.
            latest = max(p.stat().st_mtime for p in snapshot.rglob('*') if p.is_file())
            delay = latest - time.time() + 1
            if 0 < delay < 30:
                time.sleep(delay)

    for source in (snapshot / 'docs/lab02').glob('trace-*.txt'):
        if source.name not in ('trace-build.txt', 'trace-clean.txt', 'trace-selftest.txt',
                               'trace-lab01-regression.txt'):
            shutil.copyfile(source, out / source.name)
    for source in (snapshot / 'docs/lab02/validation').glob('qemu-*.txt'):
        shutil.copyfile(source, out / source.name)

for path in out.glob('*.txt'):
    normalize(path)
context['evidence_sha256'] = {
    p.name: hashlib.sha256(p.read_bytes()).hexdigest()
    for p in sorted(out.glob('*.txt'))
}
(out / 'run-context.json').write_text(json.dumps(context, ensure_ascii=False, indent=2) + '\n')
print(f'All checks passed on {commit}; evidence: {out}', flush=True)
