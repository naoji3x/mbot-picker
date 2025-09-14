#include "tag_tracker.h"
#include <Arduino.h>

void setup() {
    setupTagTracker();
    noTone(45);  // ブザーを止める
}
void loop() {
    loopTagTracker();
}
