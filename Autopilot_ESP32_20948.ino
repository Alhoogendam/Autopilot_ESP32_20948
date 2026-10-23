/*
  ESP32 WROOM-32D AUTOPILOT
  V45 - CVBS HMI, Arduino-ESP32 core 3.3.11

  HMI:
   - Screen only
   - controlled by remote RX500

  VIDEO:sxs             runs in dedicated core 1
    Composite CVBS on GPIO26
    320 x 270
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
#define SOFTWARE_VERSION "V45"

// ============================================================
// CVBS
// ============================================================

// v09 uses the Panel_CVBS implementation built into current M5GFX.
// This avoids the old ESP32_8BIT_CVBS Panel_CVBS name collision.
#include <M5GFX.h>  //version=0.2.33 -----------34 geeft storing! niet updaten
#include <M5ModuleRCA.h>
#include <RCSwitch.h>
#include <EasyTransfer.h>
#include <WiFi.h>



static M5ModuleRCA cvbs(
  320, 270, 320, 270,
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



// ============================================================
// Autopilot settings globals
// ============================================================

Preferences prefs;

float Pilot_gain = 100.0;
float Course_window = 4.0;
int Helm_speed = 100;
float Helm_window = 3.0;
int Rudder_timeout = 5;  // seconds
int Heading_error_timeout = 30;
float Minimum_rudder = 0.0;
float Maximum_rudder = 80.0;
float Rudder_physical_max = 40.0;
int Low_speed = 60;
float Pilot_comp = 0.0;

float headingError = 0.0;
float Tack_angle = 100.0;
int Tack_speed = 10;
float Response_Span = 160;

int steps = 0;
bool isWindValid = false;
bool pilot_working = false;

// Globale variabelen voor de GPS weergave op het AP-scherm
double displayLat = 0.0;
double displayLon = 0.0;
bool gpsDataValid = false;
char lastProcessedNMEA[82] = "";

// ============================================================
// Rudder settings globals
// ============================================================
// Physical rudder angles.
float Rudder_port_deg = -80.0;
float Rudder_center_deg = 0.0;
float Rudder_star_deg = 80.0;
float Rudder_deg = 0.0;
float Target_rudder_deg = 0.0;
// float filteredHeading = 0.0;

// Rudder ADC calibration.
int Rudder_port_adc = 1000;
int Rudder_center_adc = 2048;
int Rudder_star_adc = 3000;
bool rudderCalibrationWaiting = false;
bool rudderCalibrationExitRequested = false;

// ============================================================
// Motorcontroll settings globals
// ============================================================

// Change only if motor direction is reversed.
const int MOTOR_DIR_STARBOARD = HIGH;
const int MOTOR_DIR_PORT = LOW;


// ============================================================
// HEADING / AUTOPILOT
// ============================================================

float Heading = NAN;
float Set_heading = NAN;
float Heading_error = 0.0;
float pendingTargetHeading = 0.0f;

float windSpeed = 0.0;
float windDirection = 0.0;
float lastValidWindDir = 0.0;
float lastValidWindSpeed = 0.0;
float lastValidHeading = 0.0;

bool Auto_active = false;
bool Heading_valid = false;
bool Rudder_fault = false;
bool Course_fault = false;
bool rudderMoveActive = false;
bool tack_port = false;       // tack status
bool tack_starboard = false;  // tack status
bool tackStepDone = false;    // is used to pass the tacktimer only
bool tack_error = false;

unsigned long lastHeadingMillis = 0;
unsigned long rudderMoveStartMillis = 0;
unsigned long lastValidCompassTime = 0;

const unsigned long HEADING_TIMEOUT = 2000;
float pendingWindCommand = 0;

// ============================================================
// RX500 433 MHz REMOTE
// ============================================================

RCSwitch rx500 = RCSwitch();

unsigned long rx500PressCode = 0;
unsigned long rx500PressStart = 0;
unsigned long rx500LastFrameTime = 0;
bool rx500LongActionDone = false;
bool rx500OK = false;

unsigned long lastPlus10ShortRelease = 0;
const unsigned long DOUBLE_CLICK_MS = 500;

// ============================================================
// RX500 EVENTS
// RX500 detects the physical action only. Menu.h decides what it means.
// ============================================================
enum RX500Event {
  RX500_EVENT_NONE,
  RX500_EVENT_SHORT_MINUS1,
  RX500_EVENT_SHORT_PLUS1,
  RX500_EVENT_SHORT_MINUS10,
  RX500_EVENT_SHORT_PLUS10,
  RX500_EVENT_DOUBLE_PLUS10,
  RX500_EVENT_LONG_MINUS1,
  RX500_EVENT_LONG_PLUS1,
  RX500_EVENT_LONG_MINUS10,
  RX500_EVENT_LONG_PLUS10
};

RX500Event rx500Event = RX500_EVENT_NONE;
bool rx500Plus10Pending = false;
unsigned long rx500Plus10PendingTime = 0;

bool lastLeftState = HIGH;
bool lastRightState = HIGH;
bool lastOkState = HIGH;

unsigned long lastLeftChange = 0;
unsigned long lastRightChange = 0;
unsigned long lastOkChange = 0;

const unsigned long RX500_HOLD_TIME_MS = 1000;
const unsigned long RX500_RELEASE_TIME_MS = 250;


// ============================================================
// TIMING
// ============================================================

bool flash = false;      // changes every 500ms , used globaly
bool flash_1 = false;    // changes every 100ms , used globaly
bool flash_2 = false;    // changes every 300ms , used globaly
bool flash_PAL = false;  // changes every 300ms , used globaly

unsigned long previousMillis_T1 = 0;  // is used for the global 500ms timer
unsigned long previousMillis_T2 = 0;  // is used for the global 100ms timer
unsigned long previousMillis_T3 = 0;  // is used for the global 300ms timer
unsigned long previousMillis_T4 = 0;  // is used for the global PAL timer

// ------------------------------------------------------

// ============================================================
// FUNCTION PROTOTYPES
// ============================================================
void UpdateUARTlan();
void setupPreferences();
void loadSettings();
void saveSettings();

// FUNCTION PROTOTYPES HMI & DISPLAY
void setupVideo();
void updateDisplay();
void drawMainScreen();
void drawCompassRose();
void drawRudderIndicator(int x, int y);
void drawSettingsScreen();
void drawCalibrationScreen(const char *line1, const char *line2, const char *line3);
void presentFrame();
void beep(int durationMs);
void handleBeep();

// FUNCTION PROTOTYPES RUDDER
void readRudder();
float rudderAdcToDegrees(int adc);
void calibrateRudder();
void saveRudderCalibration();
void loadRudderCalibration();
void rudderTimeoutFault();

// FUNCTION PROTOTYPES RX500 remote
void processRX500();
void processRX500Event();
void processSerialCommands();

// FUNCTION PROTOTYPES Autopilot motor control
void controlAutopilot();
void autopilotOn();
void autopilotOff();
void stopMotor();
void setMotor(int direction, int pwm);
float signedHeadingError(float target, float actual);
float normalize360(float value);

// FUNCTION PROTOTYPES Heading
void change_heading();
void headingReceived(float newHeading);
void calculateHeadingError();

// FUNCTION PROTOTYPES TACK wind safety
bool tackHeadingWindSafe(float heading);
void abortTack();


// ============================================================
// EASYTRANSFER UART
// ============================================================

#define COMPASS_RX_PIN 2
#define COMPASS_TX_PIN 4

HardwareSerial CompassSerial(2);

// -----------------------------------------------------------
EasyTransfer ETout, ETin;
struct RECEIVE_DATA_STRUCTURE {
  float RX_heading;
  float RX_targetHeading;  // Toegevoegd voor Autopiloot OpenCpn
  float RX_windSpeed;
  float RX_windDirection;
  float RX_rudder;
  float RX_speed;
  float RX_depth;
  char RX_terminalResponse[64];
  char RX_NMEA[82];
};
RECEIVE_DATA_STRUCTURE RX_EData;

struct SEND_DATA_STRUCTURE {
  float TX_heading;
  float TX_targetHeading;  // Toegevoegd voor Autopiloot OpenCpn
  float TX_windSpeed;
  float TX_windDirection;
  float TX_rudder;
  float TX_speed;
  float TX_depth;
  char TX_terminalResponse[64];
  char TX_NMEA[82];
};
SEND_DATA_STRUCTURE TX_EData;
//================================ end Struct========================


// ============================================================
// OPENCPN VALIDATIE LOGICA
// ============================================================
unsigned long lastOpenCPNTargetMillis = 0;
const unsigned long OPENCPN_TIMEOUT_MS = 3000;

bool isOpenCPNValid() {
  if (isnan(RX_EData.RX_targetHeading)) return false;
  // Als de Wi-Fi node 511 stuurt, is OpenCPN niet meer actief
  if (RX_EData.RX_targetHeading == 511.0f) return false;
  if (millis() - lastOpenCPNTargetMillis > OPENCPN_TIMEOUT_MS) return false;
  if (RX_EData.RX_targetHeading < 0.0f || RX_EData.RX_targetHeading >= 360.0f) return false;
  return true;
}
// ============================================================

#include "Storage.h"
#include "Menu.h"
#include "Display.h"
#include "RX500.h"
#include "Rudder.h"
#include "MotorControl.h"
#include "Heading.h"
#include "Buzzer.h"
#include "CompassTerminal.h"
#include "UARTlan.h"


#include "tack.h"

// ==================================================================================================
// SETUP
// ==================================================================================================
void setup() {
  
  // Schakel Wi-Fi volledig uit power saving
    WiFi.mode(WIFI_OFF);
    WiFi.disconnect(true);
    
    // Schakel Bluetooth uit (indien geladen)
    #if defined(CONFIG_BT_ENABLED)
        btStop();
    #endif
  
  
  lastRxTime = millis();            //used for UARTlan.h
  lastValidCompassTime = millis();  // VOEG DIT TOE om valse triggers bij boot te voorkomen

  Serial.begin(115200);

  delay(300);

  CompassSerial.begin(115200, SERIAL_8N1, COMPASS_RX_PIN, COMPASS_TX_PIN);
  ETin.begin(details(RX_EData), &CompassSerial);
  ETout.begin(details(TX_EData), &CompassSerial);

  pinMode(PIN_CLUTCH, OUTPUT);
  pinMode(PIN_MOTOR_RPWM, OUTPUT);
  pinMode(PIN_MOTOR_LPWM, OUTPUT);
  // PIN_MOTOR_RPWM op kanaal 1 (gekoppeld aan timer 1)
  ledcAttachChannel(PIN_MOTOR_RPWM, 20000, 8, 1);

  // PIN_MOTOR_LPWM op kanaal 2 (gekoppeld aan timer 1)
  ledcAttachChannel(PIN_MOTOR_LPWM, 20000, 8, 2);
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

  stopMotor();  //safe boot
  // ensure that cluctch is OFF during boot.
  digitalWrite(PIN_CLUTCH, LOW);

  Heading_valid = false;
  Auto_active = false;

  beep(80);   //boot sound
  beep(200);  //boot sound
  beep(80);   //boot sound
}




void loop() {
  updateDisplay();
  /*
// ------------------------- NMEA parser (Geoptimaliseerd & Stabiel) -------------------
  if (strcmp(RX_EData.RX_NMEA, lastProcessedNMEA) != 0) {
    strncpy(lastProcessedNMEA, RX_EData.RX_NMEA, sizeof(lastProcessedNMEA) - 1);
    lastProcessedNMEA[sizeof(lastProcessedNMEA) - 1] = '\0';

    // (Optioneel: Serial.println hier weggehaald om de CPU te sparen en wippen te voorkomen)

    // Maak een veilige kopie om te parsen met strtok
    char nmeaCopy[100];
    strncpy(nmeaCopy, RX_EData.RX_NMEA, sizeof(nmeaCopy));
    nmeaCopy[sizeof(nmeaCopy) - 1] = '\0';

    // Controleer of het een RMC zin is
    if (strstr(nmeaCopy, "RMC") != NULL) {
      char *token = strtok(nmeaCopy, ",");
      char fields[15][16];
      int fIdx = 0;

      while (token != NULL && fIdx < 15) {
        strncpy(fields[fIdx], token, 15);
        fields[fIdx][15] = '\0';
        token = strtok(NULL, ",");
        fIdx++;
      }

      // Veld 3 = Lat, Veld 4 = N/S, Veld 5 = Lon, Veld 6 = E/W
      if (fIdx >= 7 && fields[2][0] == 'A' && strlen(fields[3]) > 0 && strlen(fields[5]) > 0) {
        double rawLat = atof(fields[3]);
        int degLat = (int)(rawLat / 100);
        double tempLat = degLat + (rawLat - (degLat * 100)) / 60.0;
        if (fields[4][0] == 'S') tempLat = -tempLat;

        double rawLon = atof(fields[5]);
        int degLon = (int)(rawLon / 100);
        double tempLon = degLon + (rawLon - (degLon * 100)) / 60.0;
        if (fields[6][0] == 'W') tempLon = -tempLon;

        // Alleen overschrijven als de coördinaten daadwerkelijk zijn binnengekomen
        displayLat = tempLat;
        displayLon = tempLon;
        gpsDataValid = true;
      } else {
        gpsDataValid = false;
        displayLat = 0.0;
        displayLon = 0.0;
      }
    }
  }
  // ---------------------------end -------------------------
*/


