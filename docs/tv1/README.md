# Bàn giao TV1 — Đăng Khoa

Phạm vi: `sleep`, `pingpong`, `primes` và chuẩn bị tích hợp. `cp`, `diff`
thuộc TV2; `tree`, `du` thuộc TV3. Đây là bản bàn giao TV1, chưa phải gói
nộp cuối của nhóm. Ngày kiểm thử: 12/10/2026.

## Source và môi trường

- Nhánh làm việc: `lab01-process`.
- Baseline `util`: `79e8024d61c3dd92ccf064fc67bb99cb999f70ab`
  (lưu trong `BASELINE`).
- Commit bàn giao: xem `git log -1 --oneline` hoặc `HANDOFF.txt` trong gói ZIP.
- Công cụ đã dùng: `riscv64-linux-gnu-gcc`, `qemu-system-riscv64`, Python 3.12.
- Source chương trình: `user/sleep.c`, `user/pingpong.c`, `user/primes.c`.
- Entry đã thêm vào `UPROGS`: `$U/_sleep`, `$U/_pingpong`, `$U/_primes`.
- Giữ nguyên kernel và script chấm MIT `grade-lab-util`.

## Nội dung giải pháp để đưa vào báo cáo

### sleep

Chương trình nhận đúng một đối số là số ticks. Trước khi gọi `atoi`, chương
trình kiểm tra chuỗi chỉ chứa chữ số và giá trị không vượt giới hạn của số
nguyên có dấu 32 bit. Sau đó gọi system call `sleep(ticks)` và `exit(0)` khi
thành công. Thiếu/thừa đối số, số âm, chuỗi rỗng, ký tự không hợp lệ hoặc
số vượt 2147483647 đều được báo trên stderr và kết thúc với mã 1.

Ticks là đơn vị đồng hồ của kernel, không phải số giây. `sleep 0` không yêu
cầu chờ một tick mới. Kiểm thử dùng `uptime()` để đo số ticks khi chạy
`sleep 10`, đồng thời test MIT kiểm tra chương trình đi vào `sys_sleep`.

### pingpong

Hai pipe tạo hai chiều liên lạc: `tochild` đưa dữ liệu từ cha đến con,
`toparent` đưa dữ liệu từ con về cha. Sau `fork`, mỗi tiến trình đóng hai
đầu pipe không dùng. Cha gửi một byte; con đọc byte đó, in PID của chính
nó cùng `received ping`, rồi gửi lại byte. Cha nhận byte trả về và in PID
của mình cùng `received pong`.

Con in trước khi gửi byte trả về nên dòng ping xuất hiện trước dòng pong.
PID được lấy bằng `getpid()`. Các đầu pipe được đóng sau khi sử dụng;
cha gọi `wait` và kiểm tra mã thoát của con. Các lỗi `pipe`, `fork`, `read`
và `write` đều có nhánh báo lỗi và giải phóng tài nguyên đã tạo.

### primes

Tiến trình phát ghi lần lượt các số từ 2 đến 280 qua pipe bằng
`write(..., sizeof(int))`; trên xv6 này, mỗi số nguyên chiếm 4 byte.
Tiến trình lọc đọc số đầu tiên làm số nguyên tố `p` và in `prime p`.
Những số chia hết cho `p` bị loại; những số còn lại được gửi đến tầng sau.

Chỉ khi tìm được một số không chia hết cho `p`, tầng hiện tại mới tạo
pipe và tiến trình con. Con trở thành tầng lọc kế tiếp bằng cách đổi FD
đầu vào rồi tiếp tục vòng lặp. Cách này vẫn tạo một tiến trình cho mỗi số
nguyên tố, đồng thời không làm tăng độ sâu ngăn xếp như gọi hàm đệ quy.
Cha gửi số đã tìm được và lọc tiếp phần dữ liệu còn lại.

Sau khi đọc hết đầu vào, mỗi tầng đóng FD đọc và FD ghi sang tầng sau,
rồi mới `wait` con. Việc đóng đầu ghi cho phép tầng sau nhận EOF;
chuỗi `wait` bảo đảm tiến trình phát chỉ kết thúc sau toàn bộ pipeline.
Lỗi của tầng con được truyền ngược bằng mã thoát. Kết quả có 59 số nguyên
tố, bắt đầu bằng 2 và kết thúc bằng 277.

