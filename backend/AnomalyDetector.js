// AnomalyDetector.js
class AnomalyDetector {
    constructor(windowSize = 30, zThreshold = 3, cusumSlack = 0.5, cusumThreshold = 10) {
        this.windowSize = windowSize;
        this.buffer = [];
        this.cusumHigh = 0; 
        this.cusumLow = 0;
        this.zThreshold = zThreshold;
        this.cusumSlack = cusumSlack;
        this.cusumThreshold = cusumThreshold;
        
        // CƠ CHẾ MUTE ĐỘNG
        this.isDeviceActive = false; // Trạng thái thực của Bơm/Quạt
        this.settlingUntil = 0;      // Mốc thời gian chờ môi trường ổn định sau khi tắt
    }

    // Hàm gọi khi thiết bị BẬT hoặc TẮT
    setDeviceState(isActive, settlingMs = 15000) {
        // Nếu thiết bị vừa chuyển từ BẬT sang TẮT
        if (this.isDeviceActive && !isActive) {
            // Cho môi trường thêm 1 khoảng thời gian ngắn để ổn định (vd: nước ngấm, nhiệt độ tản đều)
            this.settlingUntil = Date.now() + settlingMs;
        }
        
        this.isDeviceActive = isActive;
        this.resetCusum(); // Xóa sạch tích lũy cũ
    }

    // Thuật toán bị Mute nếu thiết bị ĐANG CHẠY hoặc ĐANG TRONG THỜI GIAN CHỜ ỔN ĐỊNH
    isMuted() {
        return this.isDeviceActive || (Date.now() < this.settlingUntil);
    }

    detect(value) {
        if (this.buffer.length < this.windowSize) {
            this.buffer.push(value);
            return { isAnomaly: false, type: 'WARM_UP', detail: 'Đang khởi tạo...' };
        }

        const sum = this.buffer.reduce((a, b) => a + b, 0);
        const mean = sum / this.windowSize;
        const variance = this.buffer.reduce((a, b) => a + Math.pow(b - mean, 2), 0) / this.windowSize;
        const stdDev = Math.sqrt(variance) || 0.001;

        const zScore = Math.abs((value - mean) / stdDev);
        const isShock = zScore > this.zThreshold;

        let isDrift = false;

        if (!this.isMuted()) {
            this.cusumHigh = Math.max(0, this.cusumHigh + (value - mean) - this.cusumSlack);
            this.cusumLow = Math.max(0, this.cusumLow + (mean - value) - this.cusumSlack);
            isDrift = (this.cusumHigh > this.cusumThreshold) || (this.cusumLow > this.cusumThreshold);

            if (isShock) {
                this.resetCusum();
                return { 
                    isAnomaly: true, 
                    type: 'SHOCK', 
                    value: value,
                    detail: `Đo được ${value.toFixed(1)}, bình thường chỉ khoảng ${mean.toFixed(1)}. Đang thay đổi quá nhanh!` 
                };
            }
            if (isDrift) {
                this.resetCusum();
                return { 
                    isAnomaly: true, 
                    type: 'DRIFT', 
                    value: value, 
                    detail: `Đo được ${value.toFixed(1)} và đang lệch dần so với mức ổn định (${mean.toFixed(1)}). Cần kiểm tra lại thiết bị!` 
                };
            }
        } else {
            this.resetCusum();
        }

        this.buffer.shift();
        this.buffer.push(value);

        let detailMsg = null;
        if (this.isDeviceActive) detailMsg = 'Bỏ qua báo động (Thiết bị đang chạy)';
        else if (Date.now() < this.settlingUntil) detailMsg = 'Bỏ qua báo động (Chờ môi trường ổn định)';

        return { isAnomaly: false, type: 'NORMAL', detail: detailMsg };
    }

    resetCusum() {
        this.cusumHigh = 0;
        this.cusumLow = 0;
    }
}
module.exports = AnomalyDetector;