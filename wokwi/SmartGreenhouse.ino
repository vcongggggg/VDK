#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <ESP32Servo.h>

const char* ssid     = "KTKH P208 C";
const char* password = "DUTITF2005";
const char* ws_host  = "192.168.1.8";
const int   ws_port  = 3000;

#define DHTPIN 13
#define DHTTYPE DHT11

#define SOIL_PIN 2
#define LDR_PIN 1
#define SERVO_PIN 14
#define PUMP_RELAY_PIN 18
#define FAN_RELAY_PIN 17


DHT dht(DHTPIN, DHTTYPE);
Servo roofServo;
WebSocketsClient webSocket;

bool isAutoMode = true;

bool pumpState = false;
bool fanState = false;
bool roofState = false;

bool lastPumpState = false;
bool lastFanState = false;
bool lastRoofState = false;

// Bộ lọc nhiễu số (EMA Filter)
float filteredSoil = 4095.0;
float filteredLight = 4095.0;
const float emaAlpha = 0.15;

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 2000;

float currentTemp = 0.0;
float currentHum = 0.0;
float currentSoilMoisture = 0.0;
float currentLightLevel = 0.0;

void updateActuators() {
  if (pumpState != lastPumpState) {
    digitalWrite(PUMP_RELAY_PIN, pumpState ? HIGH : LOW);
    lastPumpState = pumpState;
    Serial.printf("[HÀNH ĐỘNG] Máy bơm -> %s\n", pumpState ? "BẬT" : "TẮT");
  }
  
  if (fanState != lastFanState) {
    digitalWrite(FAN_RELAY_PIN, fanState ? HIGH : LOW);
    lastFanState = fanState;
    Serial.printf("[HÀNH ĐỘNG] Quạt tản nhiệt -> %s\n", fanState ? "BẬT" : "TẮT");
  }
  
  if (roofState != lastRoofState) {
    roofServo.write(roofState ? 45 : 90);
    lastRoofState = roofState;
    Serial.printf("[HÀNH ĐỘNG] Mái che -> %s\n", roofState ? "MỞ" : "ĐÓNG");
  }
}

void handleGreenhouseLogic() {
  int rawSoil = analogRead(SOIL_PIN);
  int rawLight = analogRead(LDR_PIN);

  filteredSoil = (emaAlpha * rawSoil) + ((1.0 - emaAlpha) * filteredSoil);
  filteredLight = (emaAlpha * rawLight) + ((1.0 - emaAlpha) * filteredLight);

  currentSoilMoisture = map((int)filteredSoil, 4095, 0, 0, 100);
  if (currentSoilMoisture < 0) currentSoilMoisture = 0;
  if (currentSoilMoisture > 100) currentSoilMoisture = 100;

  currentLightLevel = map((int)filteredLight, 4095, 0, 0, 1000);
  if (currentLightLevel < 0) currentLightLevel = 0;

  unsigned long now = millis();
  if (now - lastSendTime >= sendInterval) {
    lastSendTime = now;

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    
    if (!isnan(temp) && !isnan(hum)) {
      currentTemp = temp;
      currentHum = hum;
    } else {
      Serial.println("Lỗi: Không đọc được dữ liệu từ cảm biến DHT11!");
    }

    if (isAutoMode) {
      if (currentLightLevel > 400.0) {
        roofState = true;
      } else if (currentLightLevel < 300.0) {
        roofState = false;
      }

      if (currentSoilMoisture < 30.0 && currentLightLevel < 800.0) {
        pumpState = true;
      } else if (currentSoilMoisture > 45.0 || currentLightLevel >= 800.0) {
        pumpState = false;
      }

      if (currentTemp > 40.0) {
        fanState = true;
      } else if (currentTemp < 35.0) {
        fanState = false;
      }
    }

    updateActuators();

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

  if (!isAutoMode) {
    updateActuators();
  }
}

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
      
      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, payload, length);
      if (error) {
        Serial.print("Lỗi parse JSON: ");
        Serial.println(error.c_str());
        return;
      }

      if (doc.containsKey("cmd") && doc["cmd"] == "set_mode") {
        String mode = doc["mode"];
        isAutoMode = (mode == "auto");
        Serial.printf("Đã chuyển chế độ: %s\n", isAutoMode ? "AUTO" : "MANUAL");
      }

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

void setup() {
  Serial.begin(115200);

  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(FAN_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, LOW);
  digitalWrite(FAN_RELAY_PIN, LOW);

  dht.begin();
  roofServo.attach(SERVO_PIN);
  roofServo.write(90);

  WiFi.begin(ssid, password);
  Serial.print("Đang kết nối WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nĐã kết nối WiFi thành công!");
  Serial.print("Địa chỉ IP của ESP32: ");
  Serial.println(WiFi.localIP());

  webSocket.begin(ws_host, ws_port, "/ws");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

void loop() {
  webSocket.loop();
  handleGreenhouseLogic();
  delay(50);
}