## Kiểm thử thực tế

Chạy từ terminal Linux tại thư mục repo:

```sh
make -j4 kernel/kernel fs.img
./grade-lab-util sleep pingpong primes
./grade-tv1
make qemu
```

Trong shell xv6 có thể thử `sleep 10`, `sleep 0`, `sleep`, `pingpong`,
`primes`. Thoát QEMU bằng Ctrl-a rồi x.

| Kiểm thử | Kết quả |
|---|---|
| Build với `-Wall -Werror` | Đạt |
| 5 test MIT tương ứng `sleep`, `pingpong`, `primes` | Đạt |
| `sleep`: thiếu/thừa đối số, số âm, chữ, số vượt giới hạn | Đạt |
| `sleep 0`, `sleep 10`: trả lại shell | Đạt |
| `pingpong` 50 lần: đúng 2 dòng/lần, PID khác nhau, ping trước pong | Đạt |
| `primes` 20 lần: so sánh toàn bộ kết quả bằng phép chia thử độc lập | Đạt |
| `primes` không thiếu/trùng số, có cả 277, trả lại shell | Đạt |
| `primes \| wc`, `pingpong \| cat` | Đạt |
| Mã thoát và số ticks của `sleep 10` qua helper chạy trong xv6 | Đạt, đo được 10 ticks |

`mit-tests.txt` và `functional-tests.txt` lưu kết quả chạy thật.
`status-output.txt` lưu output helper kiểm tra `wait(&status)` và
`uptime()`. Helper nằm ở `tests/tv1check.c`; `grade-tv1` tự build và đưa
nó vào filesystem kiểm thử, không thêm nó vào `UPROGS` của bản nộp.

Không chạy toàn bộ `make grade` để đánh giá bài môn học này: script MIT
còn chấm `find`, `xargs` và `time`, không thuộc phần phân công TV1.
Các test bổ sung không thay thế kiểm thử chéo của TV3.

## Tạo và kiểm tra gói bàn giao

Sau khi commit source và tài liệu, chạy:

```sh
python3 tools/package-tv1.py
```

Script tạo patch so với baseline bằng `git diff BASELINE HEAD`, nên có
cả file mới đã commit. Nó áp patch lên bản baseline sạch, build kernel
và filesystem, chạy `make clean` trên bản đó rồi mới tạo ZIP source.
Thư mục làm việc chính được giữ để tiếp tục phát triển.

Kết quả ở `submissions/tv1/`: `TV1.patch`, `xv6-labs-2024.zip`,
`verify-build.txt`, `HANDOFF.txt`, `TV1-handoff.zip`. `submissions/`
được repo bỏ qua trong Git. Đây là tên gói bàn giao nội bộ.

## Phần còn lại khi tích hợp cả nhóm

1. Nhận source và commit bàn giao của TV2/TV3, thêm `$U/_cp`, `$U/_diff`,
   `$U/_tree`, `$U/_du` vào `UPROGS` khi có đủ source. Hiện chưa thêm
   các entry này vì chúng chưa tồn tại và sẽ làm build thất bại.
2. TV1 kiểm thử chéo `cp`/`diff`; TV3 kiểm thử chéo ba chương trình TV1.
3. Build và chạy lại đủ 7 chương trình trên bản chung; gửi nội dung giải
   pháp và log trong thư mục này cho TV2 để ghép báo cáo PDF.
4. Tạo lại patch và source ZIP từ commit đã tích hợp, áp patch và build
   lại trên baseline sạch. Gói TV1 hiện tại chỉ chứa phần TV1.
5. Điền ba MSSV theo thứ tự tăng dần để đặt tên gói cuối và report.
   Dùng cấu trúc `Realease/`, `Source/` theo `Task.md`, kiểm tra lại đề
   `CQ_Lab01.docx` và hướng dẫn MOODLE trước khi nộp.

Giới hạn: phạm vi sàng cố định 2–280; tài nguyên process/FD của xv6 có
hạn nên không tự ý tăng giới hạn này. Các nhánh lỗi syscall đã được
xử lý trong code; chưa kiểm thử cưỡng bức mọi trường hợp cạn tài nguyên.
Chưa có đề DOCX gốc, MSSV, source TV2/TV3, PDF chung hoặc kết quả review
của thành viên khác để nghiệm thu gói nộp cuối.
