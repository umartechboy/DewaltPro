#include "IRFMotorDriver.h"

IRFMotorDriver::IRFMotorDriver(uint8_t pinHighA, uint8_t pinHighB, uint8_t pinLowA, uint8_t pinLowB) :
    _pinHighA(pinHighA), _pinHighB(pinHighB), _pinLowA(pinLowA), _pinLowB(pinLowB),
    _currentPower(0.0f), _isEBreak(false), _pwmPeriod(1000), _lastPwmUpdate(0)
{
}

void IRFMotorDriver::begin() {
    pinMode(_pinHighA, OUTPUT);
    pinMode(_pinHighB, OUTPUT);
    pinMode(_pinLowA,  OUTPUT);
    pinMode(_pinLowB,  OUTPUT);
    idle();
}

void IRFMotorDriver::idle() {
    _currentPower = 0.0f;
    _isEBreak = false;
    setPinsIdle();
}

void IRFMotorDriver::eBreak() {
    _currentPower = 0.0f;
    _isEBreak = true;
    setPinsIdle();
    setPinsEBreak();
}

void IRFMotorDriver::setPower(float p) {
    if (p < -100.0f) p = -100.0f;
    if (p > 100.0f) p = 100.0f;
    _currentPower = p;
    _isEBreak = false;
}

// Backwards compatibility methods
void IRFMotorDriver::SetPower(float speed) {
    if (speed < -1.0f) speed = -1.0f;
    if (speed > 1.0f) speed = 1.0f;
    setPower(speed * 100.0f);
}

void IRFMotorDriver::HardStop() {
    eBreak();
}

float IRFMotorDriver::GetSpeed() const {
    return _currentPower / 100.0f; // Scale back to -1.0 to 1.0
}

bool IRFMotorDriver::IsHardStopped() const {
    return _isEBreak;
}

void IRFMotorDriver::setPwmPeriod(uint32_t periodMicros) {
    _pwmPeriod = periodMicros;
}

void IRFMotorDriver::loop() {
    if (_currentPower > -5 && _currentPower < 5) {
        // Not driving, hold idle or eBreak continuously
        if (_isEBreak) {
            setPinsEBreak();
        } else {
            setPinsIdle();
        }
        return;
    }

    uint32_t now = micros();
    
    // Cycle boundary check
    if (now - _lastPwmUpdate >= _pwmPeriod) {
        _lastPwmUpdate = now;
    }

    // Absolute power calculation
    float p_abs = _currentPower >= 0 ? _currentPower : -_currentPower;
    uint32_t onTime = (uint32_t)((p_abs / 100.0f) * _pwmPeriod);

    if ((now - _lastPwmUpdate) < onTime) {
        // ON period
        if (_currentPower > 0.0f) {
            setPinsRight();
        } else {
            setPinsLeft();
        }
    } else {
        // OFF period (coasting)
        setPinsIdle();
    }
}

// Low-level pin toggles replicating the original functionality.
// Writing to the outputs sequentially as original reference implementation.

void IRFMotorDriver::setPinsLeft() {
    digitalWrite(_pinHighA, HIGH);
    digitalWrite(_pinHighB, LOW);
    digitalWrite(_pinLowA,  LOW);
    digitalWrite(_pinLowB,  HIGH);
}

void IRFMotorDriver::setPinsRight() {
    digitalWrite(_pinHighB, HIGH);
    digitalWrite(_pinHighA, LOW);
    digitalWrite(_pinLowB,  LOW);
    digitalWrite(_pinLowA,  HIGH);
}

void IRFMotorDriver::setPinsIdle() {
    digitalWrite(_pinHighA, HIGH);
    digitalWrite(_pinHighB, HIGH);
    digitalWrite(_pinLowA,  LOW);
    digitalWrite(_pinLowB,  LOW);
}

void IRFMotorDriver::setPinsEBreak() {
    digitalWrite(_pinHighA, LOW);
    digitalWrite(_pinHighB, LOW);
    digitalWrite(_pinLowA,  LOW);
    digitalWrite(_pinLowB,  LOW);
}
