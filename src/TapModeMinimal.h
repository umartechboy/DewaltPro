#ifndef TAP_MODE_MINIMAL_H
#define TAP_MODE_MINIMAL_H

#include "DrillModeMinimal.h"

// Maximum number of cycles we support (can be adjusted)
#define MAX_CYCLES 5

struct TapCycle {
    float forwardSpeed;     // CW speed (0.0 to 1.0)
    unsigned long forwardTime;  // milliseconds
    float backwardSpeed;    // CCW speed (-1.0 to 0.0)
    unsigned long backwardTime; // milliseconds
};

struct TapConfig {
    const char* displayName;
    const char* material;
    uint8_t thickness;
    uint8_t numCycles;                // Actual number of cycles (1 to MAX_CYCLES)
    TapCycle cycles[MAX_CYCLES];      // Array of cycles
};

class TapMode : public DrillMode {
private:
    const TapConfig* config;
    uint8_t currentCycleIndex;        // Which cycle we're in (0-based)
    uint8_t currentStep;              // 0 = forward, 1 = backward
    bool sequenceActive;
    bool waitingForRelease;
    uint32_t stepStartTime;
    uint32_t totalSequenceTime;
    uint32_t sequenceStartTime;
    uint32_t completedTime;           // Time spent on completed steps
    
public:
    TapMode(const TapConfig* cfg) : 
        DrillMode(cfg->displayName), config(cfg),
        currentCycleIndex(0), currentStep(0), 
        sequenceActive(false), waitingForRelease(false),
        stepStartTime(0), totalSequenceTime(0), sequenceStartTime(0),
        completedTime(0)
    {
        // Calculate total sequence time
        if (config) {
            totalSequenceTime = 0;
            for (uint8_t i = 0; i < config->numCycles; i++) {
                totalSequenceTime += config->cycles[i].forwardTime;
                totalSequenceTime += config->cycles[i].backwardTime;
            }
        }
    }
    
    void begin() override {
        currentCycleIndex = 0;
        currentStep = 0;
        sequenceActive = false;
        waitingForRelease = false;
        stepStartTime = 0;
        completedTime = 0;
        setState(STATE_IDLE);
    }
    void loop(float knob) override {
        if (!motor) return;
        
        // Start sequence only if:
        // 1. Not currently running a sequence
        // 2. Not waiting for release (i.e., sequence just completed)
        // 3. Knob is pressed
        // 4. We're idle
        if (!sequenceActive && !waitingForRelease && knob > 0 && getState() == STATE_IDLE) {
            startSequence();
        }
        
        // Run active sequence
        if (sequenceActive) {
            runSequence();
        }
        
        // Handle knob release
        if (knob == 0) {
            if (sequenceActive) {
                // Knob released during sequence - hard stop and reset
                motor->HardStop();
                stopSequence();
                waitingForRelease = false;
            }
            else if (waitingForRelease) {
                // Knob was released AFTER sequence completed
                waitingForRelease = false;
                // Now we can start a new sequence on next knob press
            }
        }
        
        // REMOVED: The auto-restart when waitingForRelease && knob > 0
        // This prevents the sequence from restarting while knob is held after completion
    }
    
    void stop() override {
        if (sequenceActive) {
            motor->HardStop();
        }
        stopSequence();
        waitingForRelease = false;
    }
    
    float getSequenceProgress() const override {
        if (!sequenceActive || totalSequenceTime == 0) return -1.0f;
        
        // Calculate progress including completed steps
        uint32_t currentStepElapsed = millis() - stepStartTime;
        uint32_t currentStepTime = getCurrentStepTime();
        
        // Cap current step elapsed at step time
        if (currentStepElapsed > currentStepTime) {
            currentStepElapsed = currentStepTime;
        }
        
        uint32_t totalElapsed = completedTime + currentStepElapsed;
        float progress = totalElapsed / (float)totalSequenceTime;
        
        // Cap at 1.0
        return progress > 1.0f ? 1.0f : progress;
    }
    
    // Get current cycle number (1-based)
    uint8_t getCurrentCycle() const {
        return currentCycleIndex + 1;
    }
    
    // Get total cycles
    uint8_t getTotalCycles() const {
        return config ? config->numCycles : 0;
    }
    
    // Get step within current cycle (1 = forward, 2 = backward)
    uint8_t getCurrentStep() const {
        return currentStep + 1;
    }
    
private:
    void startSequence() {
        if (!config) return;
        
        currentCycleIndex = 0;
        currentStep = 0;
        sequenceActive = true;
        waitingForRelease = false;
        sequenceStartTime = millis();
        stepStartTime = millis();
        completedTime = 0;
        setState(STATE_RUNNING);
        
        // Start first step
        applyCurrentStep();
    }
    
    void runSequence() {
        if (!config || !sequenceActive) return;
        
        unsigned long currentTime = millis();
        unsigned long stepDuration = getCurrentStepTime();
        
        // Check if current step is complete
        if (currentTime - stepStartTime >= stepDuration) {
            // Update completed time
            completedTime += stepDuration;
            
            // Move to next step
            currentStep++;
            
            // Check if we've completed both steps of current cycle
            if (currentStep >= 2) {
                currentStep = 0;
                currentCycleIndex++;
                
                // Check if all cycles are complete
                if (currentCycleIndex >= config->numCycles) {
                    sequenceComplete();
                    return;
                }
            }
            
            // Start next step
            stepStartTime = currentTime;
            applyCurrentStep();
        }
    }
    
    void applyCurrentStep() {
        if (!motor || !config || currentCycleIndex >= config->numCycles) return;
        
        const TapCycle& cycle = config->cycles[currentCycleIndex];
        
        if (currentStep == 0) {
            // Forward step
            motor->SetPower(cycle.forwardSpeed);
        } else {
            // Backward step
            motor->SetPower(cycle.backwardSpeed);
        }
    }
    
    unsigned long getCurrentStepTime() const {
        if (!config || currentCycleIndex >= config->numCycles) return 0;
        
        const TapCycle& cycle = config->cycles[currentCycleIndex];
        
        if (currentStep == 0) {
            return cycle.forwardTime;
        } else {
            return cycle.backwardTime;
        }
    }
    
    void sequenceComplete() {
        sequenceActive = false;
        waitingForRelease = true;  // Set flag to wait for knob release
        setState(STATE_IDLE);
        if (motor) motor->HardStop();
    }
    
    void stopSequence() {
        sequenceActive = false;
        waitingForRelease = false;
        currentCycleIndex = 0;
        currentStep = 0;
        completedTime = 0;
        setState(STATE_IDLE);
        if (motor) motor->HardStop();
    }
};

#endif