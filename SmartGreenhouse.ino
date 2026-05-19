#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <ESP32Servo.h>

// ── CẤU HÌNH WIFI & WEBSOCKET SERVER ───────────────────────────
const char* ssid     = "Wokwi-GUEST"; // Thay đổi bằng WiFi nhà bạn khi nạp thực tế
const char* password = "";
const char* ws_host  = "192.168.1.7"; // Thay đổi bằng địa chỉ IP của máy tính chạy Node.js
const int   ws_port  = 3000;

// ── KHAI BÁO CÁC CHÂN CẮM (PINS) ───────────────────────────────
#define DHTPIN 13
#define DHTTYPE DHT11

#define SOIL_PIN 2
#define LDR_PIN 1
#define SERVO_PIN 14
#define PUMP_RELAY_PIN 18
#define FAN_RELAY_PIN 17

// ── KHỞI TẠO ĐỐI TƯỢNG ─────────────────────────────────────────
DHT dht(DHTPIN, DHTTYPE);
Servo roofServo;
WebSocketsClient webSocket;

// ── BIẾN TOÀN CỤC QUẢN LÝ TRẠNG THÁI ───────────────────────────
bool isAutoMode = true; // Mặc định là chế độ Tự động

bool pumpState = false; // false = Tắt, true = Bật
bool fanState = false;  // false = Tắt, true = Bật
bool roofState = false; // false = Đóng (0 độ), true = Mở (90 độ)

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 2000; // Gửi dữ liệu định kỳ mỗi 2s

// ── HÀM ĐỌC CẢM BIẾN & ĐIỀU KHIỂN TỰ ĐỘNG ────────────────────────
void handleGreenhouseLogic() {
  // Đọc dữ liệu từ DHT11
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  
  if (isnan(temp) || isnan(hum)) {
    Serial.println("Lỗi: Không đọc được dữ liệu từ cảm biến DHT11!");
    temp = 0.0;
    hum = 0.0;
  }

  // Đọc độ ẩm đất và ánh sáng (Giá trị analog 0 - 4095)
  int rawSoil = analogRead(SOIL_PIN);
  int rawLight = analogRead(LDR_PIN);

  // Quy đổi giá trị Analog sang phần trăm (%) để hiển thị dễ hiểu
  float soilMoisture = map(rawSoil, 4095, 0, 0, 100); // 4095 tương đương khô, 0 tương đương ngập nước
  if (soilMoisture < 0) soilMoisture = 0;
  if (soilMoisture > 100) soilMoisture = 100;

  float lightLevel = map(rawLight, 4095, 0, 0, 1000); // Quy đổi tượng trưng sang lux
  if (lightLevel < 0) lightLevel = 0;

  // LOGIC TỰ ĐỘNG (Chỉ hoạt động khi ở chế độ AUTO)
  if (isAutoMode) {
    // 1. Điều khiển Mái che bằng Ánh sáng
    if (lightLevel > 400) { // Trời sáng
      roofState = true;
      roofServo.write(90); // Mở mái che 90 độ
    } else { // Trời tối
      roofState = false;
      roofServo.write(0); // Đóng mái che về 0 độ
    }

    // 2. Điều khiển Máy bơm bằng Độ ẩm đất
    if (soilMoisture < 30.0) { // Đất quá khô
      pumpState = true;
      digitalWrite(PUMP_RELAY_PIN, HIGH); // Bật bơm (Relay mức CAO)
    } else {
      pumpState = false;
      digitalWrite(PUMP_RELAY_PIN, LOW); // Tắt bơm
    }

    // 3. Điều khiển Quạt tản nhiệt bằng Nhiệt độ
    if (temp > 35.0) { // Nhiệt độ vượt ngưỡng an toàn
      fanState = true;
      digitalWrite(FAN_RELAY_PIN, HIGH); // Bật quạt
    } else {
      fanState = false;
      digitalWrite(FAN_RELAY_PIN, LOW); // Tắt quạt
    }
  } else {
    // Trong chế độ MANUAL: Động cơ sẽ giữ nguyên trạng thái được điều khiển từ Web
    roofServo.write(roofState ? 90 : 0);
    digitalWrite(PUMP_RELAY_PIN, pumpState ? HIGH : LOW);
    digitalWrite(FAN_RELAY_PIN, fanState ? HIGH : LOW);
  }

  // Gửi gói tin JSON lên Server qua WebSocket
  unsigned long now = millis();
  if (now - lastSendTime >= sendInterval) {
    lastSendTime = now;

    StaticJsonDocument<256> doc;
    doc["temperature"] = temp;
    doc["humidity"] = hum;
    doc["soil_moisture"] = soilMoisture;
    doc["light_level"] = lightLevel;
    doc["roof_status"] = roofState ? "MO (90 DEG)" : "DONG (0 DEG)";
    doc["pump_status"] = pumpState ? "BAT" : "TAT";
    doc["fan_status"] = fanState ? "BAT" : "TAT";
    doc["mode"] = isAutoMode ? "auto" : "manual";

    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.sendTXT(jsonString);
    Serial.println("Đã gửi dữ liệu: " + jsonString);
  }
}

