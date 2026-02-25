#ifndef MANUAL_MODE_MINIMAL_H
#define MANUAL_MODE_MINIMAL_H

#include "DrillModeMinimal.h"

class ManualMode : public DrillMode {
private:
    int8_t direction;
    
public:
    ManualMode(const char* name, int8_t dir) : DrillMode(name), direction(dir) {}
    
    void loop(float knob) override {
        if (!motor) return;
        
        if (knob > 0.01f) {
            motor->SetPower(knob * direction);
            setState(STATE_RUNNING);
        } else {
            motor->SetPower(0);
            setState(STATE_IDLE);
        }
    }
    
    void stop() override {
        if (motor) motor->SetPower(0);
        setState(STATE_IDLE);
    }
};

#endif