#ifndef MOMENTUM_MODE_MINIMAL_H
#define MOMENTUM_MODE_MINIMAL_H

#include "DrillModeMinimal.h"

class MomentumMode : public DrillMode {
private:
    int8_t direction;
    float currentSpeed;
    float targetSpeed;
    uint32_t lastUpdate;
    
public:
    MomentumMode(const char* name, int8_t dir) : 
        DrillMode(name), direction(dir),
        currentSpeed(0), targetSpeed(0), lastUpdate(0) {}
    
    void begin() override {
        currentSpeed = 0;
        targetSpeed = 0;
        lastUpdate = millis();
        setState(STATE_IDLE);
    }
    
    void loop(float knob) override {
        if (!motor) return;
        
        targetSpeed = knob * direction;
        uint32_t now = millis();
        
        if (now - lastUpdate > 10) {
            float dt = (now - lastUpdate) / 1000.0f;
            lastUpdate = now;
            float inc = 2.0F;
            float dec = 0.3F;
            if (direction == -1)
                inc = 0.3F, dec = 2.0F;
            if (targetSpeed > currentSpeed) {
                currentSpeed += inc * dt;
                if (currentSpeed > targetSpeed) currentSpeed = targetSpeed;
            } else if (targetSpeed < currentSpeed) {
                currentSpeed -= dec * dt;
                if (currentSpeed < targetSpeed) currentSpeed = targetSpeed;
            }
            
            motor->SetPower(currentSpeed);
            
            if (fabs(currentSpeed) > 0.01f) {
                setState(STATE_RUNNING);
            } else {
                setState(STATE_IDLE);
            }
        }
    }
    
    void stop() override {
        targetSpeed = 0;
        setState(STATE_IDLE);
        if (motor) motor->SetPower(0);
    }
};

#endif