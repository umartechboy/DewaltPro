#ifndef DRILL_MODE_MINIMAL_H
#define DRILL_MODE_MINIMAL_H

#include <Arduino.h>
#include "IRFMotorDriver.h"

class DrillMode {
protected:
    const char* name;
    uint8_t state;
    IRFMotorDriver* motor;
    
public:
    static const uint8_t STATE_IDLE = 0;
    static const uint8_t STATE_RUNNING = 1;
    
    DrillMode(const char* n) : name(n), state(STATE_IDLE), motor(nullptr) {}
    
    virtual void begin() {}
    virtual void loop(float knob) = 0;
    virtual void stop() {}
    
    virtual float getSequenceProgress() const {
        return -1.0f; // Default: no sequence
    }
    
    void setMotor(IRFMotorDriver* m) { motor = m; }
    const char* getName() const { return name; }
    uint8_t getState() const { return state; }
    
protected:
    IRFMotorDriver* getMotor() { return motor; }
    void setState(uint8_t s) { state = s; }
};

#endif