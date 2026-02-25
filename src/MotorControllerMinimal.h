#ifndef MOTOR_CONTROLLER_MINIMAL_H
#define MOTOR_CONTROLLER_MINIMAL_H
#define MaxPower 90
#include <Arduino.h>

// TA6586 Motor Driver Controller
// Replaces L298N - uses PWM on both control pins instead of separate enable pin
class MotorController {
private:
    uint8_t in1, in2;
    float currentSpeed;
    bool isHardStopped;
    uint32_t hardStopStart;
    
public:
    // Constructor: in1 and in2 are PWM-capable pins (TA6586 driver)
    MotorController(uint8_t pinIn1, uint8_t pinIn2)
        : in1(pinIn1), in2(pinIn2),
          currentSpeed(0), isHardStopped(false), hardStopStart(0) {}
    
    // Legacy constructor for compatibility
    MotorController(uint8_t pinIn1, uint8_t pinIn2, uint8_t pinEna)
        : in1(pinIn1), in2(pinIn2),
          currentSpeed(0), isHardStopped(false), hardStopStart(0) {
        // pinEna is ignored for TA6586
        (void)pinEna;
    }
    
    void begin() {
        pinMode(in1, OUTPUT);
        pinMode(in2, OUTPUT);
        analogWrite(in1, 0);
        analogWrite(in2, 0);
    }
    
    void SetPower(float speed) {
        speed = constrain(speed, -1.0f, 1.0f);
        speed = (MaxPower / 100.0F) * speed;  // Scale down max speed
        currentSpeed = speed;
        
        if (isHardStopped) return;
        
        uint8_t pwm = (uint8_t)(fabs(speed) * 255.0f);
        
        if (speed > 0.01f) {
            // Forward: in1 = PWM, in2 = 0
            analogWrite(in1, pwm);
            analogWrite(in2, 0);
            Serial.print(F("FORWARD: "));
            Serial.println(pwm);
        } 
        else if (speed < -0.01f) {
            // Reverse: in1 = 0, in2 = PWM
            analogWrite(in1, 0);
            analogWrite(in2, pwm);
            Serial.print(F("BACKWARD: "));
            Serial.println(pwm);
        }
        else {
            // Stop: both = 0
            analogWrite(in1, 0);
            analogWrite(in2, 0);
            Serial.println(F("Stop"));
        }
    }
    
    void HardStop() {
        isHardStopped = true;
        hardStopStart = millis();
        // TA6586 safe braking: Apply reverse polarity momentarily (50ms)
        // This creates braking effect without shorting the IC
        if (currentSpeed > 0) {
            // Was going forward, apply reverse briefly to brake
            analogWrite(in1, 0);
            analogWrite(in2, 128);
        } else if (currentSpeed < 0) {
            // Was going reverse, apply forward briefly to brake
            analogWrite(in1, 128);
            analogWrite(in2, 0);
        } else {
            // Already stopped
            analogWrite(in1, 0);
            analogWrite(in2, 0);
        }
    }
    
    void UpdateHardStop() {
        if (isHardStopped) {
            uint32_t elapsed = millis() - hardStopStart;
            if (elapsed >= 50) {  // 50ms braking duration
                isHardStopped = false;
                analogWrite(in1, 0);
                analogWrite(in2, 0);
            }
        }
    }
    
    float GetSpeed() const {
        return currentSpeed;
    }
    
    bool IsHardStopped() const {
        return isHardStopped;
    }
};

#endif