/*
  Code test máy bơm và quạt qua Module Relay 2 kênh (dành cho ESP32-S3)
  
  Sơ đồ nối dây (Theo so_do_mach.md):
  - Chân IN1 (Relay Bơm) -> GPIO 18 của ESP32-S3
  - Chân IN2 (Relay Quạt) -> GPIO 17 của ESP32-S3
  
  Lưu ý quan trọng về Relay:
  - Hầu hết các module Relay trên thị trường hiện nay là loại kích mức THẤP (Active LOW):
    + Xuất mức LOW (0) -> Relay đóng (Bật thiết bị), đèn LED trên relay sáng.
    + Xuất mức HIGH (1) -> Relay ngắt (Tắt thiết bị), đèn LED trên relay tắt.
  - Một số ít module Relay là loại kích mức CAO (Active HIGH):
    + Xuất mức HIGH (1) -> Bật.
    + Xuất mức LOW (0) -> Tắt.
  
  Code dưới đây được viết cho Relay Active LOW (phổ biến nhất). 
  Nếu bạn thấy ngược lại, hãy đổi giá trị ON và OFF ở phần khai báo bên dưới.
*/

// Định nghĩa chân điều khiển
#define RELAY_BOM_PIN   18  // GPIO 18 điều khiển Relay Bơm
#define RELAY_QUAT_PIN  17  // GPIO 17 điều khiển Relay Quạt

// Định nghĩa mức logic để Bật/Tắt (Mặc định cho Relay Active LOW)
#define RELAY_ON   LOW
#define RELAY_OFF  HIGH

void setup() {
  // Khởi tạo Serial để theo dõi trạng thái
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== BAT DAU KIEM TRA RELAY (MAY BOM & QUAT) ===");

  // Cấu hình chân Relay là OUTPUT
  pinMode(RELAY_BOM_PIN, OUTPUT);
  pinMode(RELAY_QUAT_PIN, OUTPUT);

  // Ban đầu tắt cả 2 thiết bị
  digitalWrite(RELAY_BOM_PIN, RELAY_OFF);
  digitalWrite(RELAY_QUAT_PIN, RELAY_OFF);
  Serial.println("Trang thai ban dau: TAT ca hai.");
  delay(2000);
}

void loop() {
  // --- BƯỚC 1: Bật Máy Bơm - Tắt Quạt ---
  Serial.println("\n[1] ---> BAT May Bom | TAT Quat");
  digitalWrite(RELAY_BOM_PIN, RELAY_ON);
  digitalWrite(RELAY_QUAT_PIN, RELAY_OFF);
  delay(5000); // Giữ trong 5 giây

  // --- BƯỚC 2: Tắt Máy Bơm - Bật Quạt ---
  Serial.println("[2] ---> TAT May Bom | BAT Quat");
  digitalWrite(RELAY_BOM_PIN, RELAY_OFF);
  digitalWrite(RELAY_QUAT_PIN, RELAY_ON);
  delay(5000); // Giữ trong 5 giây

  // --- BƯỚC 3: Bật cả hai thiết bị ---
  Serial.println("[3] ---> BAT ca hai (Bom & Quat)");
  digitalWrite(RELAY_BOM_PIN, RELAY_ON);
  digitalWrite(RELAY_QUAT_PIN, RELAY_ON);
  delay(5000); // Giữ trong 5 giây

  // --- BƯỚC 4: Tắt cả hai thiết bị ---
  Serial.println("[4] ---> TAT ca hai (Bom & Quat)");
  digitalWrite(RELAY_BOM_PIN, RELAY_OFF);
  digitalWrite(RELAY_QUAT_PIN, RELAY_OFF);
  delay(5000); // Nghỉ 5 giây trước khi lặp lại vòng tuần hoàn
}
