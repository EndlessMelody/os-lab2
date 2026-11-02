# Lab 01 — Bản tích hợp để chuyển sang viết báo cáo

Nhánh tích hợp: `util`. Ngày kiểm tra: 12/10/2026.
Phạm vi nghiệm thu code là 7 chương trình trong `Task.md`:
`sleep`, `pingpong`, `primes`, `cp`, `diff`, `tree`, `du`.

## Source đã tích hợp

| Thành viên | Nhánh | Commit bàn giao | Chương trình |
|---|---|---|---|
| Đăng Khoa | `lab01-process` | `a577dd503a839b68c71699704b6280baeb60052d` | sleep, pingpong, primes |
| Bảo Trân | `lab01-files` | `bf5cc3aba87b7082a1d72349b9968148c3afea38` | cp, diff |
| Nhựt Duy | `lab01-directories` | `6d9d56abc57b082cc8d712f11e4db0798610813b` | tree, du |

Baseline chung: `79e8024d61c3dd92ccf064fc67bb99cb999f70ab`.
Hai nhánh TV2/TV3 được merge với lịch sử commit riêng, không chép đè
hoặc thay tác giả các phần bàn giao. Cả 7 entry có trong `UPROGS`.
Không sửa kernel hoặc script MIT `grade-lab-util`.

## Lỗi phát hiện và sửa khi tích hợp

- Bản đầu `tree` và `du` báo lỗi path không tồn tại nhưng vẫn `exit(0)`.
  Đã tái hiện bằng test chạy trong xv6; xem `before-fixes.txt`.
  Bản sửa truyền lỗi duyệt cây về `main` và trả mã 1.
- `dirent.name` của xv6 chỉ có 14 byte, có thể không có ký tự kết thúc.
  Đã chép sang buffer 15 byte và thêm NUL trước khi so sánh/in/nối path.
- Bổ sung kiểm tra `MAXPATH` cả ở đối số gốc và khi nối từng entry;
  phát hiện lỗi `stat`, đọc thư mục và thiếu FD, đóng FD đầy đủ.
- `tree -L` kiểm tra chuỗi số, số âm và tràn số; thêm kiểm tra nhiều path.
  `-L 0` chỉ in gốc; `-L 1` chỉ thêm con trực tiếp.
- `du` cộng bằng `uint64` và in bằng chuyển đổi thập phân 64 bit.
  `printf` có sẵn trong source này vẫn ép `%lu` về 32 bit nên chỉ đổi
  format từ `%d` sang `%lu` không sửa được lỗi. Test gọi formatter thật
  với 0, 4294967296 và 18446744073709551615.
- Khi duyệt lỗi, `du -s` không in một tổng thiếu như kết quả thành công.
- Giữ thuật toán của `cp`/`diff`; thống nhất tiền tố lỗi usage và chuẩn
  hóa newline/indentation ở chỗ cần thiết.

## Lệnh kiểm tra lại

Tại terminal Linux ở thư mục repo:

```sh
make clean
make -j4 kernel/kernel fs.img
make check-lab01
```

`make check-lab01` chạy lần lượt:

```sh
./grade-lab-util sleep pingpong primes
./grade-tv1
./grade-lab01
```

Script custom dùng `gradelib.py` của repo và helper C chạy thật trong
xv6 để tạo dữ liệu, bắt stdout/stderr, chờ tiến trình và kiểm tra mã thoát.
`grade-lab01` tạo lại `fs.img` cho từng nhóm test; cần lưu dữ liệu thử tay
trước khi chạy. Các helper không nằm trong `UPROGS` của filesystem chuẩn.

## Ma trận kiểm thử cho báo cáo

| Nhóm | Nội dung | Kết quả |
|---|---|---|
| Build sạch | Kernel, filesystem và đủ 7 chương trình; `-Wall -Werror` | Đạt |
| MIT tương ứng phần process | 5 test sleep/pingpong/primes, có breakpoint `sys_sleep` | Đạt |
| TV1 | 5 nhóm, 50/50 điểm custom; argv, mã thoát, `uptime`, pipeline | Đạt |
| pingpong | 50 lần, 2 PID khác nhau, ping trước pong, trở lại shell | Đạt |
| primes | 20 lần, so sánh đủ 59 số bằng phép chia thử độc lập, có 277 | Đạt |
| cp | 13 case: rỗng, nhỏ, nhị phân 8193 byte, overwrite, same inode, lỗi argv/path | Đạt |
| diff | 23 case: bằng/khác, EOF hai phía, 2 kiểu `-q`, newline, dòng 4098 byte, NUL, lỗi | Đạt |
| tree | 21 case: mặc định, cây nhiều cấp, -L 0/1/2, -d, tên 14 byte, path/FD/argv | Đạt |
| du | 17 case: tổng bytes, -a/-s, rỗng, file gốc, tên 14 byte, path/FD/argv | Đạt |
| Thiếu FD | 12 case: mở nguồn/đích, pipe thứ nhất/thứ hai, lỗi tầng primes | Đạt |
| Formatter du | 3 giá trị biên, gồm 2^32 và UINT64_MAX | Đạt |
| Luồng chung | echo → cp → diff → tree/du, sleep, pingpong, primes qua wc | Đạt |

