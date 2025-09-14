/*
#include "sample_motion.h"
#include <Arduino.h>
#include <MeAuriga.h>

#include <Adafruit_NeoPixel.h>
#define PIN 44
#define NUMPIXELS 12
static Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// 左右のオンボード・エンコーダモータ
static MeEncoderOnBoard motorL(SLOT1);   // 左
static MeEncoderOnBoard motorR(SLOT2);   // 右

// エンコーダ割り込みサービスルーチン
static void isr_encoderL() {
    if (digitalRead(motorL.getPortB()) == 0) {
        motorL.pulsePosPlus();
    }
    else {
        motorL.pulsePosMinus();
    }
}

static void isr_encoderR() {
    if (digitalRead(motorR.getPortB()) == 0) {
        motorR.pulsePosPlus();
    }
    else {
        motorR.pulsePosMinus();
    }
}

void setupSampleMotion() {
    // ぐるぐるLEDの初期化
    pixels.begin(); pixels.setBrightness(80);

    // 割り込み設定（必須）
    attachInterrupt(motorL.getIntNum(), isr_encoderL, RISING);
    attachInterrupt(motorR.getIntNum(), isr_encoderR, RISING);

    // 速度指令（PWM指定）。向きが逆なら符号を反転してください
    motorL.setMotorPwm(-120);
    motorR.setMotorPwm(120);
    delay(1000);  // 1秒前進

    // 停止
    motorL.setMotorPwm(0);
    motorR.setMotorPwm(0);
}

void loopSampleMotion() {
    for (int i = 0; i < NUMPIXELS; i++) {
        pixels.clear();
        pixels.setPixelColor(i, pixels.Color(255, 0, 0));
        pixels.show();
        delay(100);
    }
}
*/