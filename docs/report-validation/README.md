# Bằng chứng cho báo cáo Lab 01

Ngày kiểm tra: **15/10/2026**, múi giờ Asia/Bangkok.
Source: commit `35ba531d8f28cb6400182b408e0676299386fcdb`, nhánh `util`.

## Kết quả chạy thật

- `clean.txt`, `build.txt`: build sạch trên bản xuất `git archive HEAD` trong thư mục tạm.
- `tests.txt`: `make check-lab01`; 5 test MIT, TV1 50/50, tích hợp 70/70.
- `qemu-*.txt`: output QEMU; tên log đã đổi từ `xv6.out*` để không bị `.gitignore` loại bỏ.
- `environment.txt`, `run-context.json`: cấu hình quan sát được và mã thoát các lệnh.
- `pdf-build.txt`, `pdf-validation.txt`, `pdf-validation.json`: biên dịch và kiểm tra PDF.

Các điểm 50/50 và 70/70 là thang của script nội bộ, không phải điểm môn học.
Không sửa source/kernel hoặc bộ chấm trong lần viết báo cáo này.

## Kiểm tra lại an toàn

**Không chạy `make clean` tại thư mục chứa báo cáo:** target hiện tại xóa `*.tex`.
Để tái chạy test, xuất commit sang một thư mục tạm mới, rồi chạy trong bản sao:

```sh
make clean
make -j4 kernel/kernel fs.img
make check-lab01
```

Báo cáo đã chỉnh theo phong cách học thuật: A4, 12 pt, lề trái/phải 2.3 cm,
lề trên 2.4 cm, giãn dòng 1.1; TeX Gyre Termes cho nội dung, Heros cho
header và Cursor cho mã lệnh. XeLaTeX/Tectonic dùng font mở của bộ TeX,
nhúng font vào PDF; pdfLaTeX dùng `vntex` và font Times qua `mathptmx`.
Lần kiểm tra tại máy này dùng Tectonic 0.17.0 portable trong `/tmp`,
không cài gói hệ thống. Các font TeX Gyre được bộ biên dịch cung cấp,
không phụ thuộc font DejaVu của máy chủ.

Bản PDF có **12 trang tổng cộng**: 1 bìa, 1 mục lục, **8 trang nội dung**,
1 kết luận và 1 tài liệu tham khảo. Các phần ngoài nội dung được tách trang
theo xác nhận của người dùng. Bản mới bổ sung tóm tắt, từ khóa, ba câu hỏi
đánh giá và thảo luận kết quả theo từng câu hỏi; trích dẫn dùng `\cite`
và `thebibliography`, bảng có caption phía trên, header và footer thống nhất.

Các bản nháp và khung báo cáo cũ đã được chuyển vào bản sao lưu ngoài
workspace khi chốt bản nộp; thư mục này chỉ giữ báo cáo và bằng chứng hiện hành.

```sh
tectonic --keep-logs --outdir /tmp/lab01-report-pdf OS1Lab_Report.tex
python3 tools/validate-report.py /tmp/lab01-report-pdf/OS1Lab_Report.pdf
```

Script kiểm tra PDF cần `pymupdf`; kiểm tra A4, số trang nội dung, lề chữ,
font được nhúng, tóm tắt/từ khóa/câu hỏi nghiên cứu, trích dẫn, tham chiếu,
mục bắt buộc và đối chiếu kết quả với log thật. Các ô chờ xác
nhận trên bìa được liệt kê riêng, không tự điền dữ liệu cá nhân.

## Thông tin nhóm và trạng thái hoàn thiện

- Dương Nhựt Duy – 24120293: họ tên và MSSV được người dùng xác nhận ngày 16/10/2026;
  đã đồng bộ trên bìa, dòng tác giả, bảng đóng góp và metadata PDF.
- Bìa đã cập nhật giảng viên ThS. Lê Viết Long, ThS. Phạm Tuấn Sơn; lớp CQ2024/22;
  Phan Lê Đăng Khoa – 24120347; Âu Bảo Trân – 24120468; Dương Nhựt Duy – 24120293.
  Bỏ ngày nộp theo yêu cầu.
- Bảng đóng góp dựa trên commit bàn giao và phân công trong khung;
  nhóm cần xác nhận trước khi dùng như bản đóng góp cuối cùng.
