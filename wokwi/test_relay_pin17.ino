/*
  Code test Relay / Máy bơm kết nối với chân GPIO 17 (dành cho ESP32-S3)
  
  Sơ đồ nối dây:
  - Chân IN (của Relay điều khiển Bơm) -> GPIO 17 của ESP32-S3
  - VCC (Relay) -> 5V nguồn ngoài (5V_BUS)
  - GND (Relay) -> GND nguồn ngoài (GND_BUS, nối chung GND với ESP32)
*/

#define PUMP_PIN  17  // Khai báo máy bơm / relay kết nối chân GPIO 17

// Mặc định Relay Active LOW (Kích mức THẤP)
#define ON   LOW
#define OFF  HIGH

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== BAT DAU TEST RELAY PUMP TREN CHAN 17 ===");

  // Cấu hình chân GPIO 17 làm OUTPUT
  pinMode(PUMP_PIN, OUTPUT);

  // Mới khởi động: Tắt bơm
  digitalWrite(PUMP_PIN, OFF);
  Serial.println("Trạng thái: Đang TẮT bơm. Đợi 2 giây...");
  delay(2000);
}

void loop() {
  // Bật máy bơm
  Serial.println("--> BẬT Máy Bơm (Mức logic LOW)");
  digitalWrite(PUMP_PIN, ON);
  delay(4000); // Giữ bơm chạy trong 4 giây

  // Tắt máy bơm
  Serial.println("--> TẮT Máy Bơm (Mức logic HIGH)");
  digitalWrite(PUMP_PIN, OFF);
  delay(4000); // Giữ tắt trong 4 giây
}
