#!/usr/bin/env python3
"""Typeset exact excerpts of recorded test logs as PDF/PNG report figures.

These figures are log visualizations, not terminal-window screenshots.
Requires PyMuPDF. Pass --font if TeX Gyre Cursor is not in Tectonic's cache.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re

import pymupdf


ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / "docs/report-validation"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def render(name, title, source, selected, font_path, outdir):
    # Preserve original content, including tabs and trailing whitespace, in TXT.
    excerpt = "\n".join(line for _, line in selected) + "\n"
    (outdir / f"{name}.txt").write_text(excerpt)
    display = [line.expandtabs(8) for _, line in selected]
    font = pymupdf.Font(fontfile=str(font_path))
    width, padding, header = 465, 10, 22
    fontsize = min(9.5, (width - 2 * padding) /
                   max(font.text_length(line, fontsize=1) for line in display))
    if fontsize < 8.5:
        raise ValueError(f"{name}: excerpt would be too small to print")
    leading = fontsize * 1.18
    height = header + 2 * padding + len(display) * leading
    doc = pymupdf.open()
    page = doc.new_page(width=width, height=height)
    page.insert_font(fontname="EvidenceMono", fontfile=str(font_path))
    page.draw_rect(page.rect, color=(0.80, 0.83, 0.85),
                   fill=(0.965, 0.975, 0.982), width=0.5)
    page.draw_rect(pymupdf.Rect(0, 0, width, header),
                   color=None, fill=(0.094, 0.22, 0.30))
    page.insert_text((padding, 16), title, fontname="EvidenceMono",
                     fontsize=9, color=(1, 1, 1))
    for i, line in enumerate(display):
        color = (0.13, 0.16, 0.19)
        if line.startswith(("$ ", "./grade-")):
            color = (0.08, 0.27, 0.42)
        elif line.startswith(("Score:", "LAB01_")):
            color = (0.10, 0.33, 0.23)
        page.insert_text((padding, header + padding + fontsize + i * leading),
                         line, fontname="EvidenceMono", fontsize=fontsize,
                         color=color)
    pdf = outdir / f"{name}.pdf"
    # Only basic page/text/path/font objects are used. Declare PDF 1.5 so
    # XeTeX's image reader can import the figures without a version warning.
    payload = doc.tobytes(garbage=4, deflate=True)
    if not payload.startswith(b"%PDF-1.7"):
        raise ValueError("Unexpected PyMuPDF PDF header")
    pdf.write_bytes(b"%PDF-1.5" + payload[len(b"%PDF-1.7"):])
    page.get_pixmap(matrix=pymupdf.Matrix(3, 3)).save(outdir / f"{name}.png")
    # Verify the figure still contains the actual transcript as selectable text.
    actual = page.get_text().splitlines()
    if [x.rstrip() for x in actual[1:]] != [x.rstrip() for x in display]:
        raise ValueError(f"{name}: rendered text does not match the excerpt")
    return {
        "name": name,
        "source": str(source.relative_to(ROOT)),
        "source_sha256": digest(source),
        "selected_line_numbers": [number for number, _ in selected],
        "selection": "filtered lines" if name == "test-summary" else "continuous excerpt",
        "text_sha256": digest(outdir / f"{name}.txt"),
        "pdf_sha256": digest(pdf),
        "font_size_pt": round(fontsize, 2),
        "size_pt": [width, round(height, 2)],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font", type=Path)
    args = parser.parse_args()
    if args.font:
        font_path = args.font
    else:
        font_path = next((Path.home() / ".cache/tectonic").rglob(
            "texgyrecursor-regular.otf"), None)
    if font_path is None or not font_path.is_file():
        parser.error("TeX Gyre Cursor font not found; supply --font PATH")
    outdir = EVIDENCE / "figures"
    outdir.mkdir(exist_ok=True)
    source = EVIDENCE / "upgrade/tests.txt"
    lines = source.read_text().splitlines()
    pattern = re.compile(r"^(?:\./grade-|Score:|LAB01_(?:CP|DIFF|TREE|DU|RESOURCES)_OK:)")
    selected = [(i, line) for i, line in enumerate(lines, 1) if pattern.search(line)]
    if len(selected) != 10 or any("FAIL" in line for _, line in selected):
        raise ValueError("Unexpected test-summary transcript")
    records = [render("test-summary", "TEST RESULTS | filtered log excerpt",
                      source, selected, font_path, outdir)]
    source = EVIDENCE / "upgrade/qemu.lab01.flow.txt"
    lines = source.read_text().splitlines()
    start = lines.index("$ mkdir demo")
    end = next(i for i in range(start, len(lines)) if lines[i].startswith("59 118 561"))
    selected = [(i + 1, lines[i]) for i in range(start, end + 1)]
    records.append(render("workflow", "xv6 / QEMU | continuous log excerpt",
                          source, selected, font_path, outdir))
    manifest = {"kind": "typeset log excerpts; not window screenshots",
                "generator": "tools/render-report-evidence.py",
                "font": font_path.name, "figures": records}
    (outdir / "manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
    for record in records:
        print(record["name"], record["size_pt"], "font", record["font_size_pt"])


if __name__ == "__main__":
    main()
