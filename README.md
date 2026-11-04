# OS Lab 2 — System Calls

Lab 2 starts from the completed [Lab 1 repository](https://github.com/EndlessMelody/os-lab1), commit `201dd84423049eab0c8661c62388c95b3b5710f8`. All three member branches are integrated on `main`, with their commit histories preserved: `trace()`, `sysinfo()`, and `ptree()`/`pstree`. The technical review, integration fixes, and verified results are recorded in [REVIEW.md](docs/lab02/REVIEW.md).

- [Original assignment](Lab2_OS_System_Call.docx)
- [Assignment text](docs/lab02/requirements.txt), extracted from the DOCX for easy reading
- [Lab 2 task checklist](Task.md)
- [Phân công nhóm](Lab2_PhanCong_SystemCalls.html) và [quy ước branch, syscall ID, giao diện](docs/lab02/TEAM.md)
- [Lab 2 report PDF](OS2Lab_Report.pdf) and [editable LaTeX source](OS2Lab_Report.tex), based on the Lab 1 report format
- [Fresh verification context](docs/lab02/validation/run-context.json) and [report reproduction instructions](docs/lab02/REPORT.md)
- [Historical Lab 1 plan](docs/lab01/Task.md) and [verification notes](docs/lab01/README.md)

## Build and run

Use Linux with a RISC-V GCC/binutils toolchain, QEMU for RISC-V, GNU Make, Perl, and Python 3. For Ubuntu:

```sh
sudo apt update
sudo apt install build-essential gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu qemu-system-misc gdb-multiarch perl python3
git clone https://github.com/EndlessMelody/os-lab2.git
cd os-lab2
make -j4 kernel/kernel fs.img
make qemu
```

Exit QEMU with `Ctrl-a`, then `x`. Existing Lab 1 regression checks run with:

```sh
make check-lab01
```

Run the Lab 2 suites with `make check-trace` and `make check-lab02`. The latter covers sysinfo, ptree, process relationships, invalid buffers, concurrent snapshots, pstree output, 64-bit formatting, and tracing the new syscalls. Run `./grade-lab02-kernel` for the existing xv6 quick kernel regression. `python3 tools/verify-lab02.py` repeats the full clean/build/test sequence on an isolated archive of the committed source and records results under `docs/lab02/validation/`. Test suites recreate the filesystem image and can replace hand-created test data.

`conf/lab.mk` intentionally retains `LAB=util` so the inherited programs keep their existing build settings, including two user stack pages. This setting selects MIT's scaffold; it is not the course lab number. The inherited Makefile's `LAB=syscall` mode expects MIT 2024's `attack`, `attacktest`, and `secret` programs, which are absent and outside this assignment. Do not switch modes without adapting those targets and reviewing the stack setting. `make grade` still selects the MIT utility grader; use the explicit Lab 2 commands above.

## Git workflow

`main` is the verified integration branch. The member branches `lab2-trace` (Đăng Khoa), `lab2-sysinfo` (Bảo Trân), and `lab2-ptree` (Nhựt Duy) preserve the original handoffs. For follow-up changes, branch from the current main:

```sh
git switch main
git pull --ff-only
git switch -c lab2-followup
# Implement and test the feature.
git add <changed-files>
git commit -m "Implement syscall tracing"
git push -u origin lab2-followup
```

Open pull requests into `main`. All three features touch syscall registration and user stubs, so coordinate changes to `kernel/syscall.c`, `kernel/sysproc.c`, `user/user.h`, `user/usys.pl`, and `Makefile`. The shared syscall numbers are reserved in `kernel/syscall.h`: trace 22, sysinfo 23, ptree 24. See [TEAM.md](docs/lab02/TEAM.md) for interfaces and review assignments.

The `lab2-base` tag records the prepared starting point. To inspect only subsequent Lab 2 work:

```sh
git diff --stat lab2-base HEAD
git diff --binary lab2-base HEAD > ../StudentID1_StudentID2_StudentID3.patch
```

Commit all intended changes before generating the patch; `HEAD` excludes uncommitted work. The starting source hash is also recorded in [docs/lab02/LAB1_BASELINE](docs/lab02/LAB1_BASELINE). A fresh clone has only `origin`; the local setup checkout additionally has `lab1` and `upstream` remotes for reference.

## Submission

Submit through MOODLE as `StudentID1_StudentID2_StudentID3.zip`, with IDs in ascending order. Include the matching `_Report.pdf`, the patch under `Realease/` (the assignment's spelling), and a cleaned xv6 source ZIP under `Source/`. Explain the solution, tests, difficulties, and incomplete requirements in a concise report.

Run `make clean` before packaging source. The inherited `tools/package-*.py`, report files, and `submissions/lab01/` artifacts belong to Lab 1; they have not been adapted to package Lab 2. Keep a complete source archive as well as the patch, since the Lab 2 baseline includes Lab 1 work.

The original xv6 README and license remain in [README](README) and [LICENSE](LICENSE).
