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
    _currentPower(0.0f), _isEBreak(false), _timerOnTicks(0), _timerOffTicks(255), _isPwmHigh(false), _pwmPeriod(255), _appliedState(255)
{
}

void IRFMotorDriver::begin() {
    pinMode(_pinHighA, OUTPUT);
    pinMode(_pinHighB, OUTPUT);
    pinMode(_pinLowA,  OUTPUT);
    pinMode(_pinLowB,  OUTPUT);
    
    _irfMotorInstance = this;
    idle();
    
    // Configure Timer2
    noInterrupts();
    TCCR2A = 0;
    TCCR2B = 0;
    TCNT2  = 0;
    
    // Normal mode (we just let it overflow or reset TCNT2 manually)
    // Actually, CTC Mode with OCR2A dictating the bounds is best.
    TCCR2A |= (1 << WGM21); // CTC Mode
    
    // Prescaler 1024 -> 16MHz/1024 = 15.625kHz timer clock.
    // That means each tick is 64 microseconds.
    // 255 ticks = 16ms (roughly 60Hz PWM).
    // Let's use Prescaler 256 for smoother motor driving (16MHz/256 = 62.5kHz).
    // 255 ticks = 4ms (250Hz PWM).
    TCCR2B |= (1 << CS22) | (1 << CS21); 
    
    // Start with a small default OFF period
    OCR2A = 100;
    
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
    calculateTimerTicks();
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

void IRFMotorDriver::loop() {
    // Left intentionally empty. Hardware timer automatically generates PWM.
}



void IRFMotorDriver::calculateTimerTicks() {
    float p_abs = _currentPower >= 0 ? _currentPower : -_currentPower;

    // Scale 0-100% directly to 0-255 timer ticks
    uint16_t onTicks = (uint16_t)((p_abs / 100.0f) * _pwmPeriod);

    if (onTicks == 0) {
        _timerOnTicks = 0;
        _timerOffTicks = _pwmPeriod;
    } else if (onTicks >= _pwmPeriod) {
        _timerOnTicks = _pwmPeriod;
        _timerOffTicks = 0;
    } else {
        _timerOnTicks = (uint8_t)onTicks;
        _timerOffTicks = _pwmPeriod - _timerOnTicks;
    }
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
    // 1. Check Full ON / Full OFF Overrides First
    if (_isEBreak) {
        applyState(3); // E-break
        OCR2A = 255;   // Fire whenever, it doesn't matter, we're locked
        return;
    }
    
    if (_timerOnTicks == 0) {
        applyState(0); // Idle indefinitely
        OCR2A = 255;
        return;
    }
    
    if (_timerOffTicks == 0) {
        applyState(_currentPower > 0 ? 1 : 2); // Full power indefinitely
        OCR2A = 255;
        return;
    }

    // 2. Fractional PWM Toggle Logic
    if (_isPwmHigh) {
        // Currently ON. We need to turn OFF.
        applyState(0);
        OCR2A = _timerOffTicks;
        _isPwmHigh = false;
    } else {
        // Currently OFF. We need to turn ON.
        applyState(_currentPower > 0 ? 1 : 2);
        OCR2A = _timerOnTicks;
        _isPwmHigh = true;
    }
}

// Low-level pin toggles replicating the original functionality.
// Writing to the outputs sequentially as original reference implementation.

void IRFMotorDriver::setPinsRight() {
    digitalWrite(_pinHighA, HIGH);
    digitalWrite(_pinHighB, LOW);
    digitalWrite(_pinLowA,  LOW);
    digitalWrite(_pinLowB,  HIGH);
}

void IRFMotorDriver::setPinsLeft() {
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

