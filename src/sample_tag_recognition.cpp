// HuskyLens Tag(ID=1) 検出でリングLEDをぐるぐる、見失ったら停止
#include "sample_tag_recognition.h"
#include <Arduino.h>
#include <Wire.h>
#include <HUSKYLENS.h>
#include <Adafruit_NeoPixel.h>

// --- LEDリング設定（Auriga内蔵） ---
#define RING_PIN   44
#define RING_NUM   12
static Adafruit_NeoPixel ring(RING_NUM, RING_PIN, NEO_GRB + NEO_KHZ800);

// --- HuskyLens ---
static HUSKYLENS husky;
const int TARGET_ID = 1;        // 追従する/検出するタグID

// --- アニメーション制御 ---
static uint8_t spinnerIndex = 0;
static bool isShowing = false;         // 今アニメーション表示中か
static unsigned long lastAnimMs = 0;
const uint16_t animIntervalMs = 60;  // ぐるぐる更新間隔

// シンプルな“ぐるぐる”エフェクト（1ドット＋尾）
static void showSpinner(uint8_t idx) {
    ring.clear();
    // コメット風に3ドット
    ring.setPixelColor(idx % RING_NUM, ring.Color(255, 40, 0));   // 先頭（明）
    ring.setPixelColor((idx + RING_NUM - 1) % RING_NUM, ring.Color(120, 10, 0));
    ring.setPixelColor((idx + RING_NUM - 2) % RING_NUM, ring.Color(40, 0, 0));
    ring.show();
}

static void stopSpinner() {
    ring.clear();
    ring.show();
}

static void i2cScan() {
    Serial.println("[I2C] scanning...");
    byte count = 0;
    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.print("  found device at 0x"); Serial.println(addr, HEX);
            count++;
        }
    }
    if (count == 0) Serial.println("  no I2C devices found");
}

void setupSampleTagRecognition() {
    Serial.begin(115200);
    delay(200);
    Serial.println("setup start");

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); delay(150); digitalWrite(LED_BUILTIN, LOW);

    ring.begin();
    ring.setBrightness(90);
    ring.clear();
    ring.show();

    Wire.begin();              // AurigaのI2C (SDA=20, SCL=21 → Port6)
    Wire.setClock(100000);     // まずは標準100kHz

    // まずI2Cに何が見えているか確認
    i2cScan();                 // ★ ここで 0x32(=50) が見えるのが理想（HuskyLensの既定）

    // HuskyLens初期化（タイムアウト付きで3秒だけトライ）
    const unsigned long TOUT = 3000;
    unsigned long t0 = millis();
    bool ok = false;
    while (millis() - t0 < TOUT) {
        if (husky.begin(Wire)) { ok = true; break; }
        delay(100);
    }

    if (!ok) {
        Serial.println("husky.begin() FAILED. Check power/wiring/mode.");
        // ここから先もコードは続行（LEDぐるぐるはしない）
    }
    else {
        Serial.println("husky.begin() OK");
    }

    Serial.println("setup end");
}

void loopSampleTagRecognition() {
    // HuskyLensに最新データを要求
    bool found = false;
    if (husky.request()) {
        while (husky.available()) {
            HUSKYLENSResult r = husky.read();
            if (r.ID == TARGET_ID) {
                found = true;
                break;
            }
        }
    }

    // 見つかったらぐるぐる、見失ったら停止
    unsigned long now = millis();
    if (found) {
        isShowing = true;
        if (now - lastAnimMs >= animIntervalMs) {
            lastAnimMs = now;
            showSpinner(spinnerIndex++);
        }
    }
    else {
        if (isShowing) { // 直前まで表示していたら消灯
            stopSpinner();
            isShowing = false;
        }
        // 何も表示しない（消灯のまま）
    }

    // ループを軽く
    delay(5);
}
