#!/usr/bin/env python3
"""Render the Lab 2 architecture and verbatim QEMU excerpts for the report."""

import hashlib
import json
from pathlib import Path

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch
from matplotlib.font_manager import FontProperties

matplotlib.rcParams['pdf.fonttype'] = 42

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'docs/lab02/figures'
OUT.mkdir(parents=True, exist_ok=True)
EVIDENCE = ROOT / 'docs/lab02/validation'
context = json.loads((EVIDENCE / 'run-context.json').read_text())
manifest = {'verified_commit': context['commit'], 'figures': []}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


fig, ax = plt.subplots(figsize=(10.6, 3.3))
fig.patch.set_facecolor('white')
ax.set_xlim(0, 10.6)
ax.set_ylim(0, 3.3)
ax.axis('off')


def box(x, y, w, h, label, fill='#F2F5F7'):
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle='round,pad=0.08',
                               linewidth=0.9, edgecolor='#CDD3D8', facecolor=fill))
    ax.text(x + w / 2, y + h / 2, label, ha='center', va='center',
            fontsize=15, color='#18384D')


for x, label in [(0.25, 'trace + command'), (3.85, 'sysinfotest'), (7.45, 'pstree')]:
    box(x, 2.6, 2.9, 0.43, label)
box(0.25, 1.35, 10.1, 0.6,
    'user stub  /  ecall  /  syscall dispatcher  /  handler', '#E7EEF3')
for x, label in [(0.25, 'proc.tracemask\nfork -> inherited mask'),
                 (3.85, 'kmem.freelist + proc[]\nlocks -> sysinfo'),
                 (7.45, 'proc[] -> ptree records\ncopyout -> user tree')]:
    box(x, 0.1, 2.9, 0.65, label)
    for a, b in [(2.6, 1.97), (1.35, 0.78)]:
        ax.annotate('', (x + 1.45, b), (x + 1.45, a),
                    arrowprops={'arrowstyle': '-|>', 'color': '#606A73', 'lw': 1.1})
fig.savefig(OUT / 'architecture.pdf', bbox_inches='tight', pad_inches=0.08)
fig.savefig(OUT / 'architecture.png', dpi=180, bbox_inches='tight', pad_inches=0.08)
plt.close(fig)
manifest['architecture'] = {'kind': 'design schematic',
                            'pdf_sha256': digest(OUT / 'architecture.pdf')}

font = FontProperties(family='DejaVu Sans Mono', size=11)

for name, logfile, start, stop in [
    ('combined', 'qemu-combined.txt', '$ trace 25165824', 'LAB02_SNAPSHOTS_OK'),
    ('pstree', 'qemu-pstree.txt', '$ pstree', '$ qemu-system'),
]:
    source = EVIDENCE / logfile
    lines = source.read_text().splitlines()
    first = next(i for i, line in enumerate(lines) if line.startswith(start))
    if name == 'combined':
        last = next(i + 1 for i in range(first, len(lines)) if lines[i] == stop)
    else:
        last = next(i for i in range(first + 1, len(lines)) if lines[i].startswith(stop))
    selected = lines[first:last]
    text = '\n'.join(selected) + '\n'
    excerpt = OUT / (name + '.txt')
    excerpt.write_text(text)
    width, size, leading = 520, 11, 15
    height = 30 + leading * len(selected)
    fig = plt.figure(figsize=(width / 72, height / 72), facecolor='#F5F7F8')
    fig.add_artist(plt.Rectangle((0.001, 0.003), 0.998, 0.994,
                                transform=fig.transFigure, facecolor='none',
                                edgecolor='#CDD3D8', linewidth=0.6))
    for i, line in enumerate(selected):
        fig.text(18 / width, 1 - (23 + i * leading) / height, line,
                 fontproperties=font, color='#20252B')
    pdf = OUT / (name + '.pdf')
    fig.savefig(pdf)
    fig.savefig(OUT / (name + '.png'), dpi=144)
    plt.close(fig)
    manifest['figures'].append({
        'name': name, 'source': str(source.relative_to(ROOT)),
        'source_sha256': digest(source),
        'selected_line_numbers': list(range(first + 1, last + 1)),
        'excerpt_sha256': digest(excerpt), 'pdf_sha256': digest(pdf), 'font_size_pt': size,
    })

(OUT / 'manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n')
print('Rendered architecture and two QEMU excerpts with source hashes.')
