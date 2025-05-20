// hall_sensor.h
// Hall sensor module for desk position tracking

#ifndef HALL_SENSOR_H
#define HALL_SENSOR_H

#include "config.h"

// Hall sensor variables
volatile int pulseCount = 0;
int sensor1LastState = 0;
int sensor2LastState = 0;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 5;
String hallDirection = "None";
int lastPulseCount = 0;
unsigned long lastPrintTime = 0;
const unsigned long printInterval = 500;  // Print every 500ms

// Height conversion functions
float pulsesToHeight(int pulses) {
  return MIN_HEIGHT_CM + (pulses * CM_PER_PULSE);
}

int heightToPulses(float height) {
  return (int)((height - MIN_HEIGHT_CM) / CM_PER_PULSE);
}

// Initialize hall sensors
void initHallSensors() {
  pinMode(HALL_SENSOR1_PIN, INPUT);
  pinMode(HALL_SENSOR2_PIN, INPUT);
  sensor1LastState = digitalRead(HALL_SENSOR1_PIN);
  sensor2LastState = digitalRead(HALL_SENSOR2_PIN);
  Serial.println("Hall sensors initialized");
}

// Check hall sensors for pulse detection
void checkHallSensors() {
  // Read the current state of both Hall sensors
  int sensor1State = digitalRead(HALL_SENSOR1_PIN);
  int sensor2State = digitalRead(HALL_SENSOR2_PIN);
  
  // Check if sensor1 has changed from LOW to HIGH (rising edge)
  if (sensor1State == HIGH && sensor1LastState == LOW) {
    // Debounce
    if (millis() - lastDebounceTime > debounceDelay) {
      // Determine direction based on sensor2 state
      if (sensor2State == LOW) {
        // Clockwise rotation
        pulseCount++;
        hallDirection = "CW";
      } else {
        // Counterclockwise rotation
        pulseCount--;
        hallDirection = "CCW";
      }
      
      lastDebounceTime = millis();
    }
  }
  
  // Print hall sensor data periodically to reduce Serial spam
  if (millis() - lastPrintTime >= printInterval) {
    if (pulseCount != lastPulseCount) {  // Only print if there's a change
      Serial.print("Pulses: ");
      Serial.print(pulseCount);
      Serial.print(", Direction: ");
      Serial.println(hallDirection);
      
      lastPulseCount = pulseCount;
    }
    lastPrintTime = millis();
  }
  
  // Save the current states for the next comparison
  sensor1LastState = sensor1State;
  sensor2LastState = sensor2State;
}

#endif // HALL_SENSOR_H