/**
 * TEST 2 SERVO 180° (SG90 / MG90S) - GPIO 4 & GPIO 5
 * - Pha 1: Ca 2 servo ve 0 -> 90 -> 180 -> 90 (dung 1s moi goc)
 * - Pha 2: Quet muot cung chieu 0 <-> 180
 * - Pha 3: Quet nguoc chieu (servo A 0->180, servo B 180->0)
 * - Lap lai. Gui "a 90" / "b 45" / "s" qua Serial de dat goc / dung quet.
 * - Chan motor con lai (GPIO 6, 7) giu LOW.
 * - OLED SSD1306 (SDA=1, SCL=2, 0x3C) hien goc 2 servo, ve o task rieng tren core 0
 *   de I2C 100kHz khong lam giat nhip quet servo.
 * env: test_servo2
 */
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#ifndef PIN_OLED_SDA
#define PIN_OLED_SDA 1
#endif
#ifndef PIN_OLED_SCL
#define PIN_OLED_SCL 2
#endif

#ifndef SERVO_PIN_A
#define SERVO_PIN_A 4
#endif
#ifndef SERVO_PIN_B
#define SERVO_PIN_B 5
#endif

#define CH_A        0
#define CH_B        1
#define LEDC_FREQ   50      // Hz
#define LEDC_BITS   14      // 14-bit duty

// 50Hz, 14-bit: period = 20ms = 16384 ticks
// 0°   -> 0.5ms pulse -> duty = 410
// 180° -> 2.4ms pulse -> duty = 1966
#define DUTY_MIN    410
#define DUTY_MAX    1966

static volatile int angleA = 90, angleB = 90;
static volatile bool autoSweep = true;
static const char* volatile phaseName = "KHOI DONG";

// --- OLED ---
static Adafruit_SSD1306 display(128, 64, &Wire, -1);
static bool oledReady = false;

void initOLED() {
  // Thu SDA=1/SCL=2 truoc, neu khong co thi thu dao chan
  int pinPairs[][2] = {{PIN_OLED_SDA, PIN_OLED_SCL}, {PIN_OLED_SCL, PIN_OLED_SDA}};
  for (auto& p : pinPairs) {
    Wire.end();
    Wire.begin(p[0], p[1], 100000);
    Wire.setTimeOut(30);
    delay(25);
    uint8_t addrs[] = {0x3C, 0x3D};
    for (uint8_t addr : addrs) {
      Wire.beginTransmission(addr);
      if (Wire.endTransmission() == 0 && display.begin(SSD1306_SWITCHCAPVCC, addr, false, false)) {
        oledReady = true;
        Serial.printf("[OLED] OK tai 0x%02X, SDA=%d, SCL=%d\n", addr, p[0], p[1]);
        return;
      }
    }
  }
  Serial.println("[OLED] KHONG tim thay man hinh tren I2C (SDA/SCL 1,2)!");
}

void drawServoBar(int y, const char* label, int pin, int angle) {
  display.setCursor(0, y);
  display.printf("%s IO%d %3d", label, pin, angle);
  display.drawCircle(display.getCursorX() + 3, y + 1, 1, SSD1306_WHITE); // ky hieu do
  int w = map(angle, 0, 180, 0, 124);
  display.drawRect(0, y + 10, 128, 7, SSD1306_WHITE);
  if (w > 0) display.fillRect(2, y + 12, w, 3, SSD1306_WHITE);
}

