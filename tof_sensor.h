// tof_sensor.h
// VL53L0X Time-of-Flight sensor module for height measurement

#ifndef TOF_SENSOR_H
#define TOF_SENSOR_H

#include "config.h"

// Initialize the VL53L0X sensor
bool initToFSensor(VL53L0X* sensor) {
  bool success = sensor->init();
  if (success) {
    Serial.println("VL53L0X sensor initialized");
    sensor->setTimeout(500);
    sensor->startContinuous();
  }
  return success;
}

// Measure height with ToF sensor (averaged over multiple readings)
float measureHeightWithTof(VL53L0X* sensor) {
  float totalHeight = 0;
  int validReadings = 0;
  
  // Take multiple readings and average them for better accuracy
  for (uint8_t s = 0; s < DISTANCE_CYCLE; s++) {
    uint16_t dist_mm = sensor->readRangeContinuousMillimeters();
    
    if (!sensor->timeoutOccurred()) {
      float height_cm = (dist_mm / 10.0); 
      
      // // Apply constraints
      // if (height_cm < MIN_HEIGHT_CM) height_cm = MIN_HEIGHT_CM;
      // if (height_cm > MAX_HEIGHT_CM) height_cm = MAX_HEIGHT_CM;
      
      totalHeight += height_cm;
      validReadings++;
      
      Serial.print("Reading #");
      Serial.print(s+1);
      Serial.print(": ");
      Serial.print(dist_mm);
      Serial.print("mm → ");
      Serial.print(height_cm);
      Serial.println(" cm");
    } else {
      Serial.print("Reading #");
      Serial.print(s+1);
      Serial.println(": Timeout");
    }
    
    delay(5); // Short delay between readings
  }
  
  // Calculate average if we have valid readings
  if (validReadings > 0) {
    int avgHeight = totalHeight / validReadings;
    Serial.print("Average height from ");
    Serial.print(validReadings);
    Serial.print(" readings: ");
    Serial.print(avgHeight);
    Serial.println(" cm");
    return avgHeight;
  } else {
    Serial.println("No valid ToF readings obtained!");
    return -1; // Indicate error
  }
}

#endif // TOF_SENSOR_H