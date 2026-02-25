#include <Arduino.h>
#include <Wire.h>
#include <avr/wdt.h>
#include "MotorControllerMinimal.h"
#include "IRFMotorDriver.h"
#include "DisplayManagerSmart.h"
#include "ManualModeMinimal.h"
#include "MomentumModeMinimal.h"
#include "TapModeMinimal.h"

// Pin definitions
#define PIN_IN1 5
#define PIN_IN2 6
#define PIN_BUTTON_NEXT 2
#define PIN_BUTTON_PREV 3
#define PIN_ANALOG_KNOB A0

// Objects
MotorController motor(PIN_IN1, PIN_IN2, 0);  // Third parameter ignored for TA6586
DisplayManager display;
// In main.cpp, update your tapConfigs:
// In main.cpp, define your tap configurations with different cycle counts:

const TapConfig tapConfigs[] = {
    // Aluminum 1.5mm - 3 cycles with different parameters
    {
        "Tap Al1.5", "Al", 15, 3,  // 3 cycles
        {
            // Cycle 1: Fast approach, medium retreat
            {0.8f, 300, -0.7f, 250},
            // Cycle 2: Medium approach, slow retreat
            {0.6f, 400, -0.4f, 350},
            // Cycle 3: Slow approach, fast retreat (break chip)
            {0.4f, 500, -0.9f, 200}
        }
    },
    
    // Acrylic 2mm - 2 identical cycles
    {
        "Tap Ac2", "Ac", 20, 2,  // 2 cycles
        {
            // Both cycles same
            {0.6f, 400, -0.5f, 350},
            {0.6f, 400, -0.5f, 350}
        }
    },
    
    // Acrylic 4mm - 4 cycles with increasing force
    {
        "Tap Ac4", "Ac", 40, 4,  // 4 cycles
        {
            {0.3f, 500, -0.2f, 400},  // Gentle
            {0.4f, 450, -0.3f, 350},  // Medium
            {0.5f, 400, -0.4f, 300},  // Firm
            {0.3f, 300, -0.9f, 150}   // Break through
        }
    },
    
    // PLA 2mm - 1 cycle only
    {
        "Tap PL2", "PL", 20, 1,  // 1 cycle
        {
            {0.7f, 300, -0.6f, 250}
            // Remaining cycles in array are unused but must exist
        }
    },
    
    // PLA 4mm - 3 cycles
    {
        "Tap PL4", "PL", 40, 3,  // 3 cycles
        {
            {0.5f, 400, -0.4f, 350},
            {0.6f, 350, -0.5f, 300},
            {0.4f, 200, -0.8f, 150}
        }
    },
    
    // PLA 6mm - 2 cycles
    {
        "Tap PL6", "PL", 60, 2,  // 2 cycles
        {
            {0.3f, 600, -0.2f, 500},
            {0.4f, 400, -0.9f, 200}
        }
    }
};

// Note: We must initialize all MAX_CYCLES entries, but only the first 'numCycles' are used
// Unused entries can be zeros or whatever, they won't be accessed

// Mode instances
ManualMode manualCW("Manual CW", 1);
ManualMode manualCCW("Manual CCW", -1);
TapMode tap1(&tapConfigs[0]);
TapMode tap2(&tapConfigs[1]);
TapMode tap3(&tapConfigs[2]);
TapMode tap4(&tapConfigs[3]);
TapMode tap5(&tapConfigs[4]);
TapMode tap6(&tapConfigs[5]);
MomentumMode momentumCW("Momentum CW", 1);
MomentumMode momentumCCW("Momentum CCW", -1);

// Mode array
DrillMode* modes[] = {
    &manualCW, &manualCCW,
    &tap1, &tap2, &tap3, &tap4, &tap5, &tap6,
    &momentumCW, &momentumCCW
};

const uint8_t MODE_COUNT = sizeof(modes) / sizeof(modes[0]);
uint8_t currentMode = 0;

// Knob reading function
float readKnobFraction() {
    float adc = (float)analogRead(PIN_ANALOG_KNOB) * 5.0f / 1023.0f;
    
    if (adc <= 1.50f) return 0.0f;
    
    const float levels[] = {1.50f, 1.60f, 1.80f, 2.10f, 2.70f, 3.60f, 4.50f, 5.00f};
    
    for (int i = 1; i < 7; i++) {
        float transitionPoint = levels[i] - (levels[i] - levels[i-1]) * 0.2f;
        if (adc < transitionPoint) {
            return (float)i / 6.0f;
        }
    }
    
    if (adc >= 4.50f) return 1.0f;
    return 1.0f;
}