// ── XỬ LÝ SỰ KIỆN WEBSOCKET (Nhận lệnh từ Dashboard) ───────────────
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WS] Đã ngắt kết nối với Server!");
      break;
    case WStype_CONNECTED:
      Serial.println("[WS] Đã kết nối thành công tới Server!");
      break;
    case WStype_TEXT:
      Serial.printf("[WS] Nhận tin nhắn: %s\n", payload);
      
      // Giải mã lệnh điều khiển JSON
      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, payload, length);
      if (error) {
        Serial.print("Lỗi parse JSON: ");
        Serial.println(error.c_str());
        return;
      }

      // 1. Lệnh thiết lập chế độ Auto / Manual
      if (doc.containsKey("cmd") && doc["cmd"] == "set_mode") {
        String mode = doc["mode"];
        isAutoMode = (mode == "auto");
        Serial.printf("Đã chuyển chế độ: %s\n", isAutoMode ? "AUTO" : "MANUAL");
      }

      // 2. Các lệnh điều khiển thủ công (Chỉ thực hiện nếu đang ở MANUAL mode)
      if (!isAutoMode && doc.containsKey("cmd")) {
        String cmd = doc["cmd"];
        if (cmd == "toggle_roof") {
          roofState = !roofState;
          Serial.printf("Thủ công: Mái che -> %s\n", roofState ? "MO" : "DONG");
        } 
        else if (cmd == "toggle_pump") {
          pumpState = !pumpState;
          Serial.printf("Thủ công: Bơm -> %s\n", pumpState ? "BAT" : "TAT");
        } 
        else if (cmd == "toggle_fan") {
          fanState = !fanState;
          Serial.printf("Thủ công: Quạt -> %s\n", fanState ? "BAT" : "TAT");
        }
      }
      break;
  }
}

// ── SETUP & LOOP ───────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // Thiết lập chân ngõ ra cho các Relay điều khiển
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(FAN_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, LOW); // Mặc định tắt bơm ban đầu
  digitalWrite(FAN_RELAY_PIN, LOW);  // Mặc định tắt quạt ban đầu

  // Khởi động các cảm biến & Động cơ
  dht.begin();
  roofServo.attach(SERVO_PIN);
  roofServo.write(0); // Khởi tạo góc mái che ở 0 độ (Đóng)

  // Kết nối WiFi
  WiFi.begin(ssid, password);
  Serial.print("Đang kết nối WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nĐã kết nối WiFi thành công!");
  Serial.print("Địa chỉ IP của ESP32: ");
  Serial.println(WiFi.localIP());

  // Cấu hình WebSocket Client kết nối đến server Node.js
  webSocket.begin(ws_host, ws_port, "/ws");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000); // Tự động thử kết nối lại sau 5s nếu mất mạng
}

void loop() {
  webSocket.loop();
  handleGreenhouseLogic();
  delay(50); // Delay nhẹ để hệ thống chạy ổn định
}