void oledTask(void*) {
  for (;;) {
    if (oledReady) {
      display.clearDisplay();
      display.fillRect(0, 0, 128, 11, SSD1306_WHITE);
      display.setTextSize(1);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(4, 2);
      display.print("TEST 2 SERVO 180");
      display.setTextColor(SSD1306_WHITE);
      drawServoBar(15, "A", SERVO_PIN_A, angleA);
      drawServoBar(35, "B", SERVO_PIN_B, angleB);
      display.setCursor(0, 56);
      display.print(autoSweep ? phaseName : "THU CONG (Serial)");
      display.display();
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

int angleToDuty(int angle) {
  angle = constrain(angle, 0, 180);
  return map(angle, 0, 180, DUTY_MIN, DUTY_MAX);
}

void setAngles(int a, int b, bool log = true) {
  angleA = constrain(a, 0, 180);
  angleB = constrain(b, 0, 180);
  ledcWrite(CH_A, angleToDuty(angleA));
  ledcWrite(CH_B, angleToDuty(angleB));
  if (log) Serial.printf("Servo A (GPIO%d) = %3d deg | Servo B (GPIO%d) = %3d deg\n",
                         SERVO_PIN_A, angleA, SERVO_PIN_B, angleB);
}

// Doc lenh Serial; tra ve true neu nguoi dung vua tat che do tu dong
void handleSerial() {
  static String buf = "";
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c != '\n' && c != '\r') { buf += c; continue; }
    buf.trim();
    if (buf.length() == 0) continue;
    if (buf == "s") {
      autoSweep = !autoSweep;
      Serial.printf("[CMD] Quet tu dong: %s\n", autoSweep ? "BAT" : "TAT");
    } else if (buf.startsWith("a ")) {
      autoSweep = false;
      setAngles(buf.substring(2).toInt(), angleB);
    } else if (buf.startsWith("b ")) {
      autoSweep = false;
      setAngles(angleA, buf.substring(2).toInt());
    } else {
      autoSweep = false;
      int d = buf.toInt();
      setAngles(d, d);
    }
    buf = "";
  }
}

// delay nhung van doc Serial
bool waitMs(uint32_t ms) {
  uint32_t t = millis();
  while (millis() - t < ms) {
    handleSerial();
    if (!autoSweep) return false;
    delay(5);
  }
  return true;
}

void setup() {
  Serial.begin(115200);

  // Giu cac chan motor con lai o muc LOW
  pinMode(6, OUTPUT); digitalWrite(6, LOW);
  pinMode(7, OUTPUT); digitalWrite(7, LOW);

  delay(1500);
  Serial.println("\n=== TEST 2 SERVO 180 (GPIO 4 & 5) ===");
  Serial.println("Lenh: 's' = bat/tat quet | '<goc>' = ca 2 | 'a <goc>' | 'b <goc>'");

  ledcSetup(CH_A, LEDC_FREQ, LEDC_BITS);
  ledcAttachPin(SERVO_PIN_A, CH_A);
  ledcSetup(CH_B, LEDC_FREQ, LEDC_BITS);
  ledcAttachPin(SERVO_PIN_B, CH_B);

  initOLED();
  xTaskCreatePinnedToCore(oledTask, "oled", 4096, nullptr, 1, nullptr, 0);

  setAngles(90, 90);
  delay(1000);
}

void loop() {
  handleSerial();
  if (!autoSweep) { delay(10); return; }

  // Pha 1: cac goc co dinh
  Serial.println("\n[PHA 1] Goc co dinh 0 -> 90 -> 180 -> 90");
  phaseName = "PHA 1: GOC CO DINH";
  const int steps[] = {0, 90, 180, 90};
  for (int g : steps) {
    setAngles(g, g);
    if (!waitMs(1000)) return;
  }

  // Pha 2: quet muot cung chieu
  Serial.println("\n[PHA 2] Quet muot cung chieu");
  phaseName = "PHA 2: CUNG CHIEU";
  for (int g = 90; g <= 180; g += 2) { setAngles(g, g, false); if (!waitMs(20)) return; }
  for (int g = 180; g >= 0; g -= 2)  { setAngles(g, g, false); if (!waitMs(20)) return; }
  for (int g = 0; g <= 90; g += 2)   { setAngles(g, g, false); if (!waitMs(20)) return; }
  setAngles(90, 90);

  // Pha 3: quet nguoc chieu
  Serial.println("\n[PHA 3] Quet nguoc chieu");
  phaseName = "PHA 3: NGUOC CHIEU";
  for (int g = 90; g <= 180; g += 2) { setAngles(g, 180 - g, false); if (!waitMs(20)) return; }
  for (int g = 180; g >= 0; g -= 2)  { setAngles(g, 180 - g, false); if (!waitMs(20)) return; }
  for (int g = 0; g <= 90; g += 2)   { setAngles(g, 180 - g, false); if (!waitMs(20)) return; }
  setAngles(90, 90);
  waitMs(1000);
}
