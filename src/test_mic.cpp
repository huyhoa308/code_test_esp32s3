/**
 * TEST MIC INMP441 (I2S RX) - BCLK=IO11, WS=IO12, SD=IO14
 * - Moi 200ms in thong ke cho ca 2 kenh L/R: so mau 0, so mau -1, raw min/max,
 *   RMS (sau DC Blocker, >>14 nhu voice_parrot) va peak.
 * - Chan doan: toan 0 / toan -1 / khong doi -> mic khong co tin hieu.
 * - OLED (SDA=1, SCL=2) hien VU meter.
 * env: test_mic
 */
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/i2s.h>
#include <math.h>

#ifndef PIN_I2S_BCLK
#define PIN_I2S_BCLK 11
#endif
#ifndef PIN_I2S_WS
#define PIN_I2S_WS   12
#endif
#ifndef PIN_I2S_DIN
#define PIN_I2S_DIN  14
#endif
#ifndef PIN_OLED_SDA
#define PIN_OLED_SDA 1
#endif
#ifndef PIN_OLED_SCL
#define PIN_OLED_SCL 2
#endif

#define I2S_PORT    I2S_NUM_0
#define SAMPLE_RATE 16000
#define CHUNK       256

static Adafruit_SSD1306 display(128, 64, &Wire, -1);
static bool oledReady = false;

struct ChanStats {
  int zeros = 0, ones = 0, n = 0;
  int32_t rawMin = INT32_MAX, rawMax = INT32_MIN;
  double sumSq = 0;
  int32_t peak = 0;
  float dcX = 0, dcY = 0;
};

static ChanStats st[2];

void initOLED() {
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL, 100000);
  Wire.setTimeOut(30);
  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() == 0 && display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false)) {
    oledReady = true;
  }
  Serial.printf("[OLED] %s\n", oledReady ? "OK 0x3C" : "khong tim thay");
}

void initMic() {
  i2s_config_t cfg = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 6,
      .dma_buf_len = CHUNK,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0
  };
  i2s_pin_config_t pins = {
      .mck_io_num = I2S_PIN_NO_CHANGE,
      .bck_io_num = PIN_I2S_BCLK,
      .ws_io_num = PIN_I2S_WS,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = PIN_I2S_DIN
  };
  esp_err_t err = i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
  if (err == ESP_OK) err = i2s_set_pin(I2S_PORT, &pins);
  Serial.printf("[I2S] Khoi tao mic: %s (%d)\n", err == ESP_OK ? "OK" : "LOI", err);
}

void drawVU(float rms, int32_t peak) {
  if (!oledReady) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("TEST MIC INMP441");
  display.setCursor(0, 16);
  display.printf("RMS : %6.0f", rms);
  display.setCursor(0, 28);
  display.printf("PEAK: %6ld", (long)peak);
  // Thang log: 30 -> 10000
  float db = (rms > 1) ? 20.0f * log10f(rms) : 0;
  int w = constrain((int)map((long)(db * 10), 300, 800, 0, 124), 0, 124);
  display.drawRect(0, 44, 128, 14, SSD1306_WHITE);
  if (w > 0) display.fillRect(2, 46, w, 10, SSD1306_WHITE);
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n=== TEST MIC INMP441 (BCLK=11, WS=12, SD=14) ===");
  initOLED();
  initMic();
}

void loop() {
  static int32_t buf[CHUNK * 2];
  size_t bytes = 0;
  i2s_read(I2S_PORT, buf, sizeof(buf), &bytes, pdMS_TO_TICKS(100));
  int frames = bytes / (2 * sizeof(int32_t));

  for (int i = 0; i < frames; i++) {
    for (int c = 0; c < 2; c++) {
      int32_t raw = buf[i * 2 + c];
      ChanStats& s = st[c];
      if (raw == 0) s.zeros++;
      if (raw == -1) s.ones++;
      if (raw < s.rawMin) s.rawMin = raw;
      if (raw > s.rawMax) s.rawMax = raw;
      float x = (float)(raw >> 14);
      float y = x - s.dcX + 0.995f * s.dcY;
      s.dcX = x; s.dcY = y;
      s.sumSq += (double)y * y;
      int32_t a = (int32_t)fabsf(y);
      if (a > s.peak) s.peak = a;
      s.n++;
    }
  }

  static uint32_t last = 0;
  static uint32_t lastOled = 0;
  static int report = 0;
  if (millis() - last >= 200 && st[0].n > 0) {
    last = millis();
    float rms[2];
    for (int c = 0; c < 2; c++) rms[c] = sqrtf(st[c].sumSq / st[c].n);
    int act = (rms[0] >= rms[1]) ? 0 : 1;   // kenh co tin hieu

    if (report++ % 10 == 0) {
      Serial.println("ch |  n   zero  -1   rawMin      rawMax      | RMS     PEAK");
    }
    for (int c = 0; c < 2; c++) {
      ChanStats& s = st[c];
      Serial.printf("%s  | %4d %4d %4d %11ld %11ld | %7.0f %6ld\n", c == 0 ? "R" : "L",
                    s.n, s.zeros, s.ones, (long)s.rawMin, (long)s.rawMax, rms[c], (long)s.peak);
    }
    const char* verdict;
    ChanStats& a = st[act];
    if (a.zeros == a.n)            verdict = "TOAN 0 -> SD khong co du lieu / mic khong nguon";
    else if (a.ones == a.n)        verdict = "TOAN -1 -> SD bi keo len cao / chan dao";
    else if (a.rawMin == a.rawMax) verdict = "KHONG DOI -> mic treo";
    // INMP441 that: nhieu nen phong yen tinh da cho raw ~ +-1e5..1e6 (24-bit can trai)
    else if (max(-(int64_t)a.rawMin, (int64_t)a.rawMax) < (1 << 17))
                                   verdict = "QUA NHO -> chi nhieu bit thap, mic KHONG gui du lieu that";
    else                           verdict = "CO TIN HIEU";
    Serial.printf("=> kenh %s: %s\n", act == 0 ? "R" : "L", verdict);

    if (millis() - lastOled >= 100) {
      lastOled = millis();
      drawVU(rms[act], a.peak);
    }
    for (int c = 0; c < 2; c++) { float x = st[c].dcX, y = st[c].dcY; st[c] = ChanStats(); st[c].dcX = x; st[c].dcY = y; }
  }
}
