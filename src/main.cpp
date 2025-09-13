/*
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#define PIN 44
#define NUMPIXELS 12
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() { pixels.begin(); pixels.setBrightness(80); }
void loop() {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.clear();
    pixels.setPixelColor(i, pixels.Color(255, 0, 0));
    pixels.show();
    delay(100);
  }
}
  */

#include <Arduino.h>
#include <Wire.h>
#include <HUSKYLENS.h>
#include <MeAuriga.h>        // モーター制御用（Ranger/Auriga）

  // --- モータ設定（Rangerの左右） ---
MeDCMotor motorL(M1);        // 左モータ（向きが逆なら符号を反転してください）
MeDCMotor motorR(M2);        // 右モータ

// --- HuskyLens ---
HUSKYLENS huskylens;
const int TARGET_ID = 1;     // 追いかける学習ID（HuskyLens側でLearnしたID）
const int FRAME_W = 320;     // HuskyLensの横解像度
const int CENTER_X = FRAME_W / 2;

// --- 制御パラメータ ---
const int BASE_FWD = 60;     // 基本前進速度（0〜255）
const float KP_TURN = 0.5;   // 向き誤差に対する旋回ゲイン
const float KP_DIST = 0.004; // 大きさ（距離）ゲイン（前後調整）
const int TARGET_AREA = 7000;// 近づきすぎ/遠すぎ判定の基準（タグの面積の目安）

// モータ出力の適用（-255〜255）
void setMotor(int ls, int rs) {
    ls = constrain(ls, -255, 255);
    rs = constrain(rs, -255, 255);
    motorL.run(ls);    // MeDCMotorは正負で正転/逆転
    motorR.run(rs);
}

void setup() {
    Serial.begin(115200);
    Wire.begin();                         // Auriga Port6のI2C
    if (!huskylens.begin(Wire)) {
        Serial.println("HUSKYLENS init failed");
        while (1) { delay(100); }
    }
    Serial.println("HUSKYLENS ready (Tag Recognition)");
}

void loop() {
    // データ要求
    if (!huskylens.request()) {
        // データ取れない：探索動作（その場旋回）
        setMotor(50, -50);
        delay(50);
        return;
    }

    // 目標ID（TARGET_ID）のブロックを探す
    HUSKYLENSResult target;
    bool found = false;
    while (huskylens.available()) {
        HUSKYLENSResult r = huskylens.read();
        if (r.ID == TARGET_ID) { target = r; found = true; break; }
    }

    if (!found) {
        // 見失った：ゆっくり探索
        setMotor(40, -40);
        return;
    }

    // 位置・大きさから制御量を計算
    int errX = target.xCenter - CENTER_X;          // 画面中心との横方向誤差（-160〜+160）
    int area = target.width * target.height;       // タグの見かけ面積
    int errA = TARGET_AREA - area;                 // 目標面積との差（＋で前進、－で後退）

    // 旋回：誤差が右（正）なら右へ向く＝左車輪速↑、右車輪速↓
    int turn = (int)(KP_TURN * errX);

    // 前後速度：面積が小さい（遠い）→＋、大きい（近い）→－
    int fwd = BASE_FWD + (int)(KP_DIST * errA);

    // 合成して左右モータへ
    int left = fwd + turn;
    int right = fwd - turn;

    // 出力（上限下限）
    left = constrain(left, -200, 200);
    right = constrain(right, -200, 200);
    setMotor(left, right);

    // デバッグ表示（必要なら）
    // Serial.print("x="); Serial.print(target.xCenter);
    // Serial.print(" area="); Serial.print(area);
    // Serial.print(" fwd="); Serial.print(fwd);
    // Serial.print(" turn="); Serial.println(turn);

    delay(20);
}


