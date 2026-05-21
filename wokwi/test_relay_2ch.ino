/*
  Code test Module Relay 2 Kênh (dành cho ESP32-S3)
  
  Sơ đồ nối dây giữa ESP32-S3 và Module Relay 2 Kênh:
  - VCC (Relay)  -> Cấp nguồn 5V ngoài (5V_BUS)
  - GND (Relay)  -> Cấp nguồn GND ngoài (GND_BUS, nhớ nối chung GND với ESP32)
  - IN1 (Kênh 1) -> GPIO 18 của ESP32-S3
  - IN2 (Kênh 2) -> GPIO 17 của ESP32-S3
  
  Chương trình này sẽ bật/tắt tuần tự từng kênh để bạn nghe tiếng đóng ngắt "tạch tạch"
  và quan sát đèn LED trên module relay xem kênh nào đang hoạt động.
*/

// Định nghĩa chân điều khiển IN1 và IN2
#define RELAY_CH1  18  // Kênh 1 nối GPIO 18
#define RELAY_CH2  17  // Kênh 2 nối GPIO 17

// Hầu hết Relay Arduino/ESP32 trên thị trường là Active LOW (Kích mức THẤP)
// - LOW  (0V) -> Bật Relay (đèn sáng)
// - HIGH (5V) -> Tắt Relay (đèn tắt)
#define ON   LOW
#define OFF  HIGH

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== BAT DAU KIEM TRA MODULE RELAY 2 KENH ===");

  // Cấu hình chân tín hiệu đầu ra
  pinMode(RELAY_CH1, OUTPUT);
  pinMode(RELAY_CH2, OUTPUT);

  // Mới khởi động: Tắt cả 2 kênh
  digitalWrite(RELAY_CH1, OFF);
  digitalWrite(RELAY_CH2, OFF);
  Serial.println("Trạng thái: Đang TẮT cả 2 kênh. Đợi 2 giây...");
  delay(2000);
}

void loop() {
  // 1. Bật Kênh 1, Tắt Kênh 2
  Serial.println("\n--> BẬT Kênh 1 | TẮT Kênh 2");
  digitalWrite(RELAY_CH1, ON);
  digitalWrite(RELAY_CH2, OFF);
  delay(3000); // Giữ trạng thái trong 3 giây

  // 2. Tắt Kênh 1, Bật Kênh 2
  Serial.println("--> TẮT Kênh 1 | BẬT Kênh 2");
  digitalWrite(RELAY_CH1, OFF);
  digitalWrite(RELAY_CH2, ON);
  delay(3000);

  // 3. Bật cả 2 Kênh cùng lúc
  Serial.println("--> BẬT cả 2 Kênh");
  digitalWrite(RELAY_CH1, ON);
  digitalWrite(RELAY_CH2, ON);
  delay(3000);

  // 4. Tắt cả 2 Kênh cùng lúc
  Serial.println("--> TẮT cả 2 Kênh");
  digitalWrite(RELAY_CH1, OFF);
  digitalWrite(RELAY_CH2, OFF);
  delay(3000);
}
