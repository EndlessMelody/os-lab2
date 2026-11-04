# Báo cáo Lab 2

Báo cáo tiếng Việt giữ bố cục, typography, bìa, logo, lớp, giảng viên và thông tin ba thành viên từ `OS1Lab_Report.tex`. Nội dung được viết lại cho trace, sysinfo và process tree; không chuyển kết quả Lab 1 thành kết quả Lab 2.

- PDF: [OS2Lab_Report.pdf](../../OS2Lab_Report.pdf).
- Source: [OS2Lab_Report.tex](../../OS2Lab_Report.tex).
- Bản theo tên MSSV: [24120293_24120347_24120468_Report.pdf](../../24120293_24120347_24120468_Report.pdf).
- Rà soát kỹ thuật và lỗi sửa khi tích hợp: [REVIEW.md](REVIEW.md).
- Mã nguồn được kiểm chứng: `626c6e0e4c2bf96654adefb7890918a61cd70bd0`; toàn bộ lệnh và mã thoát: [validation/run-context.json](validation/run-context.json).

## Tái lập minh chứng và PDF

Máy cần toolchain xv6 như README, Tectonic (hoặc XeLaTeX) và Python có `pymupdf`, `matplotlib` để dựng/kiểm tra hình.

```sh
python3 tools/verify-lab02.py --commit 626c6e0
python3 tools/render-lab02-evidence.py
tectonic --keep-logs --outdir /tmp/lab02-pdf OS2Lab_Report.tex
```

Tạo thư mục output trước khi gọi Tectonic. Tectonic cần truy cập mạng ở lần đầu để lấy bộ TeX và font; các lần sau dùng cache. Các đoạn QEMU dùng font monospace nhúng trong hình; nội dung báo cáo giữ font TeX Gyre Termes của mẫu Lab 1. Có thể thay vị trí công cụ cài cục bộ khi chạy.

Sau biên dịch, chép PDF về root và giữ bản log compiler tại `docs/lab02/validation/pdf-build.txt`. Chạy kiểm tra bằng:

```sh
python3 tools/validate-lab02-report.py
```

Validator kiểm tra bìa, A4, font nhúng, tham chiếu, giới hạn trang, kết quả test, hash minh chứng và nội dung trích từ log. Hình `combined` và `pstree` được dựng trực tiếp từ output QEMU; `architecture` là sơ đồ thiết kế được ghi rõ trong caption. Các hình có PDF, PNG và hồ sơ nguồn trong `figures/manifest.json`.

## Phạm vi kết luận

Report ghi nhận 7/7 nhóm trace, 9/9 nhóm tích hợp Lab 2, các bộ hồi quy Lab 1 và `usertests -q`. Các điểm script là nội bộ. Không gán review kỹ thuật tự động thành review chéo mang tên từng người; bảng đóng góp dựa vào phân công và lịch sử bàn giao, không có phần trăm suy đoán.

Báo cáo và bản tích hợp đã được chuẩn bị; chưa tạo gói ZIP bài nộp hoặc gửi lên MOODLE trong yêu cầu này. Source/baseline, cách tạo patch và quy định `Realease/`, `Source/` được giữ trong README và đề gốc.
