// storage.h
// EEPROM storage module for desk position persistence

#ifndef STORAGE_H
#define STORAGE_H

#include "config.h"

// Initialize EEPROM
void initStorage() {
  EEPROM.begin(EEPROM_SIZE);
  Serial.println("EEPROM storage initialized");
}

// Save pulse count to EEPROM
void savePulseCount() {
  EEPROM.writeInt(EEPROM_PULSES_ADDR, pulseCount);
  EEPROM.commit();
  Serial.println("Saved pulse count: " + String(pulseCount));
}

// Load pulse count from EEPROM
void loadPulseCount() {
  // Check if EEPROM is initialized with our magic number
  int magic = EEPROM.readInt(EEPROM_MAGIC_ADDR);
  if (magic != EEPROM_MAGIC) {
    // First time use - initialize EEPROM
    Serial.println("Initializing EEPROM for first use");
    EEPROM.writeInt(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
    EEPROM.writeInt(EEPROM_PULSES_ADDR, 0);
    EEPROM.commit();
    pulseCount = 0;
  } else {
    // Load saved pulse count
    pulseCount = EEPROM.readInt(EEPROM_PULSES_ADDR);
    Serial.println("Loaded pulse count: " + String(pulseCount));
  }
  
  // Ensure pulse count is within valid range
  if (pulseCount < PULSES_AT_MIN_HEIGHT) pulseCount = PULSES_AT_MIN_HEIGHT;
  if (pulseCount > PULSES_AT_MAX_HEIGHT) pulseCount = PULSES_AT_MAX_HEIGHT;
}

#endif // STORAGE_H