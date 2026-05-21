# Sơ Đồ Đấu Nối Mạch Chi Tiết - Hệ Thống Nhà Kính Thông Minh (ESP32-S3)

Tài liệu này hướng dẫn chi tiết cách đấu nối dây cho mô hình mạch thật sử dụng vi điều khiển **ESP32-S3**, kết hợp nguồn đôi (Laptop cho vi điều khiển + Củ sạc ngoài 5V cho động cơ & rơ-le) nhằm đảm bảo hoạt động an toàn và chống nhiễu sụt áp.

---

## 1. Nguyên Tắc Cấp Nguồn Đôi (Dual Power)

Để bảo vệ cổng USB Laptop và tránh vi điều khiển ESP32-S3 bị reset đột ngột khi các động cơ hoạt động:
1. **Nguồn từ Laptop (qua cáp USB):** Nuôi board mạch **ESP32-S3** và **các cảm biến** (DHT11, Cảm biến đất, Quang trở LDR).
2. **Nguồn từ Củ sạc 5V-2A (qua dây cắt nối vào Breadboard):** Nuôi riêng **Động cơ Servo SG90** và **Module Relay (Máy bơm, Quạt)**.
3. **Nối chung GND:** Bắt buộc nối chân **GND của ESP32-S3** với đường **cực âm (GND) của nguồn ngoài** trên Breadboard.

---

## 2. Bảng Tra Cứu Sơ Đồ Chân (Pinout)

| Thiết bị | Chân thiết bị | Chân ESP32-S3 | Chân nguồn cấp | Ghi chú |
| :--- | :---: | :---: | :---: | :--- |
| **ESP32-S3** | GND | - | GND nguồn ngoài | Nối chung GND |
| **Cảm biến DHT11** | DATA / OUT | **GPIO 13** | 3.3V (ESP32-S3) | Dùng 3.3V để bảo vệ chân ADC |
| **Cảm biến độ ẩm đất** | AO (Analog Out)| **GPIO 2** | 3.3V (ESP32-S3) | Dùng 3.3V để bảo vệ chân ADC |
| **Cảm biến ánh sáng LDR**| AO (Analog Out)| **GPIO 1** | 3.3V (ESP32-S3) | Dùng 3.3V để bảo vệ chân ADC |
| **Động cơ Servo SG90** | Signal | **GPIO 14** | - | Nhận tín hiệu PWM điều khiển góc |
| **Động cơ Servo SG90** | VCC | - | 5V_BUS (Củ sạc) | Cấp nguồn ngoài để tránh sụt áp ESP32 |
| **Động cơ Servo SG90** | GND | - | GND_BUS (Chung) | Cực âm nguồn ngoài |
| **Module Relay (Bơm)** | IN1 | **GPIO 18** | 5V_BUS (Củ sạc) | Kích hoạt bơm nước |
| **Module Relay (Quạt)** | IN2 | **GPIO 17** | 5V_BUS (Củ sạc) | Kích hoạt quạt tản nhiệt |

---

## 3. Hướng Dẫn Đấu Nối Dây Chi Tiết Theo Từng Khối

### Bước 1: Thiết lập đường nguồn chung trên Breadboard (Nguồn ngoài)
* Nối dây Dương (+) của củ sạc 5V vào cột dọc màu đỏ (`+`) trên Breadboard (sau đây gọi là **5V_BUS**).
* Nối dây Âm (-) của củ sạc 5V vào cột dọc màu xanh/đen (`-`) trên Breadboard (sau đây gọi là **GND_BUS**).
* Nối 1 sợi dây từ chân **GND** của ESP32-S3 sang cột **GND_BUS** của Breadboard.

### Bước 2: Nối dây cho các cảm biến (DHT11, Đất, LDR)
Để đảm bảo tín hiệu đo đạc không bị nhiễu do động cơ, cấp nguồn 3.3V từ ESP32-S3:
* **Cảm biến DHT11 (Nhiệt độ/Ẩm khí):**
  * VCC ➡️ Nối chân **3.3V** của ESP32-S3.
  * GND ➡️ Nối chân **GND** của ESP32-S3.
  * DATA ➡️ Nối chân **GPIO 13** của ESP32-S3.
* **Cảm biến độ ẩm đất:**
  * VCC ➡️ Nối chân **3.3V** của ESP32-S3.
  * GND ➡️ Nối chân **GND** của ESP32-S3.
  * AO (Analog Out) ➡️ Nối chân **GPIO 2** của ESP32-S3.
* **Cảm biến ánh sáng LDR (Quang trở):**
  * VCC ➡️ Nối chân **3.3V** của ESP32-S3.
  * GND ➡️ Nối chân **GND** của ESP32-S3.
  * AO (Analog Out) ➡️ Nối chân **GPIO 1** của ESP32-S3.

### Bước 3: Nối dây cho Động cơ Servo SG90 (Mở/Đóng mái che)
* **Nối nguồn cho Servo:**
  * Dây **Đỏ (VCC)** của Servo ➡️ Nối vào **5V_BUS** (Nguồn ngoài).
  * Dây **Nâu hoặc Đen (GND)** của Servo ➡️ Nối vào **GND_BUS** (Nối chung cực âm).
* **Nối tín hiệu:**
  * Dây **Vàng hoặc Cam (Signal)** của Servo ➡️ Nối chân **GPIO 14** của ESP32-S3.

### Bước 4: Nối dây Module Relay 5V & Thiết bị chấp hành (Bơm & Quạt)
* **Nối nguồn nuôi Relay:**
  * Chân **VCC** của module Relay ➡️ Nối vào **5V_BUS**.
  * Chân **GND** của module Relay ➡️ Nối vào **GND_BUS**.
* **Nối tín hiệu điều khiển:**
  * Chân **IN1** (Bơm) ➡️ Nối chân **GPIO 18** của ESP32-S3.
  * Chân **IN2** (Quạt) ➡️ Nối chân **GPIO 17** của ESP32-S3.
* **Đấu nối máy bơm nước mini:**
  * Nối 1 sợi dây từ **5V_BUS** vào cổng **COM** (Cổng giữa) của Relay 1.
  * Nối dây **Dương (+)** của máy bơm nước vào cổng **NO** (Thường mở) của Relay 1.
  * Nối dây **Âm (-)** của máy bơm trực tiếp vào **GND_BUS**.
* **Đấu nối quạt làm mát:**
  * Nối 1 sợi dây từ **5V_BUS** vào cổng **COM** (Cổng giữa) của Relay 2.
  * Nối dây **Dương (+)** của quạt vào cổng **NO** (Thường mở) của Relay 2.
  * Nối dây **Âm (-)** của quạt trực tiếp vào **GND_BUS**.

---

## 4. Quy Trình Kiểm Tra Sau Khi Nối Dây (Chạy thử)

1. **Kiểm tra nguội (Khi chưa cắm nguồn điện):**
   * Đảm bảo không có bất kỳ chân cấp nguồn `5V_BUS` nào chạm trực tiếp vào chân `GND_BUS` (tránh chập mạch nguồn).
   * Kiểm tra xem chân GND của ESP32-S3 và GND của nguồn ngoài đã được nối chung bằng 1 sợi dây chưa.
2. **Cấp nguồn hệ thống:**
   * Cắm cáp USB nối Laptop với ESP32-S3 trước.
   * Cắm củ sạc 5V-2A ngoài vào ổ điện để cấp nguồn cho Servo/Relay.
3. **Thử nghiệm từng thiết bị:**
   * **Servo mái che:** Chạy thử code test hoặc điều khiển từ web để xem Servo có quay đúng góc 0 độ (Đóng) và 90 độ (Mở) như thiết kế hay không.
   * **Bơm & Quạt:** Bật/Tắt từ giao diện web (hoặc dùng code test), kiểm tra tiếng kêu tạch tạch của Relay và chuyển động vật lý của quạt/bơm.
