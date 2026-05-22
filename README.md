# 🌿 Dự Án Nhà Kính Nông Nghiệp Thông Minh (Smart Greenhouse) - Nhóm 10

Dự án nghiên cứu và xây dựng hệ thống **Nhà Kính Thông Minh (Smart Greenhouse)** sử dụng vi điều khiển **ESP32-S3**, kết nối thời gian thực qua WebSockets/Socket.IO, lưu trữ SQLite và hỗ trợ mô phỏng trực quan trên Wokwi với giao diện Breadboard chuyên nghiệp.

---

## 📂 1. Cấu Trúc Dự Án

* 📁 **`backend/`**: Mã nguồn máy chủ Node.js & giao diện Web Dashboard.
  * 📄 `server.js`: Web server chạy Express, Socket.IO (giao tiếp Dashboard) và WebSockets (giao tiếp ESP32).
  * 📄 `AnomalyDetector.js`: Bộ phát hiện bất thường dựa trên thuật toán độ lệch chuẩn động (Dynamic Standard Deviation Baseline) và lọc nhiễu.
  * 📁 `public/`: Chứa file `index.html` (giao diện kính mờ Bootstrap 5 & SweetAlert2) và các tài nguyên frontend.
  * 📄 `greenhouse.db`: File cơ sở dữ liệu SQLite tự động tạo để lưu trữ lịch sử cảm biến.
* 📁 **`wokwi/`**: Chứa môi trường mô phỏng trên Wokwi.
  * 📄 `SmartGreenhouse.ino`: Chương trình điều khiển ESP32-S3 (firmware).
  * 📄 `diagram.json`: Cấu hình sơ đồ đấu nối mạch ảo với Breadboard.
  * 📄 `wokwi.toml`: Cấu hình chỉ định tệp biên dịch `.bin` và `.elf` dùng để nạp ảo cho mô phỏng.

---

## ⚡ 2. Sơ Đồ Đấu Nối Mạch (Chuẩn Breadboard Nguồn Đôi)

Để chống nhiễu sụt áp và bảo vệ linh kiện, hệ thống chạy cơ chế **Nguồn Đôi (Dual Power)** với cực âm nối chung (**Common GND**):

1. **Nguồn 3.3V (từ ESP32-S3):** Cấp cho dải nguồn dưới Breadboard (`bp` & `bn`), nuôi các cảm biến:
   * **Cảm biến nhiệt độ/độ ẩm khí (DHT11/DHT22):** Chân DATA ➡️ **GPIO 13**
   * **Cảm biến quang trở (LDR):** Chân Analog Out (AO) ➡️ **GPIO 1**
   * **Cảm biến độ ẩm đất (Biến trở):** Chân Analog Out (AO) ➡️ **GPIO 2**
2. **Nguồn ngoài 5V (Củ sạc/Cáp nguồn):** Cấp cho dải nguồn trên Breadboard (`tp` & `tn`), nuôi các thiết bị chấp hành:
   * **Động cơ Servo SG90 (Mái che):** Chân tín hiệu PWM ➡️ **GPIO 14**
   * **Module Relay Kênh 1 (Bơm):** Chân kích tín hiệu IN1 ➡️ **GPIO 18** (Đấu cổng NO - Thường mở, Active-HIGH)
   * **Module Relay Kênh 2 (Quạt):** Chân kích tín hiệu IN2 ➡️ **GPIO 17** (Đấu cổng NO - Thường mở, Active-HIGH)

---

## 🚀 3. Hướng Dẫn Thiết Lập & Khởi Chạy Server (Backend)

### Yêu cầu cài đặt trước:

* Đã cài đặt [Node.js](https://nodejs.org/) (Khuyến nghị phiên bản LTS từ 18 trở lên).

### Các bước khởi chạy:

1. Mở terminal (CMD / PowerShell) tại thư mục `backend/` của dự án.
2. Cài đặt các thư viện cần thiết:
   ```bash
   npm install
   ```
3. Khởi chạy máy chủ:
   ```bash
   node server.js
   ```
4. Khi khởi chạy thành công, terminal sẽ hiển thị:
   * `Server is running on port 3000`
   * `Database SQLite initialized successfully.`
5. Mở trình duyệt Web và truy cập: **`http://localhost:3000`** để vào trang Dashboard điều khiển.

---

## 🛠️ 4. Hướng Dẫn Nạp Code Mạch Thật (Arduino IDE)

1. Mở tệp [wokwi/SmartGreenhouse.ino](file:///c:/Study/VDK/SmartGreenhouse/wokwi/SmartGreenhouse.ino) bằng **Arduino IDE**.
2. Cài đặt các thư viện hỗ trợ qua **Library Manager** (`Ctrl + Shift + I`):
   * `DHT sensor library` (by Adafruit)
   * `ESP32Servo` (by Kevin Sweet)
   * `ArduinoJson` (by Benoit Blanchon)
   * `WebSockets` (by Markus Sattler)
3. Cập nhật thông tin kết nối WiFi và IP máy chủ ở đầu file:
   ```cpp
   const char* ssid     = "TÊN_WIFI_NHÀ_BẠN";
   const char* password = "MẬT_KHẨU_WIFI";
   const char* ws_host  = "ĐỊA_CHỈ_IP_CỤC_BỘ_CỦA_MÁY_TÍNH"; // Ví dụ: 192.168.1.15
   const int   ws_port  = 3000;
   ```
4. Chọn board **ESP32-S3 Dev Module** và cổng COM tương ứng, nhấn nút **Upload** để nạp chương trình.

---
