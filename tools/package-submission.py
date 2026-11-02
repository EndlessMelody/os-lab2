#!/usr/bin/env python3
"""Build the final Lab 01 submission from committed code and validated report.

Untracked report assets are selected explicitly; tracked changes or untracked
runtime source cause a failure. Builds and make clean run only in temp copies.
"""

import argparse
import hashlib
import io
import json
from pathlib import Path
import re
import shutil
import stat
import subprocess
import sys
import tarfile
import tempfile
import zipfile


ROOT = Path(__file__).resolve().parents[1]
PREFIX = "24120293_24120347_24120468"
PROGRAMS = ("sleep", "pingpong", "primes", "cp", "diff", "tree", "du")
EVIDENCE = ROOT / "docs/report-validation"


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT)


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, cwd, log):
    print("Running:", " ".join(map(str, command)), flush=True)
    with log.open("w") as handle:
        subprocess.run(command, cwd=cwd, stdout=handle,
                       stderr=subprocess.STDOUT, check=True)


def files_under(path):
    return {str(p.relative_to(path)): p for p in path.rglob("*") if p.is_file()}


def zip_tree(source, destination):
    with zipfile.ZipFile(destination, "w", zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(source.rglob("*")):
            if path.is_file():
                archive.write(path, path.relative_to(source.parent))


def extract_zip(archive_path, destination):
    with zipfile.ZipFile(archive_path) as archive:
        if archive.testzip() is not None:
            raise ValueError(f"Invalid ZIP: {archive_path}")
        archive.extractall(destination)
        for info in archive.infolist():
            if not info.is_dir():
                (destination / info.filename).chmod(
                    stat.S_IMODE(info.external_attr >> 16))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--date", required=True, help="Local date, YYYY-MM-DD")
    args = parser.parse_args()
    if not re.fullmatch(r"\d{4}-\d{2}-\d{2}", args.date):
        parser.error("--date must be YYYY-MM-DD")
    if git("diff", "HEAD", "--").strip():
        raise SystemExit("Tracked files differ from HEAD; reconcile them before packaging.")
    if git("ls-files", "--others", "--exclude-standard", "--",
           "user", "kernel", "tests", "mkfs").strip():
        raise SystemExit("Untracked runtime source must be reviewed before packaging.")
    baseline = (ROOT / "docs/tv1/BASELINE").read_text().strip()
    context = json.loads((EVIDENCE / "run-context.json").read_text())
    # Report-only commits must not change the tested code snapshot or patch.
    commit = context["commit"]
    subprocess.run(["git", "merge-base", "--is-ancestor", baseline, commit],
                   cwd=ROOT, check=True)
    subprocess.run(["git", "merge-base", "--is-ancestor", commit, "HEAD"],
                   cwd=ROOT, check=True)
    validation = json.loads((EVIDENCE / "pdf-validation.json").read_text())
    if (validation["failures"] or validation["cover_pending"]
            or sha256(ROOT / "OS1Lab_Report.pdf") != context["report"]["pdf_sha256"]
            or sha256(ROOT / "OS1Lab_Report.tex") != context["report"]["source_sha256"]):
        raise SystemExit("Validate the current report and its evidence before packaging.")
    for program in PROGRAMS:
        if not (ROOT / f"user/{program}.c").is_file():
            raise SystemExit(f"Missing utility: {program}")
    runtime = ["Makefile", "gradelib.py", "grade-lab-util", "grade-tv1", "grade-lab01"]
    recorded_runtime = git("ls-tree", "-r", "--name-only", commit, "--",
                           "kernel", "user", "tests", "mkfs", "conf").decode().splitlines()
    if set(recorded_runtime) != set(git("ls-files", "--", "kernel", "user",
                                       "tests", "mkfs", "conf").decode().splitlines()):
        raise SystemExit("Runtime file list differs from the tested commit.")
    runtime += recorded_runtime
    for name in runtime:
        if not (ROOT / name).is_file() or (ROOT / name).read_bytes() != git("show", f"{commit}:{name}"):
            raise SystemExit(f"Runtime source differs from the tested commit: {name}")

    output = ROOT / "submissions/lab01/final"
    verification = output / "verification"
    for folder in (verification, output / "Realease", output / "Source"):
        folder.mkdir(parents=True, exist_ok=True)
    patch = output / f"Realease/{PREFIX}.patch"
    patch.write_bytes(git("diff", "--binary", baseline, commit))
    report = output / f"{PREFIX}_Report.pdf"
    shutil.copy2(ROOT / "OS1Lab_Report.pdf", report)
    expected = {name: git("show", f"{commit}:{name}") for name in
                git("ls-tree", "-r", "--name-only", commit).decode().splitlines()}

    with tempfile.TemporaryDirectory(prefix="lab01-submission-") as directory:
        source = Path(directory) / "xv6-labs-2024"
        source.mkdir()
        with tarfile.open(fileobj=io.BytesIO(git("archive", baseline))) as archive:
            archive.extractall(source, filter="data")
        run(["git", "apply", "--check", str(patch)], source,
            verification / "patch-check.txt")
        run(["git", "apply", str(patch)], source, verification / "patch-apply.txt")
        if set(files_under(source)) != set(expected):
            raise ValueError("Applied patch reconstructs a different source file list")
        if any((source / name).read_bytes() != data for name, data in expected.items()):
            raise ValueError("Applied patch does not reconstruct the tested commit exactly")
        run(["make", "-j4", "kernel/kernel", "fs.img"], source,
            verification / "patch-build.txt")
        run(["make", "check-lab01"], source, verification / "tests.txt")
        tests = (verification / "tests.txt").read_text()
        if (len(re.findall(r": OK \(", tests)) != 17
                or "Score: 50/50" not in tests or "Score: 70/70" not in tests):
            raise ValueError("Unexpected final test results")
        run(["make", "clean"], source, verification / "clean.txt")
        if set(files_under(source)) != set(expected) or any(
                (source / name).read_bytes() != data for name, data in expected.items()):
            raise ValueError("Cleaned source differs from committed source")

        # Add current reporting evidence; historical report drafts are excluded.
        current = ["README.md", "environment.txt", "run-context.json", "clean.txt",
                   "build.txt", "tests.txt", "pdf-build.txt", "pdf-compiler.log",
                   "pdf-validation.txt", "pdf-validation.json"]
        selected = [EVIDENCE / name for name in current]
        selected += sorted(EVIDENCE.glob("qemu-*.txt"))
        selected += sorted(p for folder in ("upgrade", "figures")
                           for p in (EVIDENCE / folder).rglob("*") if p.is_file())
        for path in selected:
            destination = source / path.relative_to(ROOT)
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, destination)
        for name in ("validate-report.py", "render-report-evidence.py"):
            shutil.copy2(ROOT / "tools" / name, source / "tools" / name)

        report_source = source / "docs/report-source"
        report_source.mkdir()
        for name in ("OS1Lab_Report.tex", "Logo_Truong.png"):
            shutil.copy2(ROOT / name, report_source / name)
        shutil.copytree(EVIDENCE / "figures", report_source / "docs/report-validation/figures")
        (report_source / "README.txt").write_text(
            "Editable report source, university logo and figure assets.\n"
            "Compile from this directory: tectonic OS1Lab_Report.tex\n"
            "The submitted report PDF is at the top level of the outer ZIP.\n"
            "Evidence and validation tools are under the xv6 source root.\n")
        submission_docs = source / "docs/submission"
        submission_docs.mkdir()
        for name in ("tests.txt", "patch-build.txt", "clean.txt"):
            shutil.copy2(verification / name, submission_docs / name)
        (submission_docs / "README.txt").write_text(
            f"Lab 01 submission prepared {args.date} (Asia/Bangkok).\n"
            f"Students: {PREFIX}\nBaseline: {baseline}\nCode commit: {commit}\n"
            "The patch reconstructed committed source byte for byte.\n"
            "Clean build and all 17 selected test groups passed before make clean.\n"
            "Report source and final evidence were added after cleaning code.\n"
            "Task.md and original handoff notes preserve the historical code-stage plan;\n"
            "this file records the final packaging status.\n"
            "Original report experiments were run 2026-10-15; packaging checks are newer.\n"
            "Build: make -j4 kernel/kernel fs.img\nTests: make check-lab01\n"
            "Report: cd docs/report-source && tectonic OS1Lab_Report.tex\n"
            "PDF validation from source root (requires PyMuPDF):\n"
            "python3 tools/validate-report.py /path/to/submitted_Report.pdf "
            "--tex docs/report-source/OS1Lab_Report.tex\n"
            "No upload to MOODLE has been performed.\n")
        archive_contents = {name: sha256(path) for name, path in files_under(source).items()}
        source_zip = output / "Source/xv6-labs-2024.zip"
        zip_tree(source, source_zip)

        # Verify the actual source archive, including executable permissions.
        extracted = Path(directory) / "extracted"
        extract_zip(source_zip, extracted)
        unpacked = extracted / "xv6-labs-2024"
        if archive_contents != {name: sha256(path) for name, path in files_under(unpacked).items()}:
            raise ValueError("Source ZIP contents differ from the verified source")
        run([sys.executable, "tools/validate-report.py", str(report), "--tex",
             "docs/report-source/OS1Lab_Report.tex"], unpacked,
            verification / "archived-report-validation.txt")
        run(["make", "-j4", "kernel/kernel", "fs.img"], unpacked,
            verification / "archive-build.txt")
        run(["make", "clean"], unpacked, verification / "archive-clean.txt")

    final_zip = output / f"{PREFIX}.zip"
    payloads = (report, patch, source_zip)
    with zipfile.ZipFile(final_zip, "w", zipfile.ZIP_DEFLATED) as archive:
        for path in payloads:
            archive.write(path, path.relative_to(output))
    with zipfile.ZipFile(final_zip) as archive:
        expected_names = {str(path.relative_to(output)) for path in payloads}
        if set(archive.namelist()) != expected_names or archive.testzip() is not None:
            raise ValueError("Final ZIP layout or integrity check failed")
        for path in payloads:
            if archive.read(str(path.relative_to(output))) != path.read_bytes():
                raise ValueError("Final ZIP payload differs from the verified artifact")
    manifest = {
        "prepared_at": args.date, "timezone": "Asia/Bangkok", "students": PREFIX.split("_"),
        "baseline": baseline, "commit": commit,
        "patch_matches_commit": True, "tests_passed": 17,
        "clean_before_source_zip": True, "source_archive_build_passed": True,
        "archived_report_validation_passed": True, "uploaded": False,
        "files": {str(path.relative_to(output)): {"sha256": sha256(path),
                  "bytes": path.stat().st_size} for path in (*payloads, final_zip)},
        "source_files": archive_contents,
    }
    (verification / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (output / "SHA256SUMS.txt").write_text("".join(
        f"{sha256(path)}  {path.relative_to(output)}\n" for path in (*payloads, final_zip)))
    print("Verified submission:", final_zip, flush=True)


if __name__ == "__main__":
    main()
