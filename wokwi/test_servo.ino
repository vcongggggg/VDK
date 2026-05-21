/*
  Code test Động cơ Servo SG90 (dành cho ESP32-S3)
  Sử dụng thư viện ESP32Servo.
  
  Sơ đồ đấu nối dây cho Servo SG90:
  1. Dây ĐỎ (Dương VCC)    -> Cắm vào nguồn 5V ngoài (5V_BUS)
  2. Dây NÂU/ĐEN (Âm GND)  -> Cắm vào GND ngoài (GND_BUS, nối chung GND với ESP32)
  3. Dây CAM/VÀNG (Signal) -> Cắm vào chân GPIO 14 của ESP32-S3
  
  Lưu ý: Không cắm trực tiếp dây Đỏ của Servo vào chân 3.3V hoặc 5V của ESP32 
  vì Servo khi quay có thể gây nhiễu/sụt áp làm ESP32 bị reset.
*/

#include <ESP32Servo.h>

#define SERVO_PIN 14  // Chân tín hiệu điều khiển Servo

Servo myServo;  // Khai báo đối tượng điều khiển Servo

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== BAT DAU KIEM TRA SERVO SG90 ===");

  // Cho phép ESP32Servo tự động cấp phát timer phù hợp
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  myServo.setPeriodHertz(50);    // Tần số tiêu chuẩn cho Servo SG90 là 50Hz
  myServo.attach(SERVO_PIN, 500, 2400); // Gắn chân servo với dải xung 500us - 2400us
  
  // Khởi động quay về 0 độ
  myServo.write(0);
  Serial.println("Quay về 0 độ (Trạng thái Đóng)");
  delay(2000);
}

void loop() {
  // Quay sang 90 độ (Mở một nửa hoặc toàn bộ)
  Serial.println("Quay sang 90 độ (Trạng thái Mở)");
  myServo.write(90);
  delay(3000); // Đợi 3 giây

  // Quay sang 180 độ
  Serial.println("Quay sang 180 độ");
  myServo.write(180);
  delay(3000); // Đợi 3 giây

  // Quay về 0 độ
  Serial.println("Quay về 0 độ (Trạng thái Đóng)");
  myServo.write(0);
  delay(3000); // Đợi 3 giây
}
