// sample_tag_follow.cpp（単体でも動くようにフル記載）
#include "tag_tracker.h"
#include <Arduino.h>
#include <Wire.h>
#include <HUSKYLENS.h>
#include <MeAuriga.h>

// ========== 設定 ==========
#define DEBUG 1
const int   TARGET_ID = 1;      // 追うタグの学習ID
const int   FRAME_W = 320;    // HuskyLensの横解像度
const int   CENTER_X = FRAME_W / 2;
const int   BASE_FWD = 60;     // 基本前進（0〜255）
const float KP_TURN = 0.5f;   // 向き誤差→旋回ゲイン
const float KP_DIST = 0.008f; // 面積誤差→前後ゲイン
const int   TARGET_AREA = 7000;   // 目標の見かけ面積（近さの基準）
const int   MAX_PWM = 200;    // 出力上限（0〜255）
const int   SEARCH_PWM = 80;     // 見失い時の探索速度（その場旋回）

// ========== モータ（エンコーダ） ==========
static MeEncoderOnBoard motorL(SLOT1);
static MeEncoderOnBoard motorR(SLOT2);

static void isr_encoderL() {
    if (digitalRead(motorL.getPortB()) == 0) motorL.pulsePosPlus();
    else                                      motorL.pulsePosMinus();
}

static void isr_encoderR() {
    if (digitalRead(motorR.getPortB()) == 0) motorR.pulsePosPlus();
    else                                      motorR.pulsePosMinus();
}

// Naotoさんの個体：前進は L:-speed / R:+speed
inline void setMotor(int ls, int rs) {
    ls = constrain(ls, -255, 255);
    rs = constrain(rs, -255, 255);
    motorL.setMotorPwm(ls);
    motorR.setMotorPwm(rs);
}

inline void stopMotors() { setMotor(0, 0); }

// ========== HuskyLens ==========
static HUSKYLENS husky;

// ========== ユーティリティ ==========
#if DEBUG
#define DBG(...) Serial.println(__VA_ARGS__)
#else
#define DBG(...)
#endif

void setupTagTracker() {
#if DEBUG
    Serial.begin(115200);
    delay(200);
    DBG("setup start");
#endif

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); delay(150); digitalWrite(LED_BUILTIN, LOW);

    // エンコーダ割り込み
    attachInterrupt(motorL.getIntNum(), isr_encoderL, RISING);
    attachInterrupt(motorR.getIntNum(), isr_encoderR, RISING);

    // I2C & HuskyLens
    Wire.begin();           // Port6(SDA=20,SCL=21)
    Wire.setClock(100000);
    if (!husky.begin(Wire)) {
#if DEBUG
        DBG("HUSKYLENS init failed (check power/wiring/mode)");
#endif
        // 失敗時でも動作継続（探索のみ）したいなら continue、完全停止なら無限点滅へ
        for (;;) { digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); delay(200); }
    }

#if DEBUG
    DBG("setup end");
#endif
}

void loopTagTracker() {
    bool found = false;
    HUSKYLENSResult tgt;

    // 最新データ要求
    if (husky.request()) {
        while (husky.available()) {
            HUSKYLENSResult r = husky.read();
            if (r.ID == TARGET_ID) { tgt = r; found = true; break; }
        }
    }

    if (!found) {
        // 見失い：停止
        setMotor(0, 0); // 停止
        delay(20);
        return;
    }

    // 誤差計算
    int errX = tgt.xCenter - CENTER_X;     // 中心からの横誤差（-160〜+160）
    int area = tgt.width * tgt.height;     // 見かけ面積
    int errA = TARGET_AREA - area;         // 面積誤差（＋で前進、－で減速/後退）

    // 制御量算出
    int turn = (int)(KP_TURN * errX);
    int fwd = BASE_FWD + (int)(KP_DIST * errA);

    // 左右出力（前進は L:- / R:+）
    int left = -fwd + turn;
    int right = +fwd + turn;

    left = constrain(left, -MAX_PWM, MAX_PWM);
    right = constrain(right, -MAX_PWM, MAX_PWM);

    setMotor(left, right);

#if DEBUG
    Serial.print("x=");   Serial.print(tgt.xCenter);
    Serial.print(" area="); Serial.print(area);
    Serial.print(" fwd=");  Serial.print(fwd);
    Serial.print(" turn="); Serial.print(turn);
    Serial.print(" L=");    Serial.print(left);
    Serial.print(" R=");    Serial.println(right);
#endif

    delay(20);
}
