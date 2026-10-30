# Lab 2 — System Call

Source of requirements: [Lab2_OS_System_Call.docx](Lab2_OS_System_Call.docx). This checklist describes planned work; setup does not complete the exercises. The previous Lab 1 plan is preserved in [docs/lab01/Task.md](docs/lab01/Task.md).

## 1. Trace

- [ ] Add `int trace(int mask)` to the user interface, syscall number, stub, and kernel dispatch.
- [ ] Store a trace mask per process, initialize it for new processes, and copy it on `fork`.
- [ ] After a selected syscall returns, print PID, syscall name, and return value.
- [ ] Keep tracing enabled across `exec` and inherited by descendants; other processes must remain unaffected.
- [ ] Write `user/trace.c` and add `$U/_trace` to `UPROGS`.
- [ ] Check `trace 32 grep hello README`, `trace 2147483647 grep hello README`, ordinary `grep hello README`, and `trace 2 usertests forkforkfork`.
- [ ] Test invalid usage, disabling the mask, and process-slot reuse.

## 2. Sysinfo

- [ ] Define shared `struct sysinfo` with `freemem` (free bytes) and `nproc` (processes whose state is not `UNUSED`).
- [ ] Add `int sysinfo(struct sysinfo *)`, its syscall number, stub, and kernel dispatch.
- [ ] Count the allocator's free pages and active processes using the appropriate locks.
- [ ] Copy the result into user memory with `copyout`; report invalid pointers as errors.
- [ ] Write `user/sysinfotest.c`, add `$U/_sysinfotest` to `UPROGS`, and require `sysinfotest: OK` on success.
- [ ] Test memory allocation/free, process creation/exit, and invalid user addresses.

## 3. Process tree

- [ ] Define shared `struct ptreeinfo` with `pid`, `ppid`, `state`, `uint64 memsize`, and `char name[16]`.
- [ ] Add `int ptree(struct ptreeinfo *buf, int max)`, its syscall number, stub, and dispatch.
- [ ] Collect up to `max` non-`UNUSED` processes in the kernel, respecting process and parent-pointer locking.
- [ ] Return the number of entries copied, or `-1` on invalid arguments or copy errors.
- [ ] Handle `max <= 0`, invalid buffers, and more processes than available entries without overflowing the user buffer.
- [ ] Write `user/pstree.c`, add `$U/_pstree` to `UPROGS`, and construct the indented parent/child display in user space.
- [ ] Display PID, name, state, and user-memory size; test multiple generations and a truncated snapshot.

## 4. Integration and hand-in

- [ ] Coordinate distinct syscall numbers and shared-file edits across the three features.
- [ ] Build from clean source and verify all three features together in QEMU.
- [ ] Re-run `make check-lab01` to check inherited utilities.
- [ ] Record reproducible test commands, actual results, limitations, and each member's contribution.
- [ ] Write a concise PDF report named `StudentID1_StudentID2_StudentID3_Report.pdf`.
- [ ] Generate a patch from the agreed baseline, including all newly committed source files, and verify it applies to that baseline.
- [ ] Run `make clean`, create the xv6 source ZIP, and verify the final archive contains the PDF, `Realease/` patch, and `Source/` ZIP.
- [ ] Sort student IDs ascending in the final ZIP name and submit through MOODLE.
