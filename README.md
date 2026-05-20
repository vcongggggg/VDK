# Hướng dẫn Khởi chạy Dự án: Nhà Kính Nông Nghiệp Thông Minh (Smart Greenhouse) - Nhóm 10

Dự án này đã được nâng cấp hoàn toàn theo cấu trúc chuẩn từ **ChatGPT Shared Link** và cấu hình chạy cục bộ cực kỳ tiện lợi:
*   **Realtime**: Dùng **Socket.IO** cho Dashboard kết nối thời gian thực & **Raw WebSockets** trên đường dẫn `/ws` cho ESP32.
*   **Cơ sở dữ liệu**: Dùng **SQLite** để lưu lịch sử đo đạc ngoại tuyến (tự động tạo tệp `greenhouse.db` trong thư mục dự án mà không cần cài đặt SQL Server phức tạp).
*   **Giao diện**: Dùng **Bootstrap 5** (giao diện kính mờ sang trọng) + **SweetAlert2** cho các hộp thoại cảnh báo thông minh.
*   **Giám sát kết nối**: Tích hợp tính năng phát hiện trạng thái **ONLINE/OFFLINE** của ESP32 qua cơ chế Heartbeat (Timeout 10 giây).

---

## 1. Cấu trúc thư mục dự án
*   **`backend/`**: Thư mục chứa máy chủ Node.js.
    *   `server.js`: Mã nguồn máy chủ chạy Express, Socket.IO và WebSockets.
    *   `public/index.html`: Giao diện Dashboard hiển thị số liệu, biểu đồ thời gian thực.
*   **`wokwi/`**: Thư mục chứa cấu hình giả lập Wokwi và ESP32.
    *   `SmartGreenhouse.ino`: Code nạp cho ESP32.
    *   `diagram.json` & `wokwi.toml`: Cấu hình chạy giả lập trên Wokwi (ESP32 DevKit V1).

---

## 2. Các bước thiết lập và Khởi chạy Server

### Bước 1: Khởi chạy Server Node.js
1.  Mở CMD / PowerShell trong thư mục `backend/` của dự án.
2.  Cài đặt các gói thư viện cần thiết:
    ```bash
    npm install
    ```
3.  Chạy máy chủ:
    ```bash
    node server.js
    ```
4.  Khi khởi chạy, chương trình sẽ tự động tạo file cơ sở dữ liệu `greenhouse.db` và bảng dữ liệu cảm biến trong thư mục `backend/`!
5.  Mở trình duyệt truy cập: **`http://localhost:3000`**

---

## 3. Các bước chạy trên mô phỏng Wokwi hoặc Nạp thực tế

### A. Đối với mô phỏng Wokwi (Trực tiếp trong VS Code)
1.  Đảm bảo đã cài đặt extension **Wokwi Simulator** trong VS Code.
2.  Mở thư mục `wokwi/` hoặc mở file `wokwi/diagram.json` hoặc `wokwi/SmartGreenhouse.ino`.
3.  Nhấn `F1` (hoặc `Ctrl+Shift+P`), chọn **Wokwi: Start Simulator**.
4.  ESP32 sẽ khởi chạy ảo, kết nối mạng `Wokwi-GUEST` và kết nối trực tiếp đến Node.js Server cục bộ của bạn trên máy tính qua WebSocket.

### B. Đối với mạch thực tế
1.  Mở `wokwi/SmartGreenhouse.ino` trên **Arduino IDE**.
2.  Cài đặt các thư viện sau thông qua Library Manager (Ctrl+Shift+I):
    *   `DHT sensor library`
    *   `ESP32Servo`
    *   `ArduinoJson`
    *   `WebSockets` (bởi Markus Sattler)
3.  Chỉnh sửa các cấu hình ở đầu file `SmartGreenhouse.ino`:
    *   `ssid` / `password`: WiFi của nhà bạn.
    *   `ws_host`: Địa chỉ IP cục bộ của máy tính chạy server (ví dụ: `192.168.1.15`).
4.  Chọn cổng COM, board `ESP32 Dev Module` và tiến hành nạp code.

---

## 4. Các kịch bản kiểm tra tính năng đồ án (Dành cho thi vấn đáp)

1.  **Kiểm tra tính năng Online Realtime:**
    *   Mở trình duyệt `http://localhost:3000`. Khi ESP32 chạy, badge trạng thái góc phải sẽ chuyển sang màu xanh lá: **`ESP32: ONLINE`**.
    *   Kéo thanh trượt Potentiometer (giả lập độ ẩm đất) hoặc thay đổi nhiệt độ trên DHT11, biểu đồ và các ô số liệu trên Web sẽ cập nhật tức thời (< 0.1 giây).

2.  **Kiểm tra tính năng Heartbeat Offline:**
    *   Tắt giả lập Wokwi (hoặc rút cáp ESP32). Sau đúng 10 giây, máy chủ phát hiện mất tín hiệu, Badge trên Web tự động nhấp nháy đỏ: **`ESP32: OFFLINE`**, các số đo chuyển thành `--`.

3.  **Kiểm tra tính năng Cảnh báo bất thường (SweetAlert2):**
    *   Kéo nhiệt độ DHT11 vượt **40°C** ➡️ Web lập tức hiện thông báo đỏ **SweetAlert2** báo động nguy hiểm, banner đỏ nhấp nháy.
    *   Kéo độ ẩm đất xuống dưới **30%** ➡️ Hiện thông báo cảnh báo đất bị khô hạn.
    *   Kéo ánh sáng LDR vượt **800 lux** ➡️ Hiện thông báo ánh nắng cực mạnh.

4.  **Kiểm tra Điều khiển từ xa (MANUAL Mode):**
    *   Nhấn nút chuyển sang chế độ **MANUAL Mode** trên Web.
    *   Thử nhấn nút **Đóng/Mở Mái Che**, **Bật/Tắt Máy Bơm**, **Bật/Tắt Quạt** ➡️ Đèn LED và động cơ Servo trên Wokwi/mạch thật sẽ phản hồi quay và sáng lập tức.

5.  **Kiểm tra tính năng Database SQLite (Offline Mode):**
    *   Dữ liệu cảm biến tự lưu vào file database cục bộ `greenhouse.db` mỗi 2 giây.
    *   Nhấn nút **"Tải lịch sử SQLite"** ➡️ Hệ thống gọi API xuống DB kéo dữ liệu cũ vẽ lại lên đồ thị.
