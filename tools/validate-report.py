#!/usr/bin/env python3
"""Check the rendered Lab 01 report and the evidence supporting its results.

Usage: python3 tools/validate-report.py [PDF] [--tex path/to/report.tex]
Requires PyMuPDF (python3 -m pip install pymupdf).
Cover placeholders are reported for review; they are deliberately not invented.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re

import pymupdf


ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("pdf", nargs="?", type=Path, default=ROOT / "OS1Lab_Report.pdf")
parser.add_argument("--tex", type=Path, default=ROOT / "OS1Lab_Report.tex")
args = parser.parse_args()
pdf_path = args.pdf
evidence = ROOT / "docs/report-validation"
document = pymupdf.open(pdf_path)
texts = [page.get_text() for page in document]
failures = []


def check(condition, description):
    print(("PASS: " if condition else "FAIL: ") + description)
    if not condition:
        failures.append(description)


# Find actual section headings, excluding their entries on the contents page.
intro = [i for i, text in enumerate(texts) if i >= 2 and re.search(r"(?m)^1\s+Giới thiệu\s*$", text)]
ending = [i for i, text in enumerate(texts) if i >= 2 and re.search(r"(?m)^6\s+Kết luận\s*$", text)]
check(len(intro) == len(ending) == 1, "unique introduction and conclusion headings")
if intro and ending:
    count = ending[0] - intro[0]
    check(7 <= count <= 8, f"main content occupies {count} pages (target 7-8)")
    check(intro[0] == 2, "cover and one contents page precede the main content")
    body = "\n".join(texts[intro[0]:ending[0]])
    check("Cần xác nhận" not in body, "no unresolved placeholders in main content")
    check("Tài liệu tham khảo" in "\n".join(texts[ending[0]:]), "references present")
    check(re.search(rf"(?m)^{count}\s*$", texts[ending[0] - 1]),
          "last main-content page has the correct printed page number")

all_text = "\n".join(texts)
cover = re.sub(r"\s+", "", texts[0])
check(all(field in cover for field in (
    "LêViếtLong", "PhạmTuấnSơn", "CQ2024/22", "PhanLêĐăngKhoa", "24120347",
    "ÂuBảoTrân", "24120468", "DươngNhựtDuy", "24120293")),
    "cover matches the user-confirmed names, class and student IDs")
logo = pymupdf.Pixmap(args.tex.parent / "Logo_Truong.png")
check(any(image[2:4] == (logo.width, logo.height) for image in document[0].get_images()),
      "the supplied university logo is embedded on the cover")
check("Dương Nhựt Duy" in document.metadata.get("author", "")
      and "Nguyễn Nhựt Duy" not in all_text
      and "Nguyen Nhut Duy" not in document.metadata.get("author", ""),
      "author name is consistent in the report and PDF metadata")
check(not re.search(r"24120xxx|TODO|FIXME|\[Cần xác nhận[^\]]*\]", all_text),
      "no unfilled personal information or drafting placeholders remain")
for command in ("sleep", "pingpong", "primes", "cp", "tree", "du", "diff"):
    check(command in all_text, f"report includes {command}")
check("�" not in all_text, "no Unicode replacement characters")
check("TODO" not in all_text and "Cần hoàn thành" not in all_text,
      "no internal drafting instructions")
check("??" not in all_text, "no unresolved LaTeX references")
abstract_page = re.sub(r"\s+", "", texts[2])
check("Tómtắt" in abstract_page and "Từkhóa" in abstract_page,
      "academic abstract and keywords are present")
check(all(f"RQ{i}" in all_text for i in range(1, 4)),
      "three evaluation questions are present")
font_names = {font[3] for page in document for font in page.get_fonts()}
check(any("TeXGyreTermes" in name for name in font_names)
      and not any("DejaVu" in name for name in font_names),
      "PDF embeds the new Termes academic font instead of DejaVu")
check(all(font[1] not in ("n/a", "") for page in document for font in page.get_fonts()),
      "all PDF fonts are embedded")
build_log = (evidence / "pdf-build.txt").read_text()
check("Overfull" not in build_log and "Missing character" not in build_log,
      "compiler reports no overflowing boxes or missing glyphs")
check("undefined" not in build_log.lower(), "citations and references are resolved")

# A4 and text bounds: includes captions, tables, footer and terminal excerpts.
overflow = []
for number, page in enumerate(document, 1):
    check(abs(page.rect.width - 595.276) < 1 and abs(page.rect.height - 841.89) < 1,
          f"page {number} uses A4")
    for block in page.get_text("dict")["blocks"]:
        for line in block.get("lines", []):
            for span in line["spans"]:
                x0, y0, x1, y1 = span["bbox"]
                if x0 < 50 or x1 > page.rect.width - 50 or y0 < 20 or y1 > page.rect.height - 20:
                    overflow.append({"page": number, "text": span["text"], "bbox": span["bbox"]})
check(not overflow, "all text remains within page bounds and horizontal margins")

tests = (evidence / "tests.txt").read_text()
context = json.loads((evidence / "run-context.json").read_text())
check(all(item["returncode"] == 0 for item in context["runs"]),
      "clean, build and test commands succeeded")
check(len(re.findall(r"^== Test", tests, re.M)) == 17,
      "all 17 selected test groups ran")
check("Score: 50/50" in tests and "Score: 70/70" in tests,
      "TV1 and integrated scores match the report")
for group, count in (("CP", 13), ("DIFF", 23), ("TREE", 21), ("DU", 17), ("RESOURCES", 12)):
    check(f"LAB01_{group}_OK: {count} cases" in tests, f"{group}: {count} cases verified")
flow = (evidence / "qemu-lab01-flow.txt").read_text()
check("12\tdemo" in flow and "59 118 561" in flow and "LAB01_FLOW_OK" in flow,
      "integration transcript matches the actual QEMU output")
size = (evidence / "qemu-lab01-size.txt").read_text()
check(all(value in size for value in ("0\tzero", "4294967296\twide", "18446744073709551615\tmaximum")),
      "64-bit formatting evidence matches the report")

# Check the new figures against their recorded sources, not just their captions.
upgrade = evidence / "upgrade"
upgrade_context = json.loads((upgrade / "run-context.json").read_text())
upgrade_tests = (upgrade / "tests.txt").read_text()
check(upgrade_context["commit"] == context["commit"]
      and all(item["returncode"] == 0 for item in upgrade_context["runs"])
      and len(upgrade_context["runs"]) == 3,
      "fresh clean/build/test run succeeded on the documented commit")
check(len(re.findall(r"^== Test", upgrade_tests, re.M)) == 17
      and len(re.findall(r": OK \(", upgrade_tests)) == 17
      and "Score: 50/50" in upgrade_tests and "Score: 70/70" in upgrade_tests
      and "FAIL:" not in upgrade_tests,
      "all 17 freshly rerun test groups passed")
check(upgrade_context["diff_example"]["returncode"] == 0
      and "> f2:2: two\n" in (upgrade / "diff-example.txt").read_text()
      and "> f2:2: two" in all_text,
      "diff EOF example in the PDF matches the additional QEMU run")
manifest = json.loads((evidence / "figures/manifest.json").read_text())
check({figure["name"] for figure in manifest["figures"]}
      == {"test-summary", "workflow"}, "both evidence figures have provenance records")


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


normalize = lambda value: re.sub(r"\s+", "", value)
for figure in manifest["figures"]:
    source_path = ROOT / figure["source"]
    excerpt_path = evidence / f"figures/{figure['name']}.txt"
    figure_path = evidence / f"figures/{figure['name']}.pdf"
    source_lines = source_path.read_text().splitlines()
    selected = "\n".join(source_lines[number - 1]
                         for number in figure["selected_line_numbers"]) + "\n"
    check(sha256(source_path) == figure["source_sha256"]
          and sha256(excerpt_path) == figure["text_sha256"]
          and sha256(figure_path) == figure["pdf_sha256"]
          and excerpt_path.read_text() == selected,
          f"{figure['name']}: figure and excerpt match the recorded log")
    check(any(normalize(selected) in normalize(text) for text in texts),
          f"{figure['name']}: complete excerpt is readable/selectable in the report")
    check(figure["font_size_pt"] >= 9,
          f"{figure['name']}: source text is at least 9 pt")
check("Hình 1." in all_text and "Hình 2." in all_text,
      "both figures have numbered captions")
check(sha256(args.tex) == context["report"]["source_sha256"]
      and sha256(pdf_path) == context["report"]["pdf_sha256"],
      "report source and PDF match the updated verification record")
pending = sorted(set(re.findall(r"\[Cần xác nhận:[^\]]*\]|24120xxx", texts[0])))
result = {"pdf": pdf_path.name, "pdf_pages": len(document),
          "main_content_pages": ending[0] - intro[0] if intro and ending else None,
          "failures": failures, "overflow": overflow, "cover_pending": pending}
(evidence / "pdf-validation.json").write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n")
print("Cover information awaiting the user:", len(pending), "distinct placeholders")
raise SystemExit(bool(failures))
