#!/usr/bin/env python3
"""Check the Lab 2 PDF against the verified source and saved QEMU evidence."""

import hashlib
import json
from pathlib import Path
import re
import subprocess

import pymupdf

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / 'docs/lab02/validation'
FIGURES = ROOT / 'docs/lab02/figures'
document = pymupdf.open(ROOT / 'OS2Lab_Report.pdf')
texts = [page.get_text() for page in document]
all_text = '\n'.join(texts)
checks = []


def check(condition, description):
    checks.append({'passed': bool(condition), 'description': description})
    print(('PASS: ' if condition else 'FAIL: ') + description)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def compact(text):
    return re.sub(r'\s+', '', text)


check(8 <= len(document) <= 12, f'concise report with {len(document)} pages')
cover = compact(texts[0])
check(all(value in cover for value in ('Lab02', 'CQ2024/22', 'LêViếtLong', 'PhạmTuấnSơn',
      'PhanLêĐăngKhoa', '24120347', 'ÂuBảoTrân', '24120468', 'DươngNhựtDuy', '24120293')),
      'cover preserves confirmed academic and student information')
check(bool(document[0].get_images()), 'university logo is embedded on the cover')
check('Dương Nhựt Duy' in document.metadata.get('author', ''), 'PDF metadata contains the correct author name')
check(all(word in all_text for word in ('sysinfo', 'ptree', 'trace', 'copyout', 'RQ1', 'RQ2', 'RQ3')),
      'report covers all syscalls and evaluation questions')
check('Tài liệu tham khảo' in all_text and 'pdos.csail.mit.edu' in all_text,
      'report includes primary references')
check(not re.search(r'TODO|FIXME|24120xxx|\?\?|�', all_text),
      'no drafting placeholders, replacement glyphs or unresolved references')
font_names = {font[3] for page in document for font in page.get_fonts()}
check(any('TeXGyreTermes' in name for name in font_names), 'academic Termes font is embedded')
check(all(font[1] not in ('', 'n/a') for page in document for font in page.get_fonts()),
      'all PDF fonts are embedded')
overflow = []
for number, page in enumerate(document, 1):
    check(abs(page.rect.width - 595.276) < 1 and abs(page.rect.height - 841.89) < 1,
          f'page {number} is A4')
    for block in page.get_text('dict')['blocks']:
        for line in block.get('lines', []):
            for span in line['spans']:
                x0, y0, x1, y1 = span['bbox']
                if x0 < 50 or x1 > page.rect.width - 50 or y0 < 20 or y1 > page.rect.height - 20:
                    overflow.append({'page': number, 'text': span['text'], 'bbox': span['bbox']})
check(not overflow, 'text, tables and figures stay within page bounds')
build = (EVIDENCE / 'pdf-build.txt').read_text()
check(not re.search(r'Overfull|Missing character|undefined|newer than current output', build, re.I),
      'final PDF build has no overflow, missing glyph or reference/version warnings')

context = json.loads((EVIDENCE / 'run-context.json').read_text())
check(len(context['runs']) == 6 and all(item['returncode'] == 0 for item in context['runs']),
      'isolated clean, build and all four test commands succeeded')
check(all(digest(EVIDENCE / name) == value for name, value in context['evidence_sha256'].items()),
      'verification logs match the immutable run hashes')
for filename, groups, scores in [('trace-tests.txt', 7, ['70/70']),
                                 ('integration-tests.txt', 9, ['90/90']),
                                 ('lab01-tests.txt', 17, ['50/50', '70/70'])]:
    log = (EVIDENCE / filename).read_text()
    check(len(re.findall(r'^== Test ', log, re.M)) == groups and ': FAIL' not in log
          and all('Score: ' + score in log for score in scores),
          f'{filename} matches the reported test groups and scores')
check('ALL TESTS PASSED' in (EVIDENCE / 'qemu-usertests.txt').read_text(),
      'xv6 quick kernel regression passed')
check('ptree rejects buffer at unaligned logical break' in (EVIDENCE / 'before-badbuf.txt').read_text()
      and 'ptree clears unused name bytes' in (EVIDENCE / 'before-ptree.txt').read_text(),
      'claimed pre-fix failures are preserved')
for name, sha in context['reviewed_branches'].items():
    result = subprocess.run(['git', 'merge-base', '--is-ancestor', sha, 'HEAD'], cwd=ROOT)
    check(result.returncode == 0, f'{name} handoff is preserved in the integrated history')
result = subprocess.run(['git', 'diff', '--quiet', context['commit'], '--', 'kernel', 'user',
                         'tests', 'Makefile', 'conf', 'gradelib.py', 'grade-lab02',
                         'grade-lab02-kernel', 'grade-lab02-trace', 'grade-tv1',
                         'grade-lab01', 'grade-lab-util'], cwd=ROOT)
check(result.returncode == 0, 'current code matches the exact source tested for the report')
check(context['commit'][:7] in all_text, 'report identifies the verified code commit')

manifest = json.loads((FIGURES / 'manifest.json').read_text())
check(manifest['verified_commit'] == context['commit'], 'figures identify the same verified commit')
check(digest(FIGURES / 'architecture.pdf') == manifest['architecture']['pdf_sha256'],
      'architecture schematic matches its manifest')
for item in manifest['figures']:
    source = ROOT / item['source']
    excerpt = FIGURES / (item['name'] + '.txt')
    lines = source.read_text().splitlines()
    selected = '\n'.join(lines[n - 1] for n in item['selected_line_numbers']) + '\n'
    check(digest(source) == item['source_sha256'] and selected == excerpt.read_text()
          and digest(excerpt) == item['excerpt_sha256']
          and digest(FIGURES / (item['name'] + '.pdf')) == item['pdf_sha256'],
          item['name'] + ' figure is traceable to actual QEMU log lines')
    check(compact(selected) in compact(all_text), item['name'] + ' excerpt is selectable in the report')

check(digest(ROOT / 'OS2Lab_Report.pdf') == digest(ROOT / '24120293_24120347_24120468_Report.pdf'),
      'student-ID report copy matches the canonical PDF')
result = {'pdf': 'OS2Lab_Report.pdf', 'pages': len(document),
          'source_sha256': digest(ROOT / 'OS2Lab_Report.tex'),
          'pdf_sha256': digest(ROOT / 'OS2Lab_Report.pdf'),
          'verified_code_commit': context['commit'], 'checks': checks, 'overflow': overflow}
(EVIDENCE / 'pdf-validation.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n')
raise SystemExit(any(not item['passed'] for item in checks))
