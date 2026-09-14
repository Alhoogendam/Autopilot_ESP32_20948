/*
  ESP32 WROOM-32D AUTOPILOT
  V39 - CVBS HMI + 20948 Compass, Arduino-ESP32 core 3.3.11

  HMI:
   - Screen only
   - controlled by remote

  VIDEO:sxs
    Composite CVBS on GPIO26
    256 x 240
    Existing 4.7" rear-view monitor
    Uses current M5GFX M5ModuleRCA / Panel_CVBS

  IMPORTANT PIN OUT:
    GPIO39 = rudder analog 0-2V
    GPIO32 = IBT-2 RPWM
    GPIO33 = IBT-2 LPWM
    GPIO25 = buzzer
    GPIO26 = CVBS
    GPIO13 = clutch
    GPIO21 = SDA Compass
    GPIO22 = SCL Compass




  Rudder:
    3-point calibration: FULL PORT / CENTER / FULL STARBOARD
    ADC values stored in Preferences/NVS.


*/

#include <Arduino.h>
#include <Preferences.h>

#define SOFTWARE_VERSION "V39"

// ============================================================
// ============================================================


// ============================================================
// CVBS
// ============================================================

// v09 uses the Panel_CVBS implementation built into current M5GFX.
// This avoids the old ESP32_8BIT_CVBS Panel_CVBS name collision.
#include <M5GFX.h>
#include <M5ModuleRCA.h>
#include <RCSwitch.h>

static M5ModuleRCA cvbs(
  256, 240, 256, 240,
  M5ModuleRCA::signal_type_t::PAL,
  M5ModuleRCA::psram_no_use,
  26);

static M5Canvas screen(&cvbs);

// ============================================================
// PINOUT
// ============================================================

#define PIN_RUDDER 39

#define PIN_MOTOR_RPWM 32
#define PIN_MOTOR_LPWM 33

#define PIN_BUZZER 25
#define PIN_CVBS 26


#define PIN_CLUTCH 13
#define PIN_RX500 12

//----------------Nautnect remote codes -----------------------
#define RX500_CODE_MINUS1 230821UL
#define RX500_CODE_PLUS1 880810UL
#define RX500_CODE_MINUS10 211124UL
#define RX500_CODE_PLUS10 190728UL
// -------------------------------------------------------------
#define MOTOR_PWM_MIN 100
#define MOTOR_PWM_MAX 255


// ============================================================
// SETTINGS globals
// ============================================================

Preferences prefs;

float Pilot_gain = 100.0;
float Course_window = 4.0;
int Helm_speed = 100;
float Helm_window = 3.0;
int Rudder_timeout = 5; // seconds
int Heading_error_timeout = 30;
bool rudderCalibrationExitRequested = false;
float Minimum_rudder = 0.0;
float Maximum_rudder = 80.0;
int Low_speed = 60;
float Pilot_comp = 0.0;

float Tack_angle = 100.0;
int Tack_speed = 10;

// Physical rudder angles.
float Rudder_port_deg = -80.0;
float Rudder_center_deg = 0.0;
float Rudder_star_deg = 80.0;

// Rudder ADC calibration.
int Rudder_port_adc = 1000;
int Rudder_center_adc = 2048;
int Rudder_star_adc = 3000;
bool rudderCalibrationWaiting = false;

// Change only if motor direction is reversed.
const int MOTOR_DIR_STARBOARD = HIGH;
const int MOTOR_DIR_PORT = LOW;

// ============================================================
// HEADING / AUTOPILOT
// ============================================================

float Heading = NAN;
float Set_heading = NAN;
float Heading_error = 0.0;

float Rudder_deg = 0.0;
float Target_rudder_deg = 0.0;

bool Auto_active = false;
bool Heading_valid = false;

unsigned long lastHeadingMillis = 0;
unsigned long rudderMoveStartMillis = 0;
bool rudderMoveActive = false;
bool Rudder_fault = false;
bool Course_fault = false;
const unsigned long HEADING_TIMEOUT = 2000;

// ============================================================
// HMI
// ============================================================

enum HmiMode {
  HMI_NORMAL,
  HMI_SETTINGS_SELECT,
  HMI_SETTINGS_EDIT
};

HmiMode hmiMode = HMI_NORMAL;

int normalSelection = 2;
// 0=-10, 1=-1, 2=STOP, 3=+1, 4=+10

int settingsSelection = 0;

bool okWasDown = false;
unsigned long okDownTime = 0;
unsigned long lastOkRelease = 0;

const unsigned long DEBOUNCE_MS = 35;
const unsigned long DOUBLE_CLICK_MS = 450;
unsigned long lastPlus10ShortRelease = 0;
// ============================================================
// RX500 433 MHz REMOTE
// ============================================================

RCSwitch rx500 = RCSwitch();

