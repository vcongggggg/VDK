// ============================================================
// Smart Greenhouse Dashboard - Main JavaScript
// Nhóm 10
// ============================================================

// Kết nối tới Socket.IO Server
const socket = io();
let currentMode = 'auto';

// Cấu hình Chart.js
const ctx = document.getElementById('sensorChart').getContext('2d');
const sensorChart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [
            {
                label: 'Nhiệt độ (°C)',
                borderColor: '#ff5e57',
                backgroundColor: 'rgba(255, 94, 87, 0.1)',
                data: [],
                yAxisID: 'y',
                tension: 0.3
            },
            {
                label: 'Độ ẩm đất (%)',
                borderColor: '#3498db',
                backgroundColor: 'rgba(52, 152, 219, 0.1)',
                data: [],
                yAxisID: 'y1',
                tension: 0.3
            }
        ]
    },
    options: {
        responsive: true,
        maintainAspectRatio: false,
        scales: {
            x: { grid: { display: false } },
            y: { type: 'linear', display: true, position: 'left', title: { display: true, text: 'Nhiệt độ (°C)' } },
            y1: { type: 'linear', display: true, position: 'right', grid: { drawOnChartArea: false }, title: { display: true, text: 'Độ ẩm đất (%)' } }
        }
    }
});

// Hàm cập nhật đồ thị
function updateChart(timestamp, temp, soil) {
    if (sensorChart.data.labels.length > 20) {
        sensorChart.data.labels.shift();
        sensorChart.data.datasets[0].data.shift();
        sensorChart.data.datasets[1].data.shift();
    }
    sensorChart.data.labels.push(timestamp);
    sensorChart.data.datasets[0].data.push(temp);
    sensorChart.data.datasets[1].data.push(soil);
    sensorChart.update();
}

// Hàm ghi log vào log box
function addLog(msg, type = 'info') {
    const timeStr = new Date().toLocaleTimeString('vi-VN');
    const logBox = document.getElementById('log-box');
    const div = document.createElement('div');
    div.className = 'log-line';
    if (type === 'danger') div.className += ' log-danger';
    if (type === 'success') div.className += ' log-success';
    div.innerText = `[${timeStr}] ${msg}`;
    logBox.prepend(div);
}

// Biến chặn cảnh báo liên tục để tránh spam SweetAlert
let alertSpamShield = {
    temp: false,
    soil: false,
    light: false
};

// ──────────────────────────────────────────────────────────
// Xử lý sự kiện từ Socket.IO

// 1. Nhận cập nhật trạng thái Online/Offline của ESP32
socket.on('esp_status', (data) => {
    const badge = document.getElementById('esp-status-badge');
    if (data.online) {
        badge.className = 'status-badge status-online';
        badge.innerHTML = '<i class="bi bi-broadcast me-1"></i> ESP32: ONLINE';
        addLog("Mạch ESP32 đã kết nối vào mạng (ONLINE).", "success");
    } else {
        badge.className = 'status-badge status-offline';
        badge.innerHTML = '<i class="bi bi-broadcast me-1"></i> ESP32: OFFLINE';
        addLog("Mất kết nối với mạch ESP32 (OFFLINE)!", "danger");
        
        // Xóa trắng số hiển thị nếu offline
        document.getElementById('sensor-temp').innerText = '--';
        document.getElementById('sensor-hum').innerText = '--';
        document.getElementById('sensor-soil').innerText = '--';
        document.getElementById('sensor-light').innerText = '--';
    }
});

