#ifndef MANUAL_MODE_MINIMAL_H
#define MANUAL_MODE_MINIMAL_H

#include "DrillModeMinimal.h"
#include "DataLogger.h"

class ManualMode : public DrillMode {
private:
    int8_t direction;
    uint32_t lastLoggedTime;
    uint32_t accumulatedMs;
    
public:
    ManualMode(const char* name, int8_t dir) : 
        DrillMode(name), direction(dir), 
        lastLoggedTime(0), accumulatedMs(0) {}
    
    void loop(float knob) override {
        if (!motor) return;
        
        uint32_t now = millis();
        if (knob > 0.01f) {
            if (getState() != STATE_RUNNING) {
                lastLoggedTime = now;
            } else {
                accumulatedMs += (now - lastLoggedTime);
                lastLoggedTime = now;
                
                if (accumulatedMs >= 60000) {
                    if (direction > 0) DataLogger::addManualCWSeconds(60);
                    else DataLogger::addManualCCWSeconds(60);
                    accumulatedMs -= 60000;
                }
            }
            motor->SetPower(knob * direction);
            setState(STATE_RUNNING);
        } else {
            if (getState() == STATE_RUNNING) {
                accumulatedMs += (now - lastLoggedTime);
                // We only log full 60s per requirements
                if (accumulatedMs >= 60000) {
                   uint32_t fullMinutes = accumulatedMs / 60000;
                   if (direction > 0) DataLogger::addManualCWSeconds(fullMinutes * 60);
                   else DataLogger::addManualCCWSeconds(fullMinutes * 60);
                   accumulatedMs %= 60000;
                }
            }
            motor->HardStop();
            setState(STATE_IDLE);
        }
    }
    
    void stop() override {
        if (motor) motor->SetPower(0);
        setState(STATE_IDLE);
    }
};

#endif