- Tên PDF/ZIP theo ba MSSV tăng dần. Gói nộp cuối và hồ sơ kiểm tra nằm tại
  `submissions/lab01/final/`; trạng thái kiểm chứng được lưu trong
  `verification/manifest.json`. Chưa nộp MOODLE.

Đề `CQ_Lab01.docx` yêu cầu PDF, patch trong `Realease` (giữ cách viết của đề),
source ZIP đã clean trong `Source`; báo cáo súc tích, giải thích giải pháp,
khó khăn và yêu cầu chưa hoàn tất. Đề không đặt giới hạn trang cụ thể.

## Cập nhật sau lượt review ngày 15/10/2026

Các cải tiến đã áp dụng sau lượt review:

- Thống nhất thuật ngữ tiếng Việt trong văn xuôi, giải thích EOF/FD/NUL/PID,
  viết lại tóm tắt và kết luận, rút gọn nội dung lặp giữa thiết kế và thảo luận.
- Đổi tên phần câu hỏi thành “Câu hỏi đánh giá và tiêu chí kiểm chứng”,
  giữ ba mã RQ để đối chiếu với phần thảo luận.
- Bổ sung ví dụ chạy thật của `diff` khi một tệp kết thúc trước; mô tả
  khách quan khác biệt với định dạng minh họa trong đề.
- Bổ sung **hai hình vector** từ nhật ký kiểm thử. Chữ trong hình có thể
  chọn/sao chép và được nhúng bằng TeX Gyre Cursor 9.5 pt. Đây là hình
  trình bày log, **không phải ảnh chụp cửa sổ terminal**; caption và hồ sơ
  nguồn đều ghi rõ cách dựng.
- Biên dịch lại PDF bằng Tectonic 0.17.0; vẫn **12 trang tổng, 8 trang
  nội dung chính**. Lần biên dịch cuối không có cảnh báo; kiểm tra PDF đạt.

Bản PDF hiện tại được lưu tại gốc repo và trong gói nộp cuối. Bản nháp
trước khi nâng cấp nằm trong bản sao lưu dọn workspace, ngoài repo.

### Kết quả chạy lại

Thư mục `upgrade/` chứa một lần kiểm thử mới trên bản xuất sạch của cùng
commit `35ba531`, tách khỏi workspace chứa báo cáo:

- `clean.txt`, `build.txt`, `tests.txt`: cả ba lệnh kết thúc với mã 0;
  17 nhóm kiểm thử đạt, gồm 5 kiểm thử MIT, TV1 50/50 và tích hợp 70/70.
- `qemu.*.txt`: log QEMU của từng nhóm kiểm thử, sao chép từ bản tạm.
- `diff-example.txt`: phiên QEMU bổ sung với `f1` chứa `one` và `f2`
  chứa `one`, `two` trên hai dòng; `diff f1 f2` in `> f2:2: two`.
- `run-context.json`: commit, lệnh chạy và mã thoát của lần kiểm tra mới.

Các log kiểm chứng trước đó ở thư mục cha vẫn được giữ nguyên. Những tệp
`pdf-build.txt`, `pdf-compiler.log`, `pdf-validation.txt`, `pdf-validation.json`
và phần `report` trong `run-context.json` mô tả PDF hiện tại.

### Nguồn và cách dựng hình

`tools/render-report-evidence.py` đọc trực tiếp log trong `upgrade/`:

- `figures/test-summary.pdf`: 10 dòng được lọc, gồm lệnh gọi ba bộ,
  hai điểm tổng kết và năm số lượng ca chức năng/xử lý lỗi.
- `figures/workflow.pdf`: trích đoạn liên tục từ `$ mkdir demo` đến
  kết quả `59 118 561` của `primes | wc`.
- Các bản `.png` dùng để xem nhanh; report nhúng bản `.pdf` để chữ rõ
  khi phóng to hoặc in. Tệp `.txt` giữ nguyên nội dung trích xuất,
  kể cả ký tự tab và khoảng trắng cuối dòng.
- `figures/manifest.json` lưu số dòng nguồn, cách chọn, SHA-256 của log,
  nội dung trích và PDF minh chứng.

Để dựng lại hình, cần PyMuPDF và phông TeX Gyre Cursor. Script tự tìm
phông trong bộ đệm Tectonic, hoặc nhận `--font /duong/dan/toi/phong.otf`:

```sh
python3 tools/render-report-evidence.py
tectonic --keep-logs --outdir /tmp/lab01-report-pdf OS1Lab_Report.tex
python3 tools/validate-report.py /tmp/lab01-report-pdf/OS1Lab_Report.pdf
```

Khi biên dịch một bản mới, cập nhật `pdf-build.txt` và hash trong
`run-context.json` trước khi kiểm tra; validator đối chiếu cả nội dung hình
với log và hash của TEX/PDF. Các hình đã được lưu sẵn nên không cần
dựng lại chỉ để biên dịch TEX hiện tại.

Họ tên và MSSV của cả ba thành viên đã đầy đủ. Nhóm cần đối chiếu lại bảng
đóng góp trước khi nộp. Thứ tự MSSV tăng dần để đặt
tên tệp nộp: `24120293_24120347_24120468`.

## Rà soát và biên dịch lại ngày 16/10/2026

- Đã cập nhật họ tên Dương Nhựt Duy và MSSV 24120293 theo xác nhận của
  người dùng; metadata PDF dùng đầy đủ họ tên có dấu của cả ba tác giả.
- Báo cáo không còn thông tin cá nhân bỏ trống hoặc chỉ dẫn viết nháp.
  Đã trau chuốt thêm một số câu ở phần giới thiệu, thảo luận và khó khăn.
- Hai hình kiểm thử đủ cho mục đích minh chứng: tổng kết bộ kiểm thử và
  luồng thực thi tích hợp; có chú thích, tham chiếu và log nguồn.
- Người dùng đã bổ sung `Logo_Truong.png`; TEX dùng đúng tên phân biệt
  chữ hoa/thường. PDF được biên dịch lại với logo trên bìa; validator
  kiểm tra hình đã được nhúng.
- Bìa dùng hai cột thông tin: lớp bên trái, giảng viên bên phải; nội dung
  đặt dưới từng nhãn và căn theo chiều rộng bảng thành viên. Đã giữ học vị
  `ThS.` do người dùng cập nhật, biên dịch và kiểm tra lại PDF 12 trang;
  nội dung từ mục lục trở đi không thay đổi.
- Lượt này biên dịch và kiểm tra lại báo cáo; bằng chứng kiểm thử xv6 vẫn
  thuộc các lần chạy ngày 15/10/2026, không được đổi thành ngày chạy mới.

## Chuẩn bị gói nộp ngày 16/10/2026

`tools/package-submission.py --date 2026-10-16` tạo gói cuối từ mã nguồn
đã kiểm thử tại commit ghi trong `run-context.json`, cùng báo cáo hiện tại
đã được kiểm tra. Commit bổ sung báo cáo không làm thay đổi mốc mã nguồn
dùng để sinh patch. Script cho phép các tệp
báo cáo chưa commit được chọn rõ ràng, đồng thời từ chối thay đổi chưa
commit trong mã nguồn đã theo dõi hoặc tệp runtime mới chưa được theo dõi.
Không cần commit báo cáo để đóng gói và không chạy `make clean` trong
workspace đang chứa báo cáo.

Cấu trúc ZIP ngoài gồm đúng ba tệp:

```text
24120293_24120347_24120468.zip
  24120293_24120347_24120468_Report.pdf
  Realease/24120293_24120347_24120468.patch
  Source/xv6-labs-2024.zip
```

Patch được áp lên baseline sạch, đối chiếu từng byte với commit, biên dịch
và chạy 17 nhóm kiểm thử trước khi clean mã nguồn. Source ZIP được giải
nén lại để kiểm tra nội dung, quyền thực thi, biên dịch và xác minh báo cáo
bằng các công cụ/bằng chứng nằm trong chính gói source. Nhật ký kiểm chứng
lần đóng gói nằm ngoài ZIP nộp tại `submissions/lab01/final/verification/`;
nhật ký kiểm thử chính cũng có trong `docs/submission/` của source ZIP.

Source ZIP giữ mã nguồn đã clean, bổ sung bằng chứng báo cáo hiện tại và
bản TEX/logo/hình có thể biên dịch tại `docs/report-source/`. Bản nháp cũ
và lịch sử review báo cáo không được đưa vào gói nộp. Các ghi chú bàn giao
đã commit được giữ như hồ sơ lịch sử; trạng thái đóng gói cuối được ghi
riêng ở `docs/submission/README.txt`.