// 2. Nhận gói dữ liệu cảm biến thời gian thực (Telemetry)
socket.on('telemetry', (data) => {
    if (data.temperature === undefined) return;

    // Cập nhật giao diện số liệu
    document.getElementById('sensor-temp').innerText = data.temperature.toFixed(1);
    document.getElementById('sensor-hum').innerText = data.humidity.toFixed(1);
    document.getElementById('sensor-soil').innerText = data.soil_moisture.toFixed(1);
    document.getElementById('sensor-light').innerText = data.light_level.toFixed(0);

    document.getElementById('status-mode').innerText = data.mode.toUpperCase();
    document.getElementById('status-roof').innerText = data.roof_status;
    document.getElementById('status-pump').innerText = data.pump_status;
    document.getElementById('status-fan').innerText = data.fan_status;

    currentMode = data.mode;
    updateModeButtons();

    // Vẽ đồ thị
    const timeLabel = new Date().toLocaleTimeString('vi-VN');
    updateChart(timeLabel, data.temperature, data.soil_moisture);

    // --- Kiểm tra logic cảnh báo nguy cơ bất thường (Theo link ChatGPT) ---
    
    // 2.1 Cảnh báo Nhiệt độ quá cao (> 40 độ C)
    const banner = document.getElementById('system-alert-banner');
    if (data.temperature > 40) {
        banner.classList.remove('d-none');
        if (!alertSpamShield.temp) {
            alertSpamShield.temp = true;
            Swal.fire({
                icon: 'error',
                title: 'Nhiệt độ quá cao!',
                text: `Nhiệt độ đo được là ${data.temperature.toFixed(1)}°C vượt ngưỡng 40°C. Hệ thống đã kích hoạt quạt thông gió!`,
                confirmButtonColor: '#e74c3c'
            });
            addLog(`Cảnh báo: Nhiệt độ nguy hại (${data.temperature.toFixed(1)}°C)!`, 'danger');
        }
    } else {
        banner.classList.add('d-none');
        alertSpamShield.temp = false;
    }

    // 2.2 Cảnh báo Đất khô hạn (< 30%)
    if (data.soil_moisture < 30) {
        if (!alertSpamShield.soil) {
            alertSpamShield.soil = true;
            Swal.fire({
                icon: 'warning',
                title: 'Đất bị khô hạn!',
                text: `Độ ẩm đất đo được chỉ còn ${data.soil_moisture.toFixed(1)}% (< 30%). Hãy kiểm tra máy bơm!`,
                confirmButtonColor: '#f1c40f'
            });
            addLog(`Cảnh báo: Độ ẩm đất quá thấp (${data.soil_moisture.toFixed(1)}%)!`, 'danger');
        }
    } else {
        alertSpamShield.soil = false;
    }

    // 2.3 Cảnh báo Ánh sáng quá gắt (> 800 lux)
    if (data.light_level > 800) {
        if (!alertSpamShield.light) {
            alertSpamShield.light = true;
            Swal.fire({
                icon: 'info',
                title: 'Ánh sáng cực mạnh!',
                text: `Cường độ sáng đo được đạt ${data.light_level.toFixed(0)} lux. Mái che mở tự động để tản nhiệt!`,
                confirmButtonColor: '#3498db'
            });
            addLog(`Cảnh báo: Ánh nắng trực xạ cao (${data.light_level.toFixed(0)} lux).`, 'info');
        }
    } else {
        alertSpamShield.light = false;
    }

    if (data.anomalies && data.anomalies.length > 0) {
    const timeStr = new Date().toLocaleTimeString('vi-VN'); // Lấy Timestamp

    data.anomalies.forEach(anomaly => {
        // Tạo chuỗi cảnh báo
        const alertMsg = `[CẢNH BÁO] ${anomaly.sensor}: ${anomaly.detail}`;
        
        // 1. Ghi vào hộp Nhật ký hệ thống (Log box) có timestamp
        addLog(alertMsg, 'danger');

        // 2. Hiển thị Pop-up Toast góc màn hình (dùng SweetAlert2)
        // Dùng Toast để không làm phiền người dùng như Pop-up giữa màn hình
        const Toast = Swal.mixin({
            toast: true,
            position: 'top-end',
            showConfirmButton: false,
            timer: 4000,
            timerProgressBar: true,
        });

        if (anomaly.type === 'SHOCK') {
            Toast.fire({
                icon: 'error',
                title: anomaly.sensor + ' ĐỘT BIẾN!',
                text: anomaly.detail + ` (Lúc ${timeStr})`
            });
        } else if (anomaly.type === 'DRIFT') {
            Toast.fire({
                icon: 'warning',
                title: anomaly.sensor + ' TRÔI DẠT LỖI!',
                text: anomaly.detail + ` (Lúc ${timeStr})`
            });
        }
    });
}
});

// Cập nhật màu các nút chế độ
function updateModeButtons() {
    const btnAuto = document.getElementById('btn-mode-auto');
    const btnManual = document.getElementById('btn-mode-manual');
    if (currentMode === 'auto') {
        btnAuto.classList.add('active');
        btnManual.classList.remove('active');
    } else {
        btnManual.classList.add('active');
        btnAuto.classList.remove('active');
    }
}

// Thay đổi chế độ AUTO / MANUAL
function setMode(mode) {
    currentMode = mode;
    socket.emit('control', { cmd: 'set_mode', mode: mode });
    addLog(`Gửi yêu cầu đổi chế độ -> ${mode.toUpperCase()}`);
    updateModeButtons();
}

// Gửi lệnh điều khiển tay
function sendControl(action) {
    if (currentMode !== 'manual') {
        Swal.fire({
            icon: 'error',
            title: 'Hành động bị từ chối!',
            text: 'Hệ thống đang hoạt động ở chế độ AUTO. Bạn cần chuyển sang MANUAL để điều khiển thủ công!',
            confirmButtonColor: '#e74c3c'
        });
        return;
    }
    socket.emit('control', { cmd: action });
    addLog(`Gửi lệnh thủ công: ${action.toUpperCase()}`);
}