//---------------- non-blocking timers ----------------------
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis_T1 >= 500) {
    previousMillis_T1 = currentMillis;
    flash = !flash;  // Globale flash variable indien nodig
  }
  if (currentMillis - previousMillis_T2 >= 100) {
    previousMillis_T2 = currentMillis;
    tackStepDone = false;
    flash_1 = !flash_1;
  }
  if (currentMillis - previousMillis_T3 >= 300) {  // tack timer
    previousMillis_T3 = currentMillis;
    tackStepDone = false;
    flash_2 = !flash_2;
  }

  if (currentMillis - previousMillis_T4 >= 40) {  // standaard PAL 25beelden per seconde
    previousMillis_T4 = currentMillis;
    flash_PAL = !flash_PAL;
  }
// ----------------------------------- end ----------------------------------


  // ----------------------------- TACK-TIMER LOGICA --------------------------------
  if (steps >= 1 && flash_1 == true && tackStepDone == false) {
    if (!tackHeadingWindSafe(Set_heading)) {
      abortTack();
      beep(80);
    } else {
      float tackStep = Tack_angle / ((Tack_angle / Tack_speed) * 5.0);
      float tackChange = 0.0;

      if (tack_starboard == true) tackChange = tackStep;
      else if (tack_port == true) tackChange = -tackStep;

      if (tackChange != 0.0 && !tackHeadingWindSafe(normalize360(Set_heading + tackChange))) {
        abortTack();
        beep(80);
      } else {
        tackStepDone = true;
        steps = steps - 1;
        if (tackChange != 0.0) {
          change_heading(tackChange);
        }
        if (steps == 0) {
          tack_port = false;
          tack_starboard = false;
        }
      }
    }
  }
