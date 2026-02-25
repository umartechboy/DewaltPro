#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <U8g2lib.h>

class DisplayManager {
private:
    U8G2_SSD1306_128X64_NONAME_1_HW_I2C display;
    unsigned long lastUpdate;
    
    // Get text width for current font (6x10)
    int getTextWidth(const char* text) {
        return strlen(text) * 6;
    }
    
    // Center text horizontally
    int centerTextX(const char* text) {
        return (128 - getTextWidth(text)) / 2;
    }
    
    // Center text with larger font
    int centerTextXLarge(const char* text) {
        return (128 - (strlen(text) * 7)) / 2; // 7x14 font
    }
    
public:
    DisplayManager() : 
        display(U8G2_R0, U8X8_PIN_NONE),
        lastUpdate(0) 
    {}
    
    bool begin() {
        display.begin();
        display.setFont(u8g2_font_6x10_tf);
        return true;
    }
    
    void showModeTitle(const char* modeName) {
        display.firstPage();
        do {
            // Use 7x14 font for mode switch screen
            display.setFont(u8g2_font_7x14_tf);
            
            // Center horizontally and vertically
            int xPos = centerTextXLarge(modeName);
            display.setCursor(xPos, 25);
            display.print(modeName);
            
            // Underline
            int textWidth = strlen(modeName) * 7;
            display.drawHLine(xPos, 40, textWidth);
            
            // Reset font
            display.setFont(u8g2_font_6x10_tf);
            
        } while (display.nextPage());
    }
    
    void updateModeInfo(const char* modeName, uint8_t modeIndex, float motorSpeed, float sequenceProgress) {
        unsigned long now = millis();
        if (now - lastUpdate < 40) return;
        lastUpdate = now;
        
        display.firstPage();
        do {
            // SECTION 1: TITLE BAR (Top 20px)
            // White background
            display.drawBox(0, 0, 128, 20);
            display.setDrawColor(0); // Black text on white
            
            // Proper text positioning for 6x10 font
            int nameY = 13; // Baseline for 6x10 font in 20px bar
            int nameWidth = getTextWidth(modeName);
            int nameX = (128 - nameWidth) / 2;
            
            display.setCursor(nameX, nameY);
            display.print(modeName);
            
            // Mode number at right
            char modeNum[4];
            sprintf(modeNum, "%d", modeIndex + 1);
            int numWidth = getTextWidth(modeNum);
            display.setCursor(128 - numWidth - 5, nameY);
            display.print(modeNum);
            
            // Reset drawing color
            display.setDrawColor(1);
            
            // SECTION 2: MOTOR SPEED BAR (Middle 22px)
            // Draw only when motor is running (|speed| > 0.01)
            if (fabs(motorSpeed) > 0.01f) {
                int motorBarY = 25;
                int motorBarHeight = 12;
                int motorBarWidth = 116;
                if (sequenceProgress < 0) {
                    motorBarY = 35;
                    motorBarHeight = 24;
                }
                
                // Always draw center line and bar outline
                display.drawVLine(64, motorBarY - 1, motorBarHeight + 2);
                display.drawFrame(6, motorBarY, motorBarWidth, motorBarHeight);
            
                if (motorSpeed > 0.01f) {
                    // CW (right side)
                    int fillWidth = motorSpeed * (motorBarWidth/2);
                    if (fillWidth > 0) {
                        display.drawBox(64, motorBarY, fillWidth, motorBarHeight);
                    }
                } 
                else if (motorSpeed < -0.01f) {
                    // CCW (left side)
                    int fillWidth = -motorSpeed * (motorBarWidth/2);
                    if (fillWidth > 0) {
                        display.drawBox(64 - fillWidth, motorBarY, fillWidth, motorBarHeight);
                    }
                }
            }            
            // SECTION 3: SEQUENCE PROGRESS (Bottom 22px)
            int progressY = 50;
            int progressHeight = 12;
            int progressWidth = 116;
            
            if (sequenceProgress >= 0) {
                // Sequence active - show progress bar
                display.drawFrame(6, progressY, progressWidth, progressHeight);
                
                // Fill progress
                if (sequenceProgress > 0) {
                    int fill = sequenceProgress * progressWidth;
                    if (fill > 0) {
                        display.drawBox(6, progressY, fill, progressHeight);
                    }
                }
                
                // End markers
                display.drawVLine(6, progressY - 2, progressHeight + 4);
                display.drawVLine(122, progressY - 2, progressHeight + 4);
            }
            if (fabs(motorSpeed) <= 0.01f && sequenceProgress < 0) {                
                // No sequence active - show "READY"
                int textX = centerTextX("READY");
                display.setCursor(textX, progressY);
                display.print("READY");
            }
        } while (display.nextPage());
    }
    
    void showSplash() {
        display.firstPage();
        do {
            // Use larger font for splash
            display.setFont(u8g2_font_7x14_tf);
            
            // Center "DRILL"
            int drillWidth = 5 * 7;
            int drillX = (128 - drillWidth) / 2;
            display.setCursor(drillX, 20);
            display.print("DRILL");
            
            // Center "CTRL"
            int ctrlWidth = 4 * 7;
            int ctrlX = (128 - ctrlWidth) / 2;
            display.setCursor(ctrlX, 40);
            display.print("CTRL");
            
            // Version
            display.setFont(u8g2_font_5x8_tf);
            int verWidth = 4 * 5; // "v1.0" = 4 chars
            int verX = (128 - verWidth) / 2;
            display.setCursor(verX, 55);
            display.print("v1.0");
            
            // Reset font
            display.setFont(u8g2_font_6x10_tf);
            
        } while (display.nextPage());
        delay(1500);
    }
};

#endif