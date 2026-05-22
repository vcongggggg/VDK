class AnomalyDetector {
    constructor(windowSize = 30, zThreshold = 3, cusumSlack = 0.5, cusumThreshold = 10, minStdDev = 1.0, alertCooldownMs = 30000) {
        this.windowSize = windowSize;
        this.buffer = [];
        this.cusumHigh = 0; 
        this.cusumLow = 0;
        this.zThreshold = zThreshold;
        this.cusumSlack = cusumSlack;
        this.cusumThreshold = cusumThreshold;
        this.minStdDev = minStdDev;
        this.alertCooldown = alertCooldownMs;
        this.lastAlertTime = 0;
        
        this.isDeviceActive = false; // Trạng thái thực của Bơm/Quạt
        this.settlingUntil = 0;      // Mốc thời gian chờ môi trường ổn định sau khi tắt
    }

    setDeviceState(isActive, settlingMs = 15000) {
        if (this.isDeviceActive && !isActive) {
            this.settlingUntil = Date.now() + settlingMs;
        }
        
        this.isDeviceActive = isActive;
        this.resetCusum();
    }

    muteFor(ms) {
        this.settlingUntil = Date.now() + ms;
        this.resetCusum();
    }

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
        
        const stdDev = Math.max(Math.sqrt(variance), this.minStdDev);

        const zScore = Math.abs((value - mean) / stdDev);
        const isShock = zScore > this.zThreshold;

        let isDrift = false;
        let result = { isAnomaly: false, type: 'NORMAL', detail: null };

        if (!this.isMuted()) {
            this.cusumHigh = Math.max(0, this.cusumHigh + (value - mean) - this.cusumSlack);
            this.cusumLow = Math.max(0, this.cusumLow + (mean - value) - this.cusumSlack);
            isDrift = (this.cusumHigh > this.cusumThreshold) || (this.cusumLow > this.cusumThreshold);

            const now = Date.now();
            if (isShock) {
                this.resetCusum();
                if (now - this.lastAlertTime > this.alertCooldown) {
                    this.lastAlertTime = now;
                    result = { 
                        isAnomaly: true, 
                        type: 'SHOCK', 
                        value: value,
                        detail: `Đo được ${value.toFixed(1)}, bình thường chỉ khoảng ${mean.toFixed(1)}. Đang thay đổi quá nhanh!` 
                    };
                }
            } else if (isDrift) {
                this.resetCusum();
                if (now - this.lastAlertTime > this.alertCooldown) {
                    this.lastAlertTime = now;
                    result = { 
                        isAnomaly: true, 
                        type: 'DRIFT', 
                        value: value, 
                        detail: `Đo được ${value.toFixed(1)} và đang lệch dần so với mức ổn định (${mean.toFixed(1)}). Cần kiểm tra lại thiết bị!` 
                    };
                }
            }
        } else {
            this.resetCusum();
        }

        this.buffer.shift();
        this.buffer.push(value);

        if (!result.isAnomaly) {
            let detailMsg = null;
            if (this.isDeviceActive) detailMsg = 'Bỏ qua báo động (Thiết bị đang chạy)';
            else if (Date.now() < this.settlingUntil) detailMsg = 'Bỏ qua báo động (Chờ môi trường ổn định)';
            result.detail = detailMsg;
        }

        return result;
    }

    resetCusum() {
        this.cusumHigh = 0;
        this.cusumLow = 0;
    }
}
module.exports = AnomalyDetector;