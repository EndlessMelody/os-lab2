# Lab 2 — Rà soát và tích hợp ngày 04/11/2026

Đối chiếu trực tiếp với `Lab2_OS_System_Call.docx`, mã xv6 đang dùng và quy ước trong `TEAM.md`. Rà soát tập trung vào giao diện syscall, quyền truy cập user memory, thứ tự khóa, vòng đời process và khả năng tái lập kiểm thử. Đây là rà soát kỹ thuật tích hợp theo yêu cầu nhóm; không gán kết quả này cho một thành viên thực hiện review chéo khi chưa có biên bản riêng.

## Các nhánh đã nhận

| Phần | Thành viên theo phân công | Commit GitHub đã rà soát |
|---|---|---|
| Trace | Phan Lê Đăng Khoa | `0d06ca9a3885f485ccd17bb18031373cd506efcb` |
| Sysinfo | Âu Bảo Trân | `05da92d9345e095dd095e82dbc6eb1895294ce32` |
| Process tree | Dương Nhựt Duy | `c0a4bb7e5f2128ed0d6334286fc016e2b02e4a8a` |

Các nhánh được merge bằng merge commit, giữ nguyên lịch sử và tác giả. Các sửa chữa tích hợp nằm trong commit riêng để phân biệt với phần bàn giao.

## Nhận xét triển khai

- **Trace:** đăng ký ID 22 đầy đủ; mask riêng từng process, reset khi cấp/thu hồi slot và sao chép trước khi con runnable. Log sau handler, giữ qua exec, không thay mask các process khác. CLI kiểm tra số và tràn số; syscall exit không có log trả về vì không trở lại dispatcher.
- **Sysinfo:** ID 23; đếm free list dưới `kmem.lock`, nhân theo `PGSIZE`, đếm mọi state khác `UNUSED` dưới từng `p->lock`. Kiểm tra phạm vi bằng phép trừ tránh tràn và dùng `copyout`; không ghi trực tiếp qua user pointer. Đếm memory và process là hai lần quan sát riêng, không phải snapshot toàn hệ thống tại một thời điểm. Giữ sửa chữ ký `rwsbrk(char *s)` để khớp giao diện test có sẵn.
- **Process tree:** ID 24; kernel trả bản ghi, user dựng cây. Thứ tự `wait_lock` rồi `p->lock` phù hợp quy tắc parent pointer của xv6; thả khóa trước `copyout`. Giới hạn `max` ở `NPROC`; user có mảng visited để không in lặp khi snapshot thiếu parent hoặc không còn ổn định.

Các thuật toán dùng cấu trúc dữ liệu hiện có của xv6: bitmask, duyệt danh sách free page, duyệt bảng process cố định và dựng cây trong user space. Không cần thư viện ngoài hay cơ chế kernel mới để đáp ứng đề. Nguồn tham khảo và phạm vi kiểm chứng được ghi trong báo cáo.

## Lỗi phát hiện và sửa trước khi tích hợp

1. **Ptree nhận buffer ngoài logical heap.** Với break không căn trang, địa chỉ đúng bằng `sbrk(0)` vẫn nằm trong trang được map, nên chỉ `copyout` chưa đủ kiểm tra giới hạn `sz`. Test trước sửa thất bại tại `ptree rejects buffer at unaligned logical break`. Bổ sung kiểm tra phạm vi của từng bản ghi bằng `sz - dst`.
2. **Ptree sao chép byte chưa khởi tạo.** Padding của ABI và phần đuôi `name` có thể chứa dữ liệu stack kernel; test trước sửa ghi nhận `ptree clears unused name bytes` thất bại. Zero toàn bộ struct trước khi điền thông tin; kiểm tra lại cả padding lẫn name.
3. **Pstree thu hẹp memsize.** Bản bàn giao ép `uint64` về `int`. Bỏ ép kiểu và in thập phân đủ 64 bit bằng hàm nhỏ riêng, vì user printf của baseline cũng thu hẹp `%lu`. Test gọi formatter thật với `2^32` và `UINT64_MAX`; đây là dữ liệu kiểm tra formatter, không phải dung lượng RAM xv6 đo được.
4. **Churn ngoài phạm vi ptree.** Commit cuối nhánh ptree đổi CRLF trên phần lớn source và xóa nguồn báo cáo Lab 1. Merge giữ thay đổi chức năng, chuẩn hóa LF, khôi phục `.tex` và thêm `.gitattributes` để tránh tái diễn. Lịch sử gốc vẫn được giữ.
5. **Conflict file chung.** Ghép đủ cả ba handler, user stub, prototype và UPROGS; giữ ID 22/23/24 và tên syscall tương ứng để trace được sysinfo/ptree.

Minh chứng trước sửa: `validation/before-tests.txt`, `before-badbuf.txt`, `before-ptree.txt`. Minh chứng cuối cùng và commit thực sự được kiểm thử do `tools/verify-lab02.py` ghi vào `validation/run-context.json`.

## Giới hạn được giữ rõ

- Ptree và sysinfo là quan sát trong lúc hệ thống đang chạy, không bảo đảm snapshot nguyên tử toàn bảng. Parent có thể biến mất hoặc bị reparent giữa các bản ghi; pstree in node không có parent trong snapshot như một gốc.
- Nếu một lần copyout về sau thất bại, syscall trả `-1` nhưng các bản ghi trước đó có thể đã được ghi; user không được dùng kết quả khi trả lỗi.
- Trace thêm I/O console nên làm thay đổi lịch thực thi; các test kiểm chứng chức năng, không đo hiệu năng.
- Đạt test nội bộ không đồng nghĩa chứng minh mọi xen kẽ đồng thời hoặc điểm chấm chính thức. Hồ sơ này cũng không xác nhận định lượng đóng góp chỉ từ số dòng/commit.
