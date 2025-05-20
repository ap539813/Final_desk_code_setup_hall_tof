// desk_control_with_tof.ino
// Main program file for desk height control with VL53L0X ToF sensor calibration

#include "config.h"
#include "hall_sensor.h"
#include "tof_sensor.h"
#include "motor_control.h"
#include "web_interface.h"
#include "storage.h"

// Global state
bool isMoving = false;
int moveTargetPulses = 0;
unsigned moveStartTime = 0;
bool abortMovement = false;
int lastStoppedPulses = 0;
unsigned long lastMotorStopTime = 0;
bool backlightOn = true;

// I²C devices
VL53L0X tof;
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== Desk Control System ===");
  
  // Initialize EEPROM
  initStorage();
  
  // Initialize I²C bus
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing");
  
  // Initialize VL53L0X sensor
  if (!initToFSensor(&tof)) {
    Serial.println("Failed to initialize VL53L0X sensor!");
    // Continue anyway, as we can fall back to hall sensors
  }
  
  // Load saved position
  loadPulseCount();

  // Initialize motor control
  initMotorControl();
  
  // Initialize Hall sensors
  initHallSensors();

  // Calibrate with distance sensor at startup
  delay(1000); // Wait for sensor to stabilize
  calibrateWithTof();

  // Show loaded height
  float initialHeight = pulsesToHeight(pulseCount);
  lastStoppedPulses = pulseCount;  // Set last stopped position to loaded value
  char buf[16];
  snprintf(buf, sizeof(buf), "%.1f cm", initialHeight);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loaded Height:");
  lcd.setCursor(0, 1);
  lcd.print(buf);
  delay(2000);
  
  // Print Hall sensor setup info
  Serial.println("Desk Control Started");
  Serial.println("Pulses: " + String(pulseCount));

  // Initialize web server
  initWebServer();
}

void loop() {
  // Handle web server
  handleWebRequests();
  
  // Check Hall sensors on every loop
  checkHallSensors();

  // LCD backlight timeout control
  if (backlightOn && !isMoving && lastMotorStopTime > 0) {
    if (millis() - lastMotorStopTime > BACKLIGHT_TIMEOUT) {
      lcd.noBacklight();  // Turn off backlight
      backlightOn = false;
    }
  }

  if (isMoving) {
    // Check if we need to update LCD
    static int lastDisplayedPulses = -999;
    static unsigned long lastLcdUpdateTime = 0;
    unsigned long currentTime = millis();
    
    // Update LCD every 500ms or if pulse count has changed significantly
    if (currentTime - lastLcdUpdateTime > 500 || abs(pulseCount - lastDisplayedPulses) >= 50) {
      float currentHeight = pulsesToHeight(pulseCount);
      float targetHeight = pulsesToHeight(moveTargetPulses);
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Moving to:");
      char buf[16];
      snprintf(buf, sizeof(buf), "%.1f/%.1f cm", currentHeight, targetHeight);
      lcd.setCursor(0, 1);
      lcd.print(buf);
      lastDisplayedPulses = pulseCount;
      lastLcdUpdateTime = currentTime;
      
      Serial.print("Pulses: ");
      Serial.print(pulseCount);
      Serial.print("/");
      Serial.print(moveTargetPulses);
      Serial.print(" (Height: ");
      Serial.print(currentHeight);
      Serial.print("/");
      Serial.print(targetHeight);
      Serial.println(" cm)");
    }

    if (abortMovement) {
      isMoving = false;
      abortMovement = false;
      stopDeskMotor();
    }
    else {
      // Check if we're close enough to target based on pulse count only
      int pulseDifference = abs(pulseCount - moveTargetPulses);
      
      if (pulseDifference <= ERROR_THRESHOLD || millis() - moveStartTime > MOVEMENT_TIMEOUT) {
        // Stop when within ERROR_THRESHOLD pulses of target or timeout
        stopDeskMotor();
        isMoving = false;
        
        Serial.println("\nStopped within range - Pulses: " + String(pulseCount) + 
                       " (target was: " + String(moveTargetPulses) + 
                       ", difference: " + String(pulseDifference) + ")");
      }
    }
  }
}

// Stops motor and performs calibration with TOF sensor
void stopDeskMotor() {
  // Use smooth stop
  smoothStopMotor();
  
  // Update direction and backlight state
  currentDir = DIR_NONE;
  lastMotorStopTime = millis();
  
  // Turn backlight on when motor stops (if it was off)
  if (!backlightOn) {
    lcd.backlight();
    backlightOn = true;
  }
  
  // Wait for motor to completely stop
  delay(200);
  
  // Calibrate hall sensor using the distance sensor
  calibrateWithTof();
  
  // Update last stopped pulse count
  lastStoppedPulses = pulseCount;
  
  // Save position to EEPROM
  savePulseCount();
  
  // Show final height based on pulse count
  float height = pulsesToHeight(pulseCount);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Height:");
  char buf[16];
  snprintf(buf, sizeof(buf), "%.1f cm", height);
  lcd.setCursor(0, 1);
  lcd.print(buf);
  
  // Print to serial
  Serial.println("Final position - Pulses: " + String(pulseCount) + ", Height: " + String(height) + " cm");
}

// Dedicated function for ToF calibration - completely separate from main loop
void calibrateWithTof() {
  // Take multiple readings and average them for better accuracy
  float measuredHeight = measureHeightWithTof(&tof);
  
  if (measuredHeight > 0) {  // Only update if we got a valid reading
    // Calculate what the pulse count should be based on the measured height
    int calculatedPulses = heightToPulses(measuredHeight);
    
    // Update pulse count with the calibrated value
    pulseCount = calculatedPulses;
    lastStoppedPulses = calculatedPulses;
    
    Serial.print("Calibrated pulse count: ");
    Serial.print(pulseCount);
    Serial.print(" (Height: ");
    Serial.print(measuredHeight);
    Serial.println(" cm)");
  }
}