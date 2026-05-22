const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const WebSocket = require('ws');
const sqlite3 = require('sqlite3').verbose();
const path = require('path');
const AnomalyDetector = require('./AnomalyDetector');

const tempDetector  = new AnomalyDetector(30, 3, 0.5, 10);  // Nhiệt độ: sai số nhỏ (0.5 độ) đã tính
const humDetector   = new AnomalyDetector(30, 3, 1.0, 15);  // Độ ẩm khí: dao động mạnh hơn
const soilDetector  = new AnomalyDetector(30, 3, 1.0, 15);  // Độ ẩm đất
const lightDetector = new AnomalyDetector(15, 3, 50, 500);  // Ánh sáng: Cửa sổ ngắn (15), Slack cực lớn (50 lux) vì mây bay qua cũng làm đổi lux

let lastPumpStatus = "TAT";
let lastFanStatus = "TAT";
let lastRoofStatus = "DONG";

const app = express();
const server = http.createServer(app);

// 1. Khởi tạo Socket.IO cho Web Dashboard
const io = new Server(server, {
    cors: {
        origin: "*",
        methods: ["GET", "POST"]
    }
});

// 2. Khởi tạo Raw WebSocket Server cho ESP32
const wss = new WebSocket.Server({ noServer: true });

// 3. Cấu hình kết nối SQLite (Lưu file database cục bộ tại thư mục dự án)
const dbPath = path.join(__dirname, 'greenhouse.db');
const db = new sqlite3.Database(dbPath, (err) => {
    if (err) {
        console.error("Lỗi kết nối SQLite:", err.message);
    } else {
        console.log("Đã kết nối cơ sở dữ liệu SQLite thành công.");
    }
});

// Khởi tạo bảng dữ liệu SQLite
db.serialize(() => {
    db.run(`
        CREATE TABLE IF NOT EXISTS env_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            temperature REAL,
            humidity REAL,
            soil_moisture REAL,
            light_level REAL,
            roof_status TEXT,
            pump_status TEXT,
            fan_status TEXT,
            mode TEXT
        )
    `, (err) => {
        if (err) {
            console.error("Lỗi khởi tạo bảng SQLite:", err.message);
        } else {
            console.log("SQLite: Bảng env_logs đã sẵn sàng.");
        }
    });
});

app.use(express.static(path.join(__dirname, 'public')));

// API lấy lịch sử dữ liệu (phục vụ Offline Mode)
app.get('/api/history', (req, res) => {
    db.all(`
        SELECT 
            datetime(timestamp, 'localtime') as local_time, 
            temperature, 
            humidity, 
            soil_moisture, 
            light_level, 
            roof_status, 
            pump_status, 
            fan_status, 
            mode 
        FROM env_logs 
        ORDER BY id DESC 
        LIMIT 100
    `, (err, rows) => {
        if (err) {
            return res.status(500).json({ error: err.message });
        }
        res.json(rows.reverse());
    });
});

// API xóa lịch sử dữ liệu
app.post('/api/history/clear', (req, res) => {
    db.run(`DELETE FROM env_logs`, (err) => {
        if (err) {
            return res.status(500).json({ error: err.message });
        }
        res.json({ success: true, message: "Đã xóa sạch lịch sử DB." });
    });
});

// Quản lý trạng thái kết nối ESP32 (Heartbeat)
let lastEspSeen = 0;
let isEspOnline = false;
let latestTelemetry = null;

// Kiểm tra trạng thái Online/Offline của ESP32 mỗi 2 giây
setInterval(() => {
    const isCurrentlyOnline = (Date.now() - lastEspSeen < 10000); // 10 giây timeout
    if (isCurrentlyOnline !== isEspOnline) {
        isEspOnline = isCurrentlyOnline;
        console.log(`ESP32 Status Changed: ${isEspOnline ? 'ONLINE' : 'OFFLINE'}`);
        io.emit('esp_status', { online: isEspOnline });
    }
}, 2000);

// Xử lý nâng cấp HTTP lên WebSocket chỉ cho đường dẫn /ws (cho ESP32)
server.on('upgrade', (request, socket, head) => {
    const pathname = new URL(request.url, `http://${request.headers.host}`).pathname;
    if (pathname === '/ws') {
        wss.handleUpgrade(request, socket, head, (ws) => {
            wss.emit('connection', ws, request);
        });
    }
});

