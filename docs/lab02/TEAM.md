# Lab 2 — Quy ước bàn giao

Phân công gốc: [Lab2_PhanCong_SystemCalls.html](../../Lab2_PhanCong_SystemCalls.html). Đợt này thực hiện code, self-test, review chéo và tích hợp; báo cáo, đóng gói và nộp MOODLE thực hiện sau.

| Thành viên | Nhánh | Phần phụ trách | Người review |
|---|---|---|---|
| Đăng Khoa | `lab2-trace` | `trace()`, `user/trace.c`, chuẩn bị tích hợp | Nhựt Duy |
| Bảo Trân | `lab2-sysinfo` | `sysinfo()`, `user/sysinfotest.c` | Đăng Khoa |
| Nhựt Duy | `lab2-ptree` | `ptree()`, `user/pstree.c` | Bảo Trân |

Ba nhánh bắt đầu từ cùng một commit chuẩn bị trên `main`, sau tag `lab2-base`. Giữ lịch sử commit khi merge; chưa đánh dấu review hoặc tích hợp hoàn thành khi chưa có bàn giao thực tế.

## Giao diện chung

`kernel/syscall.h` đã dành sẵn ID: `SYS_trace=22`, `SYS_sysinfo=23`, `SYS_ptree=24`. ID được dành trước không có nghĩa syscall đã có handler; mỗi thành viên thêm handler, user stub và đăng ký của phần mình.

- Trace: `int trace(int mask)`; bit `1U << SYS_xxx` chọn syscall.
- Sysinfo: `kernel/sysinfo.h` định nghĩa `struct sysinfo { uint64 freemem; uint64 nproc; };`. Trong `user/user.h`, khai báo trước `struct sysinfo;` và `int sysinfo(struct sysinfo *);`.
- Process tree: `kernel/ptree.h` định nghĩa `struct ptreeinfo { int pid; int ppid; int state; uint64 memsize; char name[16]; };`. Trong `user/user.h`, khai báo trước `struct ptreeinfo;` và `int ptree(struct ptreeinfo *, int);`.
- Include `kernel/types.h` trước các header dùng `uint64` ở chương trình user. Kernel thu thập dữ liệu, chương trình user trình bày kết quả.

Giữ `LAB=util` và môi trường build hiện tại như giải thích trong [README](../../README.md). Khi thêm chương trình, thêm vào `UPROGS` chung; không bật block MIT 2024 `LAB=syscall` dành cho bài `attack`.

## Lấy nhánh và bàn giao

```sh
git clone https://github.com/EndlessMelody/os-lab2.git
cd os-lab2
git switch lab2-sysinfo  # Bảo Trân; Nhựt Duy dùng lab2-ptree
make -j4 kernel/kernel fs.img
make qemu
```

Mỗi người push commit vào nhánh của mình, lưu lệnh test và output trong `docs/lab02/`, rồi gửi link nhánh/commit cho người review. Khi `main` có code đã review, cập nhật bằng `git fetch origin` và `git merge origin/main`; kiểm tra các file chung trước khi push.

Các file dễ conflict: `kernel/syscall.c`, `kernel/sysproc.c`, `kernel/proc.c`, `user/user.h`, `user/usys.pl`, `Makefile`. Bản trace đã chuẩn bị tên `sysinfo` và `ptree` cho output; khi thêm handler, giữ bảng tên và bảng dispatch tương ứng.

Sau khi đủ ba phần đã review, Đăng Khoa merge lần lượt vào `main`, xử lý conflict, chạy test từng syscall cùng `make check-lab01`, lưu output tích hợp và cập nhật checklist. Self-test của tác giả không thay thế review của thành viên khác.