`grade-lab01` có 7 nhóm, tổng 70/70 điểm custom. Các nhóm native gồm
86 case; test formatter và luồng shell được chấm riêng. Đây là điểm của
bộ test nội bộ, không phải điểm chính thức của giảng viên.

Log cuối cùng nằm trong `all-tests.txt`; log build/clean trong `build.txt`,
`clean.txt`. Các file `*-output.txt` lưu output QEMU theo nhóm. Không dùng
`make grade` toàn bộ MIT để kết luận bài này: `find`, `xargs`, `time` nằm
ngoài 7 chương trình được phân công và chưa được triển khai trong Lab 01
của nhóm. Kiểm tra tích hợp bằng công cụ không thay thế việc các thành
viên tự đọc và xác nhận phần kiểm thử chéo.

## Quy ước và giới hạn cần ghi trong báo cáo

- `diff` so sánh các dòng cùng vị trí, không tìm phép chèn/xóa tối ưu như
  GNU diff; in `< tên-file:số-dòng:` / `> tên-file:số-dòng:`.
  Dòng cuối thiếu newline có thông báo riêng. Mã thoát: 0 bằng, 1 khác,
  2 lỗi. Dòng dài được tăng buffer động, lỗi cấp phát được báo rõ.
- `tree` dùng `|--` và `|   `, theo thứ tự entry trên đĩa, không sắp xếp.
  Chỉ nhận một path; option có thể đứng trước hoặc sau path.
- `du` tính tổng `st.size` của file, không cộng metadata thư mục; thư mục
  rỗng bằng 0, `-s` ưu tiên `-a`. Mặc định chỉ in thư mục; nếu path gốc
  là file thì dùng `-a` hoặc `-s` để in. Hard link được tính theo từng
  entry, không khử trùng inode. Device được tính 0 byte.
- Path phải ngắn hơn `MAXPATH` (128 byte gồm NUL). Duyệt đệ quy giữ một
  FD cho mỗi thư mục tổ tiên; cây quá sâu có thể chạm `NOFILE=16`.
  Lúc đó chương trình báo lỗi và trả 1. Test cây sâu 20 cấp kiểm tra
  thất bại có kiểm soát; không tuyên bố hỗ trợ thành công độ sâu này.
- `tree` và `du` có thể đã in các nhánh duyệt được trước khi gặp lỗi.
  `du -s` không in tổng khi có bất kỳ lỗi duyệt nào.
- `primes` giữ phạm vi 2–280 do giới hạn tài nguyên process/FD của xv6.
- Đã kiểm thử cạn FD và một số lỗi pipe; chưa cưỡng bức mọi lỗi `fork`,
  cấp phát bộ nhớ hoặc ổ đĩa đầy. Không tuyên bố bộ test là vét cạn.

## Ghi chú giải pháp để viết báo cáo

Phần `sleep`, `pingpong`, `primes`: dùng nội dung giải thích syscall và
quản lý pipe/process trong `docs/tv1/README.md` (tài liệu bàn giao TV1
trước tích hợp).

`cp` mở nguồn read-only, kiểm tra file thường và so inode/dev với đích
trước truncate. Đích được tạo hoặc truncate; dữ liệu được đọc theo buffer
1024 byte. Vòng lặp ghi tiếp đến khi ghi đủ số byte đã đọc. Nhánh lỗi và
thành công đều đóng các FD đã mở.

`diff` đọc mỗi file theo từng dòng, giữ cả newline và chiều dài. Buffer
được tăng gấp đôi khi cần; so sánh theo byte nên xử lý cả NUL trong dòng.
Hai file được đọc đến EOF; chỉ in phía còn dữ liệu khi file kia kết thúc
trước. `-q` dừng ở khác biệt đầu tiên và chỉ in một thông báo.

`tree` đọc entry thư mục bằng `open`/`read`, bỏ `.` và `..`, dùng thông
tin stat để lọc và quyết định đệ quy. Cấp hiện tại quyết định thụt lề và
giới hạn `-L`; `-d` bỏ file khỏi output. Mã lỗi từ nhánh con được truyền
lên gốc.

`du` duyệt hậu thứ tự: tính từng cây con, cộng tổng rồi in thư mục cha.
Chỉ cộng kích thước file thường. Dùng biến `uint64` để chứa tổng; bộ in
thập phân riêng giữ đủ 64 bit. Tham số trả về phân biệt tổng dung lượng
với tình trạng lỗi, tránh coi một lần mở thất bại là thư mục 0 byte.

## Bàn giao sang giai đoạn báo cáo

Code đã tích hợp và có kiểm thử tái chạy bằng một lệnh. Sau khi commit,
tạo lại patch và source sạch bằng:

```sh
python3 tools/package-lab01.py
```

Script áp patch vào baseline sạch, build, `make clean`, so sánh từng file
với commit rồi tạo `submissions/lab01/Lab01-code-handoff.zip`.
Đây là gói kỹ thuật để viết báo cáo, chưa phải gói nộp MOODLE cuối cùng.

Việc còn lại: điền MSSV, hoàn thiện báo cáo PDF có bảng đóng góp, giải
pháp, ảnh/log minh chứng, khó khăn và giới hạn; cả nhóm xác nhận nội dung;
đặt tên PDF/ZIP theo MSSV tăng dần và kiểm tra cấu trúc đề gốc.
`CQ_Lab01.docx` chưa có trong repo nên chưa đối chiếu các yêu cầu nằm
ngoài `Task.md`.