// Memory check (optional)
int getFreeRam() {
    extern int __heap_start, *__brkval; 
    int v; 
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

#define MA1 5
#define MA2 6
#define MB1 8
#define MB2 7

IRFMotorDriver irfMotor(MA1, MB2, MA2, MB1);

void _9540(int pin, int state){
    digitalWrite(pin, state);
}
void _540(int pin, int state){
    digitalWrite(pin, state); // invert for IRF540
}
void setup() {
    Serial.begin(115200);
    irfMotor.begin();
    return;
    delay(100);
    
    Serial.print(F("Free RAM: "));
    Serial.println(getFreeRam());
    
    // Initialize I2C
    Wire.begin();
    
    // Initialize motor
    motor.begin();
    
    // Initialize display
    if (!display.begin()) {
        Serial.println(F("Display failed"));
        while(1);
    }
    
    display.showSplash();
    
    // Initialize buttons
    pinMode(PIN_BUTTON_NEXT, INPUT_PULLUP);
    pinMode(PIN_BUTTON_PREV, INPUT_PULLUP);
    
    // Link motor to all modes
    for (uint8_t i = 0; i < MODE_COUNT; i++) {
        modes[i]->setMotor(&motor);
        modes[i]->begin();
    }
    
    // Enable watchdog timer - 1 second timeout
    wdt_enable(WDTO_250MS);
    
    Serial.print(F("Ready. Modes: "));
    Serial.println(MODE_COUNT);
}

int motorPower = 0;
void loop() {
    irfMotor.loop();
    if (Serial.available()) {
        char b = Serial.read();
        if (b == 'a'){
            motorPower += 10; if (motorPower > 100) motorPower = 100; // Cap at 100%
            irfMotor.setPower(motorPower);
            Serial.print(F("motorLeft: "));
            Serial.println(motorPower);
        }
        else if (b == 'd'){
            motorPower -= 10; if (motorPower < -100) motorPower = -100; // Cap at -100%
            irfMotor.setPower(motorPower);
            Serial.print(F("motorRight: "));
            Serial.println(motorPower);
        }
        else if (b == 's'){
            motorPower = 0;
            irfMotor.eBreak();
            Serial.println("E-break");
        }
        else {
            motorPower = 0;
            Serial.println("motorIdle");
            irfMotor.idle();
        }
    }
    return;
    // Feed the watchdog timer
    wdt_reset();
    
    static uint32_t modeDisplayUntil = 0;
    uint32_t now = millis();
    
    // Read knob
    float knob = readKnobFraction();
    
    // Button handling with edge detection
    static bool lastNext = HIGH, lastPrev = HIGH;
    bool nextPressed = (digitalRead(PIN_BUTTON_NEXT) == LOW);
    bool prevPressed = (digitalRead(PIN_BUTTON_PREV) == LOW);
    
    // Mode switching
    if ((nextPressed && !lastNext) || (prevPressed && !lastPrev)) {
        modes[currentMode]->stop();
        
        if (nextPressed && !lastNext) {
            currentMode = (currentMode + 1) % MODE_COUNT;
        }
        if (prevPressed && !lastPrev) {
            currentMode = (currentMode + MODE_COUNT - 1) % MODE_COUNT;
        }
        
        // Show centered mode title
        display.showModeTitle(modes[currentMode]->getName());
        
        // Set display timeout
        modeDisplayUntil = now + 1000;
    }
    
    lastNext = nextPressed;
    lastPrev = prevPressed;
    
    // Run current mode
    modes[currentMode]->loop(knob);
    motor.UpdateHardStop();
    
    // Get display data
    float motorSpeed = motor.GetSpeed();
    float sequenceProgress = modes[currentMode]->getSequenceProgress();
    
    // Update display
    if (now < modeDisplayUntil) {
        // Mode title display
        if (nextPressed || prevPressed) {
            // Refresh while holding
            static uint32_t lastHoldRefresh = 0;
            if (now - lastHoldRefresh > 300) {
                display.showModeTitle(modes[currentMode]->getName());
                lastHoldRefresh = now;
            }
        }
    } else {
        // Normal display
        display.updateModeInfo(
          modes[currentMode]->getName(),  // const char* modeName
          currentMode,                    // uint8_t modeIndex  
          motorSpeed,                     // float motorSpeed
          sequenceProgress                // float sequenceProgress
      );
    }
    
    // Optional: Debug output
    static uint32_t lastDebug = 0;
    if (now - lastDebug > 2000) {
        Serial.print(F("Mode: "));
        Serial.print(currentMode);
        Serial.print(F(" Speed: "));
        Serial.println(motorSpeed);
        lastDebug = now;
    }
    
    delay(10);
}