// ---------------------------------end -----------------------------

// ----------------- update lan with PAL sync ------------
  if (flash_PAL == true) {
    UpdateUARTlan();
  }
// -----------------------end -------------------------------
 
  
  // ------------------ openCPN waypoint handeling ----------------
  headingReceived(RX_EData.RX_heading);

  if (currentNavMode == NAV_MODE_OPENCPN) {
    if (isOpenCPNValid()) {
      if (Auto_active) {
        Set_heading = RX_EData.RX_targetHeading;
      }
    } else {
      currentNavMode = NAV_MODE_COMPASS;
      beep(300);
    }
  }
// ------------------------end ---------------------------------------


  // --- SENSORS & REMOTE ---
  readRudder();
  processRX500();
  processRX500Event();


  // ==========================================================
  // WIND  heartbeat filter
  // ==========================================================
  if (isWindValid == true && RX_EData.RX_windDirection >= 0.0 && RX_EData.RX_windDirection <= 360.0) {
    windSpeed = RX_EData.RX_windSpeed;
    windDirection = RX_EData.RX_windDirection;
  }
  if (isWindValid == true && RX_EData.RX_windDirection >= 360.0) {
    windSpeed = lastValidWindSpeed;
    windDirection = lastValidWindDir;
  }



  // ==========================================================
  // UART COMPASS heartbeat filter
  // ==========================================================
  if (Heading_valid == true && RX_EData.RX_heading >= 0.0 && RX_EData.RX_heading < 360.0) {
    Heading = RX_EData.RX_heading;
    lastValidHeading = Heading;  // Sla de echte koers op als back-up
  }

  // Als de waarde >= 360 is (zoals de 512.00 heartbeat), vallen we terug op de laatste geldige koers
  if (Heading_valid == true && RX_EData.RX_heading >= 360.0) {
    Heading = lastValidHeading;
  }

  controlAutopilot();
  handleBeep();
}
