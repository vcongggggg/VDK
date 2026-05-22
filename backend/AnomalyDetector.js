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
        
        // BIẾN NGỮ CẢNH: Lưu thời điểm kết thúc thời gian tạm ngưng báo động
        this.muteUntil = 0; 
    }

    // Hàm gọi khi thiết bị (Bơm/Quạt) thay đổi trạng thái
    muteFor(milliseconds) {
        this.muteUntil = Date.now() + milliseconds;
        this.resetCusum(); // Xóa tích lũy cũ vì môi trường sắp thay đổi mạnh
    }

    // Kiểm tra xem thuật toán có đang bị "bịt mắt" không
    isMuted() {
        return Date.now() < this.muteUntil;
    }

    detect(value) {
        // 1. Giai đoạn làm nóng
        if (this.buffer.length < this.windowSize) {
            this.buffer.push(value);
            return { isAnomaly: false, type: 'WARM_UP', detail: 'Đang khởi tạo...' };
        }

        // 2. Tính toán
        const sum = this.buffer.reduce((a, b) => a + b, 0);
        const mean = sum / this.windowSize;
        const variance = this.buffer.reduce((a, b) => a + Math.pow(b - mean, 2), 0) / this.windowSize;
        const stdDev = Math.sqrt(variance) || 0.001;

        const zScore = Math.abs((value - mean) / stdDev);
        const isShock = zScore > this.zThreshold;

        let isDrift = false;

        // 3. CHỈ KIỂM TRA BẤT THƯỜNG NẾU KHÔNG BỊ MUTE (Thiết bị đang không can thiệp)
        if (!this.isMuted()) {
            this.cusumHigh = Math.max(0, this.cusumHigh + (value - mean) - this.cusumSlack);
            this.cusumLow = Math.max(0, this.cusumLow + (mean - value) - this.cusumSlack);
            isDrift = (this.cusumHigh > this.cusumThreshold) || (this.cusumLow > this.cusumThreshold);

            if (isShock) {
                this.resetCusum();
                return { isAnomaly: true, type: 'SHOCK', detail: `Sốc đột biến! (Z-Score: ${zScore.toFixed(1)})` };
            }
            if (isDrift) {
                this.resetCusum();
                return { isAnomaly: true, type: 'DRIFT', detail: `Trôi dạt dữ liệu bất thường!` };
            }
        } else {
            // Nếu đang MUTE, reset CUSUM để không cộng dồn sai số do thiết bị gây ra
            this.resetCusum();
        }

        // 4. Luôn trượt cửa sổ để cập nhật dữ liệu bình thường mới (dù có Mute hay không)
        this.buffer.shift();
        this.buffer.push(value);

        return { 
            isAnomaly: false, 
            type: 'NORMAL', 
            detail: this.isMuted() ? 'Đang bỏ qua báo động do thiết bị hoạt động' : null 
        };
    }

    resetCusum() {
        this.cusumHigh = 0;
        this.cusumLow = 0;
    }
}
module.exports = AnomalyDetector;