unsigned long rx500PressCode = 0;
unsigned long rx500PressStart = 0;
unsigned long rx500LastFrameTime = 0;
bool rx500LongActionDone = false;
bool rx500OK = false;

const unsigned long RX500_HOLD_TIME_MS = 1000;
const unsigned long RX500_RELEASE_TIME_MS = 250;


bool lastLeftState = HIGH;
bool lastRightState = HIGH;
bool lastOkState = HIGH;

unsigned long lastLeftChange = 0;
unsigned long lastRightChange = 0;
unsigned long lastOkChange = 0;

// ============================================================
// TIMING
// ============================================================

unsigned long lastDisplayUpdate = 0;
unsigned long lastControlUpdate = 0;



// ============================================================
// FUNCTION PROTOTYPES
// ============================================================

void setupPreferences();
void loadSettings();
void saveSettings();

void setupVideo();
void updateDisplay();
void drawMainScreen();
void drawCompassRose();
void drawRudderIndicator();
void drawSettingsScreen();
void drawCalibrationScreen(const char *line1, const char *line2, const char *line3);
void presentFrame();

void leftPressed();
void rightPressed();
void okPressed();
void executeNormalFunction();
void enterSettings();
void editSetting();

void readRudder();
float rudderAdcToDegrees(int adc);
void calibrateRudder();
void saveRudderCalibration();
void loadRudderCalibration();
bool waitForOK();
void processRX500();

void processSerialCommands();
void readRudder();
void controlAutopilot();
void updateDisplay();

void autopilotOn();
void autopilotOff();
void stopMotor();
void controlAutopilot();
void setMotor(int direction, int pwm);
void rudderTimeoutFault();

void headingReceived(float newHeading);
void calculateHeadingError();

void beep(int durationMs);

float normalize360(float value);
float signedHeadingError(float target, float actual);

// ============================================================
// SETUP
// ============================================================


#include "Storage.h"
#include "Display.h"
#include "RX500.h"
#include "Rudder.h"
#include "MotorControl.h"
#include "Heading.h"
#include "ICM20948_Compass.h"
#include "Buzzer.h"

void setup() {
  Serial.begin(115200);
  delay(300);

  initCompass();

  pinMode(PIN_CLUTCH, OUTPUT);
  pinMode(PIN_MOTOR_RPWM, OUTPUT);
  pinMode(PIN_MOTOR_LPWM, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // Ensure the motor and clutch are off during startup.
  digitalWrite(PIN_CLUTCH, LOW);
  digitalWrite(PIN_MOTOR_RPWM, LOW);
  digitalWrite(PIN_MOTOR_LPWM, LOW);

  // Attach PWM outputs.
  ledcAttach(PIN_MOTOR_RPWM, 1000, 8);
  ledcAttach(PIN_MOTOR_LPWM, 1000, 8);

  // Explicitly stop the motor after PWM initialization.
  ledcWrite(PIN_MOTOR_RPWM, 0);
  ledcWrite(PIN_MOTOR_LPWM, 0);

  analogReadResolution(12);
  // get stored parameters for autopilot > storga.h
  setupPreferences();
  loadSettings();
  loadRudderCalibration();

  setupVideo();

  rx500.enableReceive(digitalPinToInterrupt(PIN_RX500));

  stopMotor();

  // Start safely in STANDBY. The pilot is enabled only by a long +1 command.
  // Heading wordt geleverd door de ICM20948 via updateCompass().
  Heading_valid = false;
  Auto_active = false;
  
  // ensure that cluctch is OFF during boot.
  digitalWrite(PIN_CLUTCH, LOW);



  Serial.println("==============================================");
  Serial.print("ESP32 Autopilot ");
  Serial.println(SOFTWARE_VERSION);
  Serial.println("==============================================");
  Serial.println("COMMANDS");
  Serial.println("==============================================");
  Serial.println(
    "A = Start autopilot");
  Serial.println(
    "S = Stop autopilot");
  Serial.println("==============================================");

  beep(80);
}

// ============================================================
// LOOP
// ============================================================

void loop() {
  // get heading from compass
  updateCompass();
  // get rudder value
  readRudder();
  // check if there is a command entered in the monitor
  processSerialCommands();
  // check if there is a remote button is pushed for the remote
  processRX500();

  if (Heading_valid && millis() - lastHeadingMillis > HEADING_TIMEOUT) {
    Heading_valid = false;

    stopMotor();      //no heading = stop motor and clutch

    if (Auto_active)
      Auto_active = false;

    digitalWrite(PIN_CLUTCH, LOW);
  }

  if (millis() - lastControlUpdate >= 30) {
    lastControlUpdate = millis();
    controlAutopilot();
  }

  if (millis() - lastDisplayUpdate >= 200) {
    lastDisplayUpdate = millis();
    updateDisplay();
  }
}
