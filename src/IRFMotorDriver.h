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
    
    // Call this frequently in your main loop() to generate the software PWM
    void loop();
    
    // Adjust PWM frequency if needed by setting the period (default 1000us = 1kHz)
    void setPwmPeriod(uint32_t periodMicros);

private:
    uint8_t _pinHighA;
    uint8_t _pinHighB;
    uint8_t _pinLowA;
    uint8_t _pinLowB;
    
    float _currentPower;
    bool _isEBreak;
    
    uint32_t _pwmPeriod;
    uint32_t _lastPwmUpdate;

    void setPinsRight();
    void setPinsLeft();
    void setPinsIdle();
    void setPinsEBreak();
};

#endif // IRFMOTORDRIVER_H
