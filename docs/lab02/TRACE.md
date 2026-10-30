# Bàn giao trace — Đăng Khoa

Nhánh: `lab2-trace`. Baseline chung của ba thành viên: `fcbb90ef2217f0805447337e021a34e70e6320e0`. Hoàn thành implementation và self-test 5/5 theo bản phân công. Nhựt Duy chưa thực hiện review; chưa merge phần trace vào `main`.

## Cách chạy

```sh
git fetch origin
git switch lab2-trace
make clean
make -j4 kernel/kernel fs.img
make qemu
```

Trong shell xv6:

```sh
trace 32 grep hello README
trace 2147483647 grep hello README
grep hello README
trace 2 usertests forkforkfork
```

Chạy self-test tự động từ terminal Linux:

```sh
make check-trace
make check-lab01
```

`check-trace` dùng QEMU/GDB và `gradelib.py` có sẵn, cần `gdb-multiarch`. Script tạo lại `fs.img` cho mỗi nhóm; dữ liệu thử tay trên image này sẽ bị thay thế. Helper `tracecheck` chỉ được đưa vào image test, không nằm trong `UPROGS` của image chuẩn.

## Cài đặt

- `SYS_trace=22`; `sys_trace()` lấy đối số qua `argint()` và lưu vào `proc.tracemask` kiểu `uint`.
- `allocproc()` khởi tạo mask bằng 0, `freeproc()` xóa mask và `fork()` sao chép từ cha trước khi con được chạy. Mask là dữ liệu riêng của process; đổi mask ở cha không đổi mask của con hoặc các process khác.
- `exec()` giữ nguyên process nên giữ mask. Sau khi handler trả về, `syscall()` kiểm tra mask hiện tại và in một dòng `PID: syscall NAME -> RESULT`. Kết quả in dạng số có dấu 64 bit, bao gồm `-1` khi thất bại. Kiểm tra giới hạn bit trước phép dịch tránh dịch quá độ rộng mask.
- `trace(0)` tắt tracing. Nếu bit `SYS_trace` được bật sau lời gọi `trace()`, chính lời gọi này cũng được log. `exit()` không trở về dispatcher nên không có dòng giá trị trả về cho `exit`.
- CLI nhận mask thập phân trong khoảng 0–2147483647, kiểm tra ký tự và tràn số trước khi chuyển đổi. API kernel giữ toàn bộ bit của đối số `int`, kể cả khi truyền `-1`; test riêng kiểm tra bit 31 không bật nhầm syscall.
- `user/trace.c` chuyển trực tiếp `argv + 2` cho `exec()`, giữ đủ đối số và kết thúc với mã 1 khi usage, mask hoặc exec lỗi.
- Trong `Makefile`, thêm `_trace`, helper test và `check-trace`. Đồng thời bỏ `*.tex` khỏi `clean` vì đó là source báo cáo được Git theo dõi; đã kiểm tra `make clean` giữ nguyên `OS1Lab_Report.tex`.
- Sửa một lỗi tái chạy trong `grade-tv1`: tạo lại `fs.img` trước khi đưa helper `tv1check` vào. Nếu helper đã build từ lần trước nhưng image hiện tại không chứa nó, Make có thể không tạo lại image và test báo `exec tv1check failed`. Thay đổi này giúp regression chạy lại ổn định.

## Kết quả tự kiểm thử

| Nhóm | Kiểm tra | Output |
|---|---|---|
| Mask đơn | Chỉ log `read`, giữ qua `exec`, tổng byte đọc bằng kích thước README, lần cuối trả 0 | [trace-read.txt](trace-read.txt) |
| Mask toàn bộ | Đúng tên/PID, `trace -> 0`, `exec -> 3`, open/read/close đúng thứ tự | [trace-all.txt](trace-all.txt) |
| Không trace | Lệnh grep sau đó và mask 0 không phát sinh log | [trace-plain.txt](trace-plain.txt) |
| Đổi/tắt mask | getpid, close lỗi trả -1, thay mask, tắt mask, bit 31 và mask -1 qua API | [trace-masks.txt](trace-masks.txt) |
| Process | Cha → con → cháu, sibling có sẵn không bị ảnh hưởng, 68 lần fork/wait tái sử dụng slot không còn log cũ | [trace-inherit.txt](trace-inherit.txt) |
| CLI | Thiếu đối số, 7 mask lỗi, exec lỗi, mã thoát, mask 0/0000, chuyển tiếp nhiều đối số | [trace-cli.txt](trace-cli.txt) |
| forkforkfork | Test thật của MIT chạy thành công, fork được log ở nhiều thế hệ | [trace-forkforkfork.txt](trace-forkforkfork.txt) |

Cả 7 nhóm self-test đạt, tổng 70/70 của bộ test nội bộ; đây không phải điểm giảng viên. Regression Lab 1 cũng đạt: 5 test MIT tương ứng sleep/pingpong/primes, TV1 50/50 và Lab01 70/70. Xem [log self-test](trace-selftest.txt), [build sạch](trace-build.txt), [clean](trace-clean.txt) và [regression Lab 1](trace-lab01-regression.txt). Môi trường kiểm tra: GCC RISC-V 13.3.0, QEMU 8.2.2, 3 CPU ảo. PID và số lần fork có thể thay đổi giữa các lần chạy; fork trả `-1` khi hết slot trong bài stress là hợp lệ.

Chưa có kết quả `sysinfo`, `ptree`, review chéo hoặc tích hợp cả ba phần. Bảo Trân và Nhựt Duy làm việc trên `lab2-sysinfo` và `lab2-ptree` từ baseline chung; quy ước giao diện nằm trong [TEAM.md](TEAM.md). Đăng Khoa review sysinfo sau bàn giao và tích hợp khi đủ phần đã review. Không tạo báo cáo hoặc gói nộp trong đợt này.