// Kéo lịch sử từ SQLite và hiển thị trong Bảng dữ liệu của Modal
async function loadHistory() {
    try {
        const response = await fetch('/api/history');
        const data = await response.json();
        
        const tbody = document.getElementById('history-table-body');
        tbody.innerHTML = ''; // Xóa dữ liệu cũ

        if (data.length === 0) {
            tbody.innerHTML = `<tr><td colspan="9" class="text-muted py-3">Không có bản ghi dữ liệu nào trong SQLite.</td></tr>`;
        } else {
            // Đảo ngược để hiển thị dòng mới nhất lên trên cùng của bảng
            const reversedData = [...data].reverse();
            reversedData.forEach(row => {
                const tr = document.createElement('tr');
                
                const modeBadge = row.mode === 'auto' 
                    ? `<span class="badge bg-success-subtle text-success border border-success">Tự động (AUTO)</span>` 
                    : `<span class="badge bg-warning-subtle text-warning border border-warning">Thủ công (MANUAL)</span>`;
                
                const roofBadge = row.roof_status.includes('MO')
                    ? `<span class="badge bg-primary-subtle text-primary border border-primary">${row.roof_status}</span>`
                    : `<span class="badge bg-secondary-subtle text-secondary border border-secondary">${row.roof_status}</span>`;
                    
                const pumpBadge = row.pump_status === 'BAT'
                    ? `<span class="badge bg-success">ĐANG BẬT</span>`
                    : `<span class="badge bg-secondary">TẮT</span>`;
                    
                const fanBadge = row.fan_status === 'BAT'
                    ? `<span class="badge bg-info text-dark">ĐANG BẬT</span>`
                    : `<span class="badge bg-secondary">TẮT</span>`;

                tr.innerHTML = `
                    <td class="fw-semibold text-dark">${row.local_time}</td>
                    <td>${modeBadge}</td>
                    <td class="fw-bold text-danger">${row.temperature.toFixed(1)}°C</td>
                    <td class="text-primary fw-semibold">${row.humidity.toFixed(0)}%</td>
                    <td class="text-info fw-semibold">${row.soil_moisture.toFixed(0)}%</td>
                    <td class="text-warning fw-semibold">${row.light_level.toFixed(0)} lux</td>
                    <td>${roofBadge}</td>
                    <td>${pumpBadge}</td>
                    <td>${fanBadge}</td>
                `;
                tbody.appendChild(tr);
            });
        }
        
        addLog(`Đã tải ${data.length} bản ghi lịch sử từ SQLite vào bảng.`, 'success');
        
        // Hiện Modal
        const modalEl = document.getElementById('historyModal');
        const modalInst = bootstrap.Modal.getInstance(modalEl) || new bootstrap.Modal(modalEl);
        modalInst.show();
    } catch (err) {
        addLog(`Lỗi tải lịch sử từ DB: ${err.message}`, 'danger');
        Swal.fire({
            icon: 'error',
            title: 'Lỗi tải lịch sử!',
            text: err.message,
            confirmButtonColor: '#e74c3c'
        });
    }
}

// Xóa sạch cơ sở dữ liệu SQLite
async function clearDbHistory() {
    const result = await Swal.fire({
        title: 'Bạn có chắc chắn?',
        text: "Hành động này sẽ xóa vĩnh viễn toàn bộ lịch sử dữ liệu cảm biến trong SQLite!",
        icon: 'warning',
        showCancelButton: true,
        confirmButtonColor: '#d33',
        cancelButtonColor: '#3085d6',
        confirmButtonText: 'Đồng ý, xóa hết!',
        cancelButtonText: 'Hủy bỏ'
    });

    if (result.isConfirmed) {
        try {
            const response = await fetch('/api/history/clear', { method: 'POST' });
            const resData = await response.json();
            if (resData.success) {
                addLog('Đã xóa sạch lịch sử cơ sở dữ liệu SQLite.', 'danger');
                Swal.fire({
                    icon: 'success',
                    title: 'Đã xóa!',
                    text: 'Toàn bộ dữ liệu lịch sử trong DB đã được dọn sạch.',
                    confirmButtonColor: '#2ecc71'
                });
                // Đóng modal và làm mới dữ liệu
                const modalEl = document.getElementById('historyModal');
                const modalInst = bootstrap.Modal.getInstance(modalEl);
                if (modalInst) modalInst.hide();
            }
        } catch (err) {
            addLog(`Lỗi xóa lịch sử DB: ${err.message}`, 'danger');
        }
    }
}
