#include "IRFMotorDriver.h"
#include <avr/interrupt.h>

IRFMotorDriver* _irfMotorInstance = nullptr;

ISR(TIMER2_COMPA_vect) {
    if (_irfMotorInstance) {
        _irfMotorInstance->_isr();
    }
}

IRFMotorDriver::IRFMotorDriver(uint8_t pinHighA, uint8_t pinHighB, uint8_t pinLowA, uint8_t pinLowB) :
    _pinHighA(pinHighA), _pinHighB(pinHighB), _pinLowA(pinLowA), _pinLowB(pinLowB),
    _currentPower(0.0f), _isEBreak(false), _pwmPeriod(100), _pwmCounter(0), _appliedState(255)
{
}

void IRFMotorDriver::begin() {
    pinMode(_pinHighA, OUTPUT);
    pinMode(_pinHighB, OUTPUT);
    pinMode(_pinLowA,  OUTPUT);
    pinMode(_pinLowB,  OUTPUT);
    
    _irfMotorInstance = this;
    idle();
    
    // Configure Timer2 for ~10kHz interrupts
    noInterrupts();
    TCCR2A = 0;
    TCCR2B = 0;
    TCNT2  = 0;
    
    // CTC Mode
    TCCR2A |= (1 << WGM21);
    // Prescaler 32 (16MHz/32 = 500kHz timer clock)
    TCCR2B |= (1 << CS21) | (1 << CS20);
    // CompA limit (500kHz / 50 = 10kHz interrupt -> 100us)
    OCR2A = 49;
    // Enable CompA Interrupt
    TIMSK2 |= (1 << OCIE2A);
    interrupts();
}

void IRFMotorDriver::idle() {
    noInterrupts();
    _currentPower = 0.0f;
    _isEBreak = false;
    applyState(0);
    interrupts();
}

void IRFMotorDriver::eBreak() {
    noInterrupts();
    _currentPower = 0.0f;
    _isEBreak = true;
    applyState(0); // transition through idle
    applyState(3); // hard break
    interrupts();
}

void IRFMotorDriver::setPower(float p) {
    if (p < -100.0f) p = -100.0f;
    if (p > 100.0f) p = 100.0f;
    noInterrupts();
    _currentPower = p;
    _isEBreak = false;
    interrupts();
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

void IRFMotorDriver::setPwmPeriod(uint16_t periodCycles) {
    _pwmPeriod = periodCycles;
}

void IRFMotorDriver::applyState(uint8_t s) {
    if (_appliedState == s) return;
    _appliedState = s;
    if (s == 0) setPinsIdle();
    else if (s == 1) setPinsRight();
    else if (s == 2) setPinsLeft();
    else if (s == 3) setPinsEBreak();
}

void IRFMotorDriver::_isr() {
    if (_currentPower > -5 && _currentPower < 5) {
        if (_isEBreak) {
            applyState(3);
        } else {
            applyState(0);
        }
        return;
    }

    _pwmCounter++;
    if (_pwmCounter >= _pwmPeriod) {
        _pwmCounter = 0;
    }

    float p_abs = _currentPower >= 0 ? _currentPower : -_currentPower;
    uint16_t onTime = (uint16_t)((p_abs / 100.0f) * _pwmPeriod);

    if (_pwmCounter < onTime) {
        if (_currentPower > 0.0f) {
            applyState(1);
        } else {
            applyState(2);
        }
    } else {
        applyState(0);
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
