// config.h
// Configuration file for desk height control system

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <VL53L0X.h>

// — Wi-Fi credentials & Auth —
typedef const char* cstr;
const cstr WIFI_SSID       = "A-19_4G";
const cstr WIFI_PASSWORD   = "lebhikhari";
const cstr AUTH_USER       = "ap539813@gmail.com";
const cstr AUTH_PASS       = "kSE16DXMeo7nwHf";
const cstr SECRET_KEY      = AUTH_PASS;

// — I2C pins —
const uint8_t SDA_PIN = 21;
const uint8_t SCL_PIN = 22;

// — BTS7960 motor driver pins —
const uint8_t R_EN_PIN  = 25;
const uint8_t L_EN_PIN  = 26;
const uint8_t R_PWM_PIN = 14;
const uint8_t L_PWM_PIN = 27;

// — Hall sensor pins —
const uint8_t HALL_SENSOR1_PIN = 32;  // Green wire - adjust pin as needed
const uint8_t HALL_SENSOR2_PIN = 33;  // White wire - adjust pin as needed

// — Height and pulse calibration —
const int PULSES_AT_MIN_HEIGHT = 0;     // Pulses at lowest position
const int PULSES_AT_MAX_HEIGHT = 1840;  // Pulses at highest position
const float MIN_HEIGHT_CM = 69.0;       // Height at lowest position
const float MAX_HEIGHT_CM = 105.0;      // Height at highest position
const float CM_PER_PULSE = (MAX_HEIGHT_CM - MIN_HEIGHT_CM) / (PULSES_AT_MAX_HEIGHT - PULSES_AT_MIN_HEIGHT);

// — EEPROM Storage —
const int EEPROM_SIZE = 16;             // Size to allocate in EEPROM
const int EEPROM_MAGIC_ADDR = 0;        // Address for magic number (to check if initialized)
const int EEPROM_PULSES_ADDR = 4;       // Address for pulse count storage
const int EEPROM_MAGIC = 0x4445534B;    // "DESK" magic number to verify EEPROM is initialized

// — PWM / ramp settings —
const uint8_t  TARGET_SPEED = 249;
const uint8_t  DISTANCE_CYCLE = 15;     // Number of readings to average for calibration
const uint8_t  PWM_RES = 8;
const uint32_t PWM_FREQ = 1000;
const uint8_t  RAMP_STEP = 10;           // Reduced for finer control
const unsigned RAMP_DELAY = 10;
const uint8_t  ERROR_THRESHOLD = 10;

// — System settings —
const unsigned long BACKLIGHT_TIMEOUT = 10000;  // 10 seconds
const unsigned long MOVEMENT_TIMEOUT = 15000;   // 15 seconds max movement time

// Direction enum
enum { DIR_NONE=0, DIR_UP=1, DIR_DOWN=-1 };

// Global variables (extern to be defined in main file)
extern bool isMoving;
extern int moveTargetPulses;
extern unsigned moveStartTime;
extern bool abortMovement;
extern int lastStoppedPulses;
extern int8_t currentDir;
extern volatile int pulseCount;
extern VL53L0X tof;
extern LiquidCrystal_I2C lcd;
extern unsigned long lastMotorStopTime;
extern bool backlightOn;

// Function declarations for cross-module use
extern void stopDeskMotor();

#endif // CONFIG_H