// --- LẮNG NGHE ESP32 (Raw WebSocket trên đường dẫn /ws) ---
wss.on('connection', (ws) => {
    console.log('ESP32 đã kết nối tới Raw WebSocket.');
    lastEspSeen = Date.now();
    isEspOnline = true;
    io.emit('esp_status', { online: true });

    ws.on('message', (message) => {
        try {
            lastEspSeen = Date.now();
            const data = JSON.parse(message);
            latestTelemetry = data;

            if (data.pump_status !== lastPumpStatus) {
            const isPumpOn = (data.pump_status === "BAT");
            soilDetector.setDeviceState(isPumpOn, 15000); // Tắt bơm xong đợi 15s cho nước ngấm
            lastPumpStatus = data.pump_status;
        }

            // Quạt thay đổi -> Ảnh hưởng Nhiệt độ & Độ ẩm khí
            if (data.fan_status !== lastFanStatus) {
                const isFanOn = (data.fan_status === "BAT");
                tempDetector.setDeviceState(isFanOn, 10000); 
                humDetector.setDeviceState(isFanOn, 10000);
                lastFanStatus = data.fan_status;
            }

            // Mái che thay đổi -> Ảnh hưởng Ánh sáng (sốc tức thời) và Nhiệt ẩm
            if (data.roof_status !== lastRoofStatus) {
                // Khác với Bơm/Quạt là chạy liên tục, Mái che chỉ mất vài giây để quay Servo thay đổi góc.
                // Do đó, ta chỉ dùng muteFor() để làm mù hệ thống trong thời gian mái đang quay và ánh sáng đang ổn định lại.
                lightDetector.muteFor(10000); // Làm mù Z-Score ánh sáng trong 10s
                tempDetector.muteFor(15000);  // Làm mù Nhiệt độ 15s
                humDetector.muteFor(15000);   // Làm mù Độ ẩm khí 15s
                
                lastRoofStatus = data.roof_status;
            }

            // --- 3. QUÉT ĐỘT BIẾN TỔNG LỰC ---
            let anomalies = [];

            if (data.temperature !== undefined) {
                const tempRes = tempDetector.detect(data.temperature);
                if (tempRes.isAnomaly) anomalies.push({ sensor: 'Nhiệt độ', type: tempRes.type, detail: tempRes.detail });
            }
            if (data.humidity !== undefined) {
                const humRes = humDetector.detect(data.humidity);
                if (humRes.isAnomaly) anomalies.push({ sensor: 'Độ ẩm khí', type: humRes.type, detail: humRes.detail });
            }
            if (data.soil_moisture !== undefined) {
                const soilRes = soilDetector.detect(data.soil_moisture);
                if (soilRes.isAnomaly) anomalies.push({ sensor: 'Độ ẩm đất', type: soilRes.type, detail: soilRes.detail });
            }
            if (data.light_level !== undefined) {
                const lightRes = lightDetector.detect(data.light_level);
                if (lightRes.isAnomaly) anomalies.push({ sensor: 'Ánh sáng', type: lightRes.type, detail: lightRes.detail });
            }

            data.anomalies = anomalies;

            // Broadcast dữ liệu cảm biến đến Dashboard qua Socket.IO
            io.emit('telemetry', data);

            // Lưu dữ liệu vào SQLite nếu là gói tin dữ liệu cảm biến thực tế
            if (data.temperature !== undefined) {
                const queryText = `
                    INSERT INTO env_logs (temperature, humidity, soil_moisture, light_level, roof_status, pump_status, fan_status, mode)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                `;
                db.run(queryText, [
                    data.temperature,
                    data.humidity,
                    data.soil_moisture,
                    data.light_level,
                    data.roof_status,
                    data.pump_status,
                    data.fan_status,
                    data.mode
                ], (err) => {
                    if (err) {
                        console.error('Lỗi khi ghi dữ liệu vào SQLite:', err.message);
                    }
                });
            }
        } catch (err) {
            console.error('Lỗi khi xử lý tin nhắn từ ESP32:', err.message);
        }
    });

    ws.on('close', () => {
        console.log('ESP32 ngắt kết nối WebSocket.');
        isEspOnline = false;
        io.emit('esp_status', { online: false });
    });
});

// --- LẮNG NGHE WEB DASHBOARD (Socket.IO) ---
io.on('connection', (socket) => {
    console.log(`Dashboard Client mới kết nối: ${socket.id}`);
    
    // Gửi trạng thái kết nối và số liệu mới nhất ngay lập tức khi mở web
    socket.emit('esp_status', { online: isEspOnline });
    if (latestTelemetry) {
        socket.emit('telemetry', latestTelemetry);
    }

    // Nhận lệnh từ Dashboard gửi xuống ESP32
    socket.on('control', (commandData) => {
        console.log('Nhận lệnh điều khiển từ Dashboard:', commandData);
        
        // Chuyển tiếp lệnh này sang ESP32 qua Raw WebSocket
        wss.clients.forEach(client => {
            if (client.readyState === WebSocket.OPEN) {
                client.send(JSON.stringify(commandData));
            }
        });
    });

    socket.on('disconnect', () => {
        console.log(`Dashboard Client ngắt kết nối: ${socket.id}`);
    });
});

const PORT = 3000;
server.listen(PORT, () => {
    console.log(`Server đang chạy tại http://localhost:${PORT}`);
});
