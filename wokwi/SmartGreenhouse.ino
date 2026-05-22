#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <ESP32Servo.h>

// ── CẤU HÌNH WIFI & WEBSOCKET SERVER ───────────────────────────
const char* ssid     = "KTKH P208 C"; // Thay đổi bằng WiFi nhà bạn khi nạp thực tế
const char* password = "DUTITF2005";
const char* ws_host  = "192.168.1.8"; // Thay đổi bằng địa chỉ IP của máy tính chạy Node.js
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

// Lưu trữ trạng thái trước đó để chỉ ghi khi có sự thay đổi
bool lastPumpState = false;
bool lastFanState = false;
bool lastRoofState = false;

// Bộ lọc nhiễu số (EMA Filter) cho các chân Analog
float filteredSoil = 4095.0;  // Khởi tạo mức khô ban đầu
float filteredLight = 4095.0; // Khởi tạo mức tối ban đầu
const float emaAlpha = 0.15;  // Hệ số lọc (0.15 giúp lọc nhiễu tốt mà không trễ nhiều)

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 2000; // Gửi dữ liệu và đọc DHT11 mỗi 2 giây

// Các biến lưu giá trị cảm biến đã được làm mượt
float currentTemp = 0.0;
float currentHum = 0.0;
float currentSoilMoisture = 0.0;
float currentLightLevel = 0.0;

// Hàm cập nhật trạng thái cơ cấu chấp hành (chỉ kích hoạt khi có thay đổi trạng thái)
void updateActuators() {
  // Điều khiển Máy bơm (Active HIGH - Đấu cổng NO)
  if (pumpState != lastPumpState) {
    digitalWrite(PUMP_RELAY_PIN, pumpState ? HIGH : LOW); // ON -> HIGH (hút relay), OFF -> LOW (nhả relay)
    lastPumpState = pumpState;
    Serial.printf("[HÀNH ĐỘNG] Máy bơm -> %s\n", pumpState ? "BẬT" : "TẮT");
  }
  
  // Điều khiển Quạt (Active HIGH)
  if (fanState != lastFanState) {
    digitalWrite(FAN_RELAY_PIN, fanState ? HIGH : LOW); // ON -> HIGH, OFF -> LOW
    lastFanState = fanState;
    Serial.printf("[HÀNH ĐỘNG] Quạt tản nhiệt -> %s\n", fanState ? "BẬT" : "TẮT");
  }
  
  // Điều khiển Mái che / Rèm (Servo)
  if (roofState != lastRoofState) {
    roofServo.write(roofState ? 45 : 90); // Mở = 45 độ, Đóng = 90 độ
    lastRoofState = roofState;
    Serial.printf("[HÀNH ĐỘNG] Mái che -> %s\n", roofState ? "MỞ" : "ĐÓNG");
  }
}

// ── HÀM ĐỌC CẢM BIẾN & ĐIỀU KHIỂN TỰ ĐỘNG ────────────────────────
void handleGreenhouseLogic() {
  // 1. Đọc và lọc nhiễu các cảm biến Analog liên tục mỗi vòng loop để bộ lọc EMA hoạt động mịn
  int rawSoil = analogRead(SOIL_PIN);
  int rawLight = analogRead(LDR_PIN);

  // Bộ lọc thông thấp Exponential Moving Average (EMA)
  filteredSoil = (emaAlpha * rawSoil) + ((1.0 - emaAlpha) * filteredSoil);
  filteredLight = (emaAlpha * rawLight) + ((1.0 - emaAlpha) * filteredLight);

  // Quy đổi các giá trị đã lọc ra đơn vị hiển thị
  currentSoilMoisture = map((int)filteredSoil, 4095, 0, 0, 100);
  if (currentSoilMoisture < 0) currentSoilMoisture = 0;
  if (currentSoilMoisture > 100) currentSoilMoisture = 100;

  currentLightLevel = map((int)filteredLight, 4095, 0, 0, 1000);
  if (currentLightLevel < 0) currentLightLevel = 0;

  // 2. Logic điều khiển tự động và gửi dữ liệu định kỳ mỗi 2 giây
  unsigned long now = millis();
  if (now - lastSendTime >= sendInterval) {
    lastSendTime = now;

    // Đọc cảm biến DHT11 (Chỉ đọc mỗi 2s để tránh quá nhiệt/lỗi cảm biến)
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    
    if (!isnan(temp) && !isnan(hum)) {
      currentTemp = temp;
      currentHum = hum;
    } else {
      Serial.println("Lỗi: Không đọc được dữ liệu từ cảm biến DHT11!");
    }

    // LOGIC TỰ ĐỘNG VỚI NGƯỠNG TRỄ (HYSTERESIS) để chống rung relay/servo
    if (isAutoMode) {
      // 2.1. Rèm/Mái che (Phụ thuộc: Ánh sáng)
      // Mở khi ánh sáng > 400, đóng khi ánh sáng < 300
      if (currentLightLevel > 400.0) {
        roofState = true;
      } else if (currentLightLevel < 300.0) {
        roofState = false;
      }

      // 2.2. Máy bơm (Phụ thuộc: Điện dung đất & Ánh sáng)
      // Bơm khi đất khô (< 30%) VÀ trời không quá nắng gắt (ánh sáng < 800) để tránh bốc hơi nhanh/cháy lá
      if (currentSoilMoisture < 30.0 && currentLightLevel < 800.0) {
        pumpState = true;
      } else if (currentSoilMoisture > 45.0 || currentLightLevel >= 800.0) {
        pumpState = false;
      }

      // 2.3. Quạt tản nhiệt (Phụ thuộc: Nhiệt độ)
      // Bật quạt khi nhiệt độ > 40°C, tắt khi < 35°C
      if (currentTemp > 40.0) {
        fanState = true;
      } else if (currentTemp < 35.0) {
        fanState = false;
      }
    }

    // Cập nhật trạng thái cơ cấu chấp hành nếu có thay đổi
    updateActuators();

    // Gửi dữ liệu JSON lên Server qua WebSocket
    StaticJsonDocument<256> doc;
    doc["temperature"] = currentTemp;
    doc["humidity"] = currentHum;
    doc["soil_moisture"] = currentSoilMoisture;
    doc["light_level"] = currentLightLevel;
    doc["roof_status"] = roofState ? "MO" : "DONG";
    doc["pump_status"] = pumpState ? "BAT" : "TAT";
    doc["fan_status"] = fanState ? "BAT" : "TAT";
    doc["mode"] = isAutoMode ? "auto" : "manual";

    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.sendTXT(jsonString);
    Serial.println("Đã gửi dữ liệu: " + jsonString);
  }

  // Trong chế độ MANUAL: Cập nhật cơ cấu chấp hành ngay lập tức nếu nhận lệnh từ web
  if (!isAutoMode) {
    updateActuators();
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

  // Thiết lập chân ngõ ra cho các Relay điều khiển (Đấu cổng NO - Active HIGH)
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(FAN_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, LOW); // Mặc định tắt bơm ban đầu (Active HIGH)
  digitalWrite(FAN_RELAY_PIN, LOW);  // Mặc định tắt quạt ban đầu (Active HIGH)

  // Khởi động các cảm biến & Động cơ
  dht.begin();
  roofServo.attach(SERVO_PIN);
  roofServo.write(90); // Khởi tạo góc mái che/rèm ở 90 độ (Đóng)

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
