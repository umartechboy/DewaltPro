#ifndef IRFMOTORDRIVER_H
#define IRFMOTORDRIVER_H

#include <Arduino.h>

class IRFMotorDriver {
public:
    /**
     * Constructor mapping directly to the 4 IRF MOSFET pins from original implementation:
     * @param pinHighA Pin for high-side A (IRF9540A), originally MA1
     * @param pinHighB Pin for high-side B (IRF9540B), originally MB2
     * @param pinLowA  Pin for low-side A  (IRF540A), originally MA2
     * @param pinLowB  Pin for low-side B  (IRF540B), originally MB1
     */
    IRFMotorDriver(uint8_t pinHighA, uint8_t pinHighB, uint8_t pinLowA, uint8_t pinLowB);
    
    // Call in setup() to initialize pins as outputs
    void begin();
    
    // Coast / Stop the motor
    void idle();
    
    // Hard break shorting the motor
    void eBreak();
    
    // Set power from -100.0 (Reverse/Left) to +100.0 (Forward/Right)
    void setPower(float p);
    
    // BACKWARDS COMPATIBILITY METHODS
    // Takes speed from -1.0 to 1.0 mapping to -100.0 to 100.0
    void SetPower(float speed);
    void HardStop();
    float GetSpeed() const;
    bool IsHardStopped() const;
    
    // Hardware timer now automatically generates PWM
    void loop() {}
    
    // Adjust PWM frequency if needed by setting the period (default 100 loop cycles)
    void setPwmPeriod(uint16_t periodCycles);
    
    // Internal method called by ISR
    void _isr();

private:
    uint8_t _pinHighA;
    uint8_t _pinHighB;
    uint8_t _pinLowA;
    uint8_t _pinLowB;
    
    volatile float _currentPower;
    volatile bool _isEBreak;
    
    uint16_t _pwmPeriod;
    uint16_t _pwmCounter;
    uint8_t  _appliedState;

    void applyState(uint8_t s);
    void setPinsRight();
    void setPinsLeft();
    void setPinsIdle();
    void setPinsEBreak();
};

#endif // IRFMOTORDRIVER_H
