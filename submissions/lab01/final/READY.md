# Gói nộp Lab 01

Ngày chuẩn bị: 16/10/2026, múi giờ Asia/Bangkok.

Tệp dùng để nộp MOODLE: **`24120293_24120347_24120468.zip`**.

```text
24120293_24120347_24120468.zip
  24120293_24120347_24120468_Report.pdf
  Realease/
    24120293_24120347_24120468.patch
  Source/
    xv6-labs-2024.zip
```

Giữ tên `Realease` theo đề gốc. ZIP ngoài chứa đúng ba tệp; thư mục
`verification/`, `READY.md` và `SHA256SUMS.txt` dùng để kiểm tra tại máy,
không được đưa thêm vào ZIP nộp.

Thành viên:

- Dương Nhựt Duy — 24120293.
- Phan Lê Đăng Khoa — 24120347.
- Âu Bảo Trân — 24120468.

Đã kiểm tra:

- Báo cáo có logo trường, đủ họ tên/MSSV, hai hình minh chứng và bảng đóng góp;
  12 trang tổng cộng, gồm 8 trang nội dung chính. Không còn chỗ trống cần điền.
- PDF trong gói đã cập nhật học vị ThS. của Lê Viết Long và Phạm Tuấn Sơn;
  thông tin lớp và giảng viên được trình bày thành hai cột trên bìa.
- PDF biên dịch không có cảnh báo và đạt kiểm tra bố cục, tham chiếu, font,
  thông tin tác giả, logo và nguồn dữ liệu của các hình.
- Patch áp được lên baseline `79e8024d61c3dd92ccf064fc67bb99cb999f70ab`
  và tái tạo đúng từng byte của commit `35ba531d8f28cb6400182b408e0676299386fcdb`.
- Bản áp patch biên dịch và đạt đủ 17 nhóm: MIT 5/5, TV1 50/50, tích hợp 70/70.
- Mã nguồn đã được clean trong bản tạm trước khi tạo ZIP.
- Source ZIP được giải nén để đối chiếu nội dung, kiểm tra quyền thực thi,
  biên dịch lại và chạy công cụ xác minh báo cáo từ chính gói source.
- Source ZIP chứa mã nguồn, log và công cụ kiểm chứng, cùng bản báo cáo
  có thể chỉnh sửa tại `docs/report-source/`. Bản nháp báo cáo cũ không được đóng gói.
- Bản TEX và hình trong Source ZIP đã được biên dịch lại; nội dung đủ
  12 trang khớp với PDF nộp kèm.
- Các tệp ZIP được kiểm tra tính toàn vẹn; SHA-256 nằm trong `SHA256SUMS.txt`.

Nhóm nên đọc lại bảng đóng góp trước khi nộp. Sau đó tải ZIP ngoài lên đúng
mục Lab 01 trên MOODLE và kiểm tra trạng thái bài nộp. Chưa thực hiện tải lên MOODLE.

Chi tiết kiểm chứng: `verification/manifest.json` và các log cùng thư mục.
