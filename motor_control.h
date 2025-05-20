// motor_control.h
// Motor control module for desk movement

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include "config.h"

// Direction tracking
int8_t currentDir = DIR_NONE;

// Initialize motor control
void initMotorControl() {
  // Motor pins & PWM
  pinMode(R_EN_PIN, OUTPUT); digitalWrite(R_EN_PIN, HIGH);
  pinMode(L_EN_PIN, OUTPUT); digitalWrite(L_EN_PIN, HIGH);
  ledcAttach(R_PWM_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(L_PWM_PIN, PWM_FREQ, PWM_RES);
  
  // Ensure motors are stopped
  ledcWrite(R_PWM_PIN, 0);
  ledcWrite(L_PWM_PIN, 0);
  
  Serial.println("Motor control initialized");
}

// Smooth start
void smoothStartMotor(uint8_t pin) {
  for (uint8_t s = 0; s <= TARGET_SPEED; s += RAMP_STEP) {
    ledcWrite(pin, s);
    delay(RAMP_DELAY);
  }
}

// Smooth stop
void smoothStopMotor() {
  uint8_t pin = (currentDir == DIR_UP) ? R_PWM_PIN
               : (currentDir == DIR_DOWN) ? L_PWM_PIN
               : 255;
               
  if (pin != 255) {
    for (int s = TARGET_SPEED; s >= 0; s -= RAMP_STEP*2) {
      ledcWrite(pin, s);
      delay(RAMP_DELAY);
    }
  }
  
  // Ensure both motors are fully stopped
  ledcWrite(R_PWM_PIN, 0);
  ledcWrite(L_PWM_PIN, 0);
}

// Move desk up
void moveDeskUp() {
  // Turn on backlight when starting movement
  if (!backlightOn) {
    lcd.backlight();
    backlightOn = true;
  }
  
  stopDeskMotor(); 
  delay(50);
  
  ledcWrite(R_PWM_PIN, 0);
  smoothStartMotor(L_PWM_PIN);
  currentDir = DIR_DOWN;
}

// Move desk down
void moveDeskDown() {
  // Turn on backlight when starting movement
  if (!backlightOn) {
    lcd.backlight();
    backlightOn = true;
  }
  
  stopDeskMotor(); 
  delay(50);
  
  ledcWrite(L_PWM_PIN, 0);
  smoothStartMotor(R_PWM_PIN);
  currentDir = DIR_UP;
}

#endif // MOTOR_CONTROL_H