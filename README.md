# XIAOZHI AI VOICE LIVE - ESP32-S3 CUSTOM PCB ROBOT
### Hệ Thống Robot Trợ Lý AI Lắng Nghe Trực Tiếp Thời Gian Thực (Siri-like Always-Listening)

Dự án phát triển phần mềm Trợ lý AI đàm thoại trực tiếp (Voice Live Assistant) cho Robot Để Bàn Xiaozhi sử dụng vi điều khiển **ESP32-S3**, màn hình OLED I2C, hệ thống âm thanh I2S Full-Duplex (Micro INMP441 + Loa MAX98357A 3W) và động cơ bánh xe Mini L298N.

> 📘 **TÀI LIỆU KỸ THUẬT**:  
> Xem chi tiết tại file [HUONG_DAN_KY_THUAT.md](HUONG_DAN_KY_THUAT.md).  
> Xem chi tiết sơ đồ chân và kinh nghiệm phần cứng tại [HUONG_DAN_SO_DO_CHAN_VA_LUU_Y.md](file:///d:/TRIAD%20PROJECT/Projects/esp32s3_xiaozhi_live/HUONG_DAN_SO_DO_CHAN_VA_LUU_Y.md).

---

## 📌 Bảng Tóm Tắt Phần Cứng & Sơ Đồ Chân (Exact Pinout)

> ⚠️ **LƯU Ý ĐẶC BIỆT**: Bo mạch này **HOÀN TOÀN KHÔNG CÓ CẢM BIẾN LASER TOF (VL53L0X)**, không có Servo, không có IMU. Hệ thống chỉ gồm đúng các linh kiện bên dưới:

| Khối linh kiện | Module | Chân Module | Chân ESP32-S3 | Ghi chú kỹ thuật |
| :--- | :--- | :--- | :--- | :--- |
| **Màn hình OLED** | **SSD1306 1.3" / 0.96"** | **SDA** | **GPIO 1** | I2C Data (Địa chỉ `0x3C`, 100 kHz) |
| | | **SCL** | **GPIO 2** | I2C Clock |
| **Loa (Amp 3W)** | **MAX98357A I2S** | **BCLK** | **GPIO 11** | Bit Clock (Dùng chung với Mic) |
| | | **LRC / WS** | **GPIO 12** | Word Select (Dùng chung với Mic) |
| | | **DIN** | **GPIO 13** | Data In từ ESP32 (`DOUT`) |
| | | **SD_MODE** | **3.3V / VCC** | **BẮT BUỘC ĐÓNG JUMPER JP1** |
| **Microphone** | **INMP441 MEMS I2S** | **SCK / BCLK**| **GPIO 11** | Bit Clock (Dùng chung với Loa) |
| | | **WS** | **GPIO 12** | Word Select (Dùng chung với Loa) |
| | | **SD** | **GPIO 14** | Data Out vào ESP32 (`DIN`) |
| | | **L/R** | **GND** | Kênh trái (Left Channel) |
| **Động cơ bánh xe** | **Mini L298N (Cầu H)** | **IN1 / IN2** | **GPIO 4 / GPIO 5** | Motor A (Bánh trái) |
| | | **IN3 / IN4** | **GPIO 6 / GPIO 7** | Motor B (Bánh phải) |
| **Nút bấm người dùng**| **BOOT Button** | **KEY / IO0** | **GPIO 0** | Nút bấm vật lý (Tích cực mức LOW) |
| **Đèn LED RGB** | **WS2812** | **DI** | **GPIO 48** | LED hiển thị trạng thái Onboard |

---

## ⚡ Các Điểm Lưu Ý Phần Cứng Sống Còn

1. **Loa MAX98357A**: Chân `SD_MODE` có mạch trở 10k tụ 100uF xuống GND $\rightarrow$ **Bắt buộc đóng Jumper JP1** kéo SD lên 3.3V thì loa mới có tiếng.
2. **Màn hình OLED**: Chuẩn I2C là `SDA = GPIO 1, SCL = GPIO 2`, địa chỉ `0x3C`.
3. **Micro INMP441**: Sử dụng **Bộ lọc DC Blocker (High-Pass 20Hz)** và **Auto-Gain Normalization** (đã viết sẵn mẫu hoàn chỉnh trong `src/voice_parrot.cpp`).
4. **Cấu hình ESP32-S3 Clone**: Dùng flash mode `dio`, memory type `qio_qspi`, **tuyệt đối không bật `-DBOARD_HAS_PSRAM`**.

---

## 🚀 Các Môi Trường Firmware Có Sẵn

- **`[env:voice_parrot]` (Mặc định)**: Firmware Robot Nhại Giọng Thông Minh. Tự động nhận diện tiếng nói (VAD), tự động khuếch đại âm lượng, 3 chế độ giọng (Talking Tom x1.35, Chuẩn x1.0, Robot trầm x0.8), cử động khuôn miệng OLED và đèn trạng thái RGB.
- **`[env:test_pcb_all]`**: Bộ kiểm tra liên hoàn toàn bộ phần cứng mới (Màn hình OLED, Nhạc Mario, Động cơ A/B tiến lùi xoay, Quét tần số + Mic VU Meter, RoboEyes).
- **`[env:test_speaker]`**: Test riêng biệt loa MAX98357A.

---

## 💻 Lệnh Nạp Code Nhanh (PlatformIO CLI)

```bash
# Nạp Robot Nhại Giọng (Voice Parrot)
pio run -e voice_parrot --target upload

# Nạp Bộ Test Toàn Diện Mạch (Test All PCB)
pio run -e test_pcb_all --target upload

# Mở Serial Monitor xem log ở baudrate 115200
pio device monitor -b 115200
```
