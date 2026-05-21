/*
  Code test Relay / Quạt làm mát kết nối với chân GPIO 17 (dành cho ESP32-S3)
  
  Sơ đồ nối dây điều khiển:
  - Chân IN (của Relay điều khiển Quạt) -> GPIO 17 của ESP32-S3
  - VCC (Relay) -> 5V nguồn ngoài (5V_BUS)
  - GND (Relay) -> GND nguồn ngoài (GND_BUS, nối chung GND với ESP32)
  
  Sơ đồ đấu nối quạt vào tiếp điểm Relay:
  - Dây Dương (+) của quạt (màu Đỏ) -> Cổng NO (Thường mở) của Relay
  - Dây Âm (-) của quạt (màu Đen)  -> Cổng GND nguồn ngoài (GND_BUS)
  - Cổng COM (Cổng ở giữa) của Relay -> Cổng 5V nguồn ngoài (5V_BUS)
*/

#define FAN_PIN  17  // Chân điều khiển Relay Quạt là GPIO 17

// Mặc định Relay Active LOW (Kích mức THẤP)
#define ON   LOW
#define OFF  HIGH

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== BAT DAU TEST RELAY QUAT TREN CHAN 17 ===");

  // Cấu hình chân GPIO 17 làm OUTPUT
  pinMode(FAN_PIN, OUTPUT);

  // Mới khởi động: Tắt quạt
  digitalWrite(FAN_PIN, OFF);
  Serial.println("Trạng thái: Đang TẮT quạt. Đợi 2 giây...");
  delay(2000);
}

void loop() {
  // Bật quạt
  Serial.println("--> BẬT Quạt (Mức logic LOW)");
  digitalWrite(FAN_PIN, ON);
  delay(4000); // Giữ quạt quay trong 4 giây

  // Tắt quạt
  Serial.println("--> TẮT Quạt (Mức logic HIGH)");
  digitalWrite(FAN_PIN, OFF);
  delay(4000); // Giữ tắt trong 4 giây
}
