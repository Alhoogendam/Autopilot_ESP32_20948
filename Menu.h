// ============================================================
// MENU.H - V44 menu state machine met Wind Speed Kalibratie
// ============================================================

// ============================================================
// NAV MODE / AP KOERSBRON DEFINITIE
// ============================================================
bool isOpenCPNValid();
enum NavMode {
  NAV_MODE_COMPASS,  // Standaard UART Kompas
  NAV_MODE_OPENCPN   // OpenCPN Waypoint via Wi-Fi
};

NavMode currentNavMode = NAV_MODE_COMPASS;
extern bool isOpenCPNValid();

// ============================================================
// UART COMPASS TERMINAL
// ============================================================

const char* const compassTerminalCommands[] = {
  "READ CAL.",
  "WRITE CAL.",
  "CALLIBRATE",
  "STOP&SAFE",
  "CLEARCAL"
};

const int COMPASS_TERMINAL_COMMAND_COUNT = 5;
int compassTerminalSelection = 0;

#define COMPASS_TERMINAL_LINES 5
#define COMPASS_TERMINAL_LINE_LENGTH 64
#define NMEA_TERMINAL_LINES 10
#define NMEA_TERMINAL_LINE_LENGTH 82

char compassTerminalResponse[COMPASS_TERMINAL_LINE_LENGTH] = "";
char compassTerminalLines[COMPASS_TERMINAL_LINES][COMPASS_TERMINAL_LINE_LENGTH] = {};
char compassTerminalLastCommand[32] = "";

void sendCompassTerminalCommand(const char* command);

// ================ MENU STATES ===========================================
enum MenuState {
  MENU_NORMAL,
  MENU_SETTINGS_SELECT,
  MENU_SETTINGS_EDIT,
  MENU_CALIBRATION_SELECT,
  MENU_CAL_RUDDER,
  MENU_CAL_UART_COMPASS,
  MENU_NMEA_MONITOR,
  MENU_CAL_WIND_SPEED, // <-- TOEGEVOEGD: Wind speed kalibratiescherm

  MENU_CAL_RUDDER_START,
  MENU_CAL_RUDDER_PORT,
  MENU_CAL_RUDDER_PORT_SAVED,
  MENU_CAL_RUDDER_CENTER,
  MENU_CAL_RUDDER_CENTER_SAVED,
  MENU_CAL_RUDDER_STARBOARD,
  MENU_CAL_RUDDER_SAVED
};

MenuState menuState = MENU_NORMAL;

// Normal screen selection: 0=-10, 1=-1, 2=+1, 3=+10
int normalSelection = 1;

int settingsSelection = 0;
const int SETTINGS_COUNT = 16;
const int PARAMETER_COUNT = 14;

const char* const settingNames[SETTINGS_COUNT] = {
  "PILOT GAIN",
  "COURSE WINDOW",
  "HELM MAXIMUM SPEED",
  "HELM MINIMUM SPEED",
  "RUDDER TIMEOUT",
  "COURSE ERROR",
  "MIN RUDDER",
  "MAX RUDDER",
  "RUDDER PHYS MAX",
  "HELM WINDOW",
  "BACKLASH COMP",
  "TACK ANGLE",
  "TACK SPEED",
  "RESPONSE SPAN",
  "CALIBRATION",
  "FACTORY RESET"
};

// Uitgebreid naar 4 opties inclusief WIND SPEED
const char* const calibrationNames[4] = {
  "RUDDER",
  "UART COMPASS",
  "NMEA MONITOR",
  "WIND SPEED"
};

int calibrationSelection = 0;

// Variabele voor de ingestelde wind speed gain in de AP
float apWindCalibrationGain = 1.00f;

#define hmiMode menuState
#define HMI_NORMAL MENU_NORMAL
#define HMI_SETTINGS_SELECT MENU_SETTINGS_SELECT
#define HMI_SETTINGS_EDIT MENU_SETTINGS_EDIT

void menuApplyLimits() {
  Pilot_gain = constrain(Pilot_gain, 0.0f, 500.0f);
  Course_window = constrain(Course_window, 0.5f, 30.0f);
  Helm_speed = constrain(Helm_speed, 0, 255);
  Helm_window = constrain(Helm_window, 0.5f, 20.0f);
  Rudder_timeout = constrain(Rudder_timeout, 0, 120);
  Heading_error_timeout = constrain(Heading_error_timeout, 0, 300);
  Minimum_rudder = constrain(Minimum_rudder, 0.0f, 80.0f);
  Maximum_rudder = constrain(Maximum_rudder, 1.0f, 90.0f);
  Response_Span = constrain(Response_Span, 0.0f, 300.0f);
  Rudder_physical_max = constrain(Rudder_physical_max, 1.0f, 90.0f);
  Rudder_port_deg = -Rudder_physical_max;
  Rudder_center_deg = 0.0f;
  Rudder_star_deg = Rudder_physical_max;
  Low_speed = constrain(Low_speed, 0, 255);
  Pilot_comp = constrain(Pilot_comp, -10.0f, 10.0f);
  Tack_angle = constrain(Tack_angle, 10.0f, 180.0f);
  Tack_speed = constrain(Tack_speed, 1, 255);
}

void menuEnterSettings() {
  if (Auto_active)
    autopilotOff();

  menuState = MENU_SETTINGS_SELECT;
  settingsSelection = 0;
  beep(60);
}

void menuBackToNormal() {
  menuState = MENU_NORMAL;
  beep(80);
}

// Functie om wind kalibratie commando's direct over de ring te sturen

void sendWindCalibrationCommand(float commandValue) {
  pendingWindCommand = commandValue;
}

void menuMoveLeft() {
  switch (menuState) {

    case MENU_NORMAL:
      normalSelection--;
      if (normalSelection < 0) normalSelection = 3;
      break;

    case MENU_SETTINGS_SELECT:
      settingsSelection--;
      if (settingsSelection < 0) settingsSelection = SETTINGS_COUNT - 1;
      break;

    case MENU_SETTINGS_EDIT:
      switch (settingsSelection) {
        case 0: Pilot_gain -= 10; break;
        case 1: Course_window -= 1; break;
        case 2: Helm_speed -= 5; break;
        case 3: Low_speed -= 5; break;
        case 4: Rudder_timeout -= 1; break;
        case 5: Heading_error_timeout -= 1; break;
        case 6: Minimum_rudder -= 1; break;
        case 7: Maximum_rudder -= 1; break;
        case 8: Rudder_physical_max -= 1; break;
        case 9: Helm_window -= 1; break;
        case 10: Pilot_comp -= 0.1f; break;
        case 11: Tack_angle -= 10; break;
        case 12: Tack_speed -= 1; break;
        case 13: Response_Span -= 10; break;
        default: break;
      }
      menuApplyLimits();
      break;

    case MENU_CALIBRATION_SELECT:
      calibrationSelection--;
      if (calibrationSelection < 0) calibrationSelection = 3; // 4 opties (0,1,2,3)
      break;

    case MENU_CAL_UART_COMPASS:
      compassTerminalSelection--;
      if (compassTerminalSelection < 0) compassTerminalSelection = COMPASS_TERMINAL_COMMAND_COUNT - 1;
      break;

    // WIND KALIBRATIE: Knoop -1 verlaagt de gain met 0.05
    case MENU_CAL_WIND_SPEED:
      apWindCalibrationGain -= 0.05f;
      if (apWindCalibrationGain < 0.05f) apWindCalibrationGain = 0.05f;
      // Stuur live gain aanpassing naar de wind node (400.0 + gain)
      sendWindCalibrationCommand(400.0f + apWindCalibrationGain);
      beep(35);
      break;

    default:
      break;
  }
}

void menuMoveRight() {
  switch (menuState) {

    case MENU_NORMAL:
      normalSelection++;
      if (normalSelection > 3) normalSelection = 0;
      break;

    case MENU_SETTINGS_SELECT:
      settingsSelection++;
      if (settingsSelection >= SETTINGS_COUNT) settingsSelection = 0;
      break;

    case MENU_SETTINGS_EDIT:
      switch (settingsSelection) {
        case 0: Pilot_gain += 10; break;
        case 1: Course_window += 1; break;
        case 2: Helm_speed += 5; break;
        case 3: Low_speed += 5; break;
        case 4: Rudder_timeout += 1; break;
        case 5: Heading_error_timeout += 1; break;
        case 6: Minimum_rudder += 1; break;
        case 7: Maximum_rudder += 1; break;
        case 8: Rudder_physical_max += 1; break;
        case 9: Helm_window += 1; break;
        case 10: Pilot_comp += 0.1f; break;
        case 11: Tack_angle += 10; break;
        case 12: Tack_speed += 1; break;
        case 13: Response_Span += 10; break;
        default: break;
      }
      menuApplyLimits();
      break;

    case MENU_CALIBRATION_SELECT:
      calibrationSelection++;
      if (calibrationSelection > 3) calibrationSelection = 0; // 4 opties (0,1,2,3)
      break;

    case MENU_CAL_UART_COMPASS:
      compassTerminalSelection++;
      if (compassTerminalSelection >= COMPASS_TERMINAL_COMMAND_COUNT) compassTerminalSelection = 0;
      break;

    // WIND KALIBRATIE: Knoop +1 verhoogt de gain met 0.05
    case MENU_CAL_WIND_SPEED:
      apWindCalibrationGain += 0.05f;
      if (apWindCalibrationGain > 5.00f) apWindCalibrationGain = 5.00f;
      // Stuur live gain aanpassing naar de wind node (400.0 + gain)
      sendWindCalibrationCommand(400.0f + apWindCalibrationGain);
      beep(35);
      break;

    default:
      break;
  }
}

void menuSelectNormal() {
  if (!Heading_valid) return;

  switch (normalSelection) {
    case 0: if (!isnan(Set_heading)) Set_heading = normalize360(Set_heading - 10); break;
    case 1: if (!isnan(Set_heading)) Set_heading = normalize360(Set_heading - 1); break;
    case 2: if (!isnan(Set_heading)) Set_heading = normalize360(Set_heading + 1); break;
    case 3: if (!isnan(Set_heading)) Set_heading = normalize360(Set_heading + 10); break;
  }

  calculateHeadingError();
  beep(35);
}

void menuOpenCalibration() {
  if (Auto_active) autopilotOff();
  calibrationSelection = 0;
  menuState = MENU_CALIBRATION_SELECT;
  beep(60);
}

void menuEnter() {
  switch (menuState) {

    case MENU_SETTINGS_SELECT:
      if (settingsSelection == 14) {
        menuOpenCalibration();
        return;
      }
      if (settingsSelection == 15) {
        restoreFactorySettings();
        menuApplyLimits();
        beep(120);
        return;
      }
      menuState = MENU_SETTINGS_EDIT;
      beep(60);
      break;

    case MENU_SETTINGS_EDIT:
      saveSettings();
      menuState = MENU_SETTINGS_SELECT;
      beep(60);
      break;

    case MENU_CALIBRATION_SELECT:
      if (calibrationSelection == 0) {
        menuState = MENU_CAL_RUDDER_START;
        beep(60);
      } else if (calibrationSelection == 1) {
        menuState = MENU_CAL_UART_COMPASS;
        compassTerminalSelection = 0;
        compassTerminalResponse[0] = '\0';
        compassTerminalLastCommand[0] = '\0';
        for (int i = 0; i < COMPASS_TERMINAL_LINES; i++) compassTerminalLines[i][0] = '\0';
        beep(60);
      } else if (calibrationSelection == 2) {
        menuState = MENU_NMEA_MONITOR;
        beep(60);
      } else if (calibrationSelection == 3) {
        // Open Wind Speed kalibratie en stuur meteen 401.0 om huidige gain van de node op te vragen
        menuState = MENU_CAL_WIND_SPEED;
        sendWindCalibrationCommand(401.0f);
        beep(60);
      }
      break;

    case MENU_CAL_RUDDER_START:
      menuState = MENU_CAL_RUDDER_PORT;
      beep(60);
      break;

    case MENU_CAL_RUDDER_PORT:
      Rudder_port_adc = analogRead(PIN_RUDDER);
      menuState = MENU_CAL_RUDDER_PORT_SAVED;
      beep(60);
      break;

    case MENU_CAL_RUDDER_PORT_SAVED:
      menuState = MENU_CAL_RUDDER_CENTER;
      beep(60);
      break;

    case MENU_CAL_RUDDER_CENTER:
      Rudder_center_adc = analogRead(PIN_RUDDER);
      menuState = MENU_CAL_RUDDER_CENTER_SAVED;
      beep(60);
      break;

    case MENU_CAL_RUDDER_CENTER_SAVED:
      menuState = MENU_CAL_RUDDER_STARBOARD;
      beep(60);
      break;

    case MENU_CAL_RUDDER_STARBOARD:
      Rudder_star_adc = analogRead(PIN_RUDDER);
      saveRudderCalibration();
      menuState = MENU_CAL_RUDDER_SAVED;
      beep(80);
      break;

    case MENU_CAL_RUDDER_SAVED:
      menuState = MENU_CALIBRATION_SELECT;
      beep(80);
      break;

    case MENU_CAL_UART_COMPASS:
      sendCompassTerminalCommand(compassTerminalCommands[compassTerminalSelection]);
      beep(60);
      break;

    // WIND KALIBRATIE: Knoop +10 (Enter) stuurt commando 402.0 (Save in EEPROM/Flash)
    case MENU_CAL_WIND_SPEED:
      sendWindCalibrationCommand(402.0f);
      beep(150);
      break;

    default:
      break;
  }
}

void menuBack() {
  switch (menuState) {

    case MENU_SETTINGS_SELECT:
      menuBackToNormal();
      break;

    case MENU_SETTINGS_EDIT:
      loadSettings();
      menuState = MENU_SETTINGS_SELECT;
      beep(80);
      break;

    case MENU_CALIBRATION_SELECT:
      menuState = MENU_SETTINGS_SELECT;
      beep(80);
      break;

    case MENU_CAL_UART_COMPASS:
    case MENU_NMEA_MONITOR:
    case MENU_CAL_WIND_SPEED: // Terug naar het kalibratiemenu
      menuState = MENU_CALIBRATION_SELECT;
      beep(80);
      break;

    case MENU_NORMAL:
    default:
      break;

    case MENU_CAL_RUDDER_START:
    case MENU_CAL_RUDDER_PORT:
    case MENU_CAL_RUDDER_PORT_SAVED:
    case MENU_CAL_RUDDER_CENTER:
    case MENU_CAL_RUDDER_CENTER_SAVED:
    case MENU_CAL_RUDDER_STARBOARD:
    case MENU_CAL_RUDDER_SAVED:
      menuState = MENU_CALIBRATION_SELECT;
      beep(80);
      break;
  }
}

void menuShortLeft();
void menuShortRight();
void menuMinus10Short();
void menuPlus10Short();
void menuLongMinus10();
void menuLongMinus1();
void menuLongPlus1();
void menuLongPlus10();

void processRX500Event() {
  if (rx500Event == RX500_EVENT_NONE) return;

  RX500Event event = rx500Event;
  rx500Event = RX500_EVENT_NONE;

  switch (event) {
    case RX500_EVENT_SHORT_MINUS1:
      if (menuState == MENU_NORMAL) menuShortLeft();
      else menuMoveLeft();
      break;

    case RX500_EVENT_SHORT_PLUS1:
      if (menuState == MENU_NORMAL) menuShortRight();
      else menuMoveRight();
      break;

    case RX500_EVENT_SHORT_MINUS10:
      if (rudderCalibrationWaiting) {
        rudderCalibrationExitRequested = true;
        rx500OK = true;
      } else {
        menuMinus10Short();
      }
      break;

    case RX500_EVENT_SHORT_PLUS10:
      if (rudderCalibrationWaiting) {
        rx500OK = true;
      } else {
        menuPlus10Short();
      }
      break;

    case RX500_EVENT_DOUBLE_PLUS10:
      if (rudderCalibrationWaiting) {
        rudderCalibrationExitRequested = true;
        rx500OK = true;
      } else {
        menuBack();
      }
      break;

    case RX500_EVENT_LONG_MINUS10: menuLongMinus10(); break;
    case RX500_EVENT_LONG_MINUS1: menuLongMinus1(); break;
    case RX500_EVENT_LONG_PLUS1: menuLongPlus1(); break;
    case RX500_EVENT_LONG_PLUS10:
      if (rudderCalibrationWaiting) rx500OK = true;
      else menuLongPlus10();
      break;

    case RX500_EVENT_NONE:
    default:
      break;
  }
}

void menuShortLeft() {
  if (menuState == MENU_NORMAL) {
    if (!Auto_active) return;
    if (!Heading_valid) { beep(200); return; }
    normalSelection = 1;
    if (!isnan(Set_heading)) Set_heading = normalize360(Set_heading - 1);
    calculateHeadingError();
    beep(35);
  } else {
    menuMoveLeft();
  }
}

void menuShortRight() {
  if (menuState == MENU_NORMAL) {
    if (!Auto_active) return;
    if (!Heading_valid) { beep(200); return; }
    normalSelection = 2;
    if (!isnan(Set_heading)) Set_heading = normalize360(Set_heading + 1);
    calculateHeadingError();
    beep(35);
  } else {
    menuMoveRight();
  }
}

void menuMinus10Short() {
  if (menuState == MENU_NORMAL) {
    normalSelection = 0;
    if (tack_error == true) { tack_error = false; return; }
    Course_fault = false;
    Rudder_fault = false;
    menuSelectNormal();
  } else {
    menuBack();
  }
}

void menuPlus10Short() {
  if (menuState == MENU_NORMAL) {
    normalSelection = 3;
    menuSelectNormal();
  } else if (menuState == MENU_CAL_UART_COMPASS) {
    sendCompassTerminalCommand(compassTerminalCommands[compassTerminalSelection]);
    beep(60);
  } else {
    menuEnter();
  }
}

void menuLongMinus10() {
  if (menuState != MENU_NORMAL) return;
  if (Rudder_fault) Rudder_fault = false;
  if (Auto_active) autopilotOff();
  else menuEnterSettings();
}

void menuLongMinus1() {
  if (menuState == MENU_NORMAL) {
    if (!Auto_active) {
      if (isOpenCPNValid()) { currentNavMode = NAV_MODE_OPENCPN; beep(150); }
      else { beep(800); }
    }
    if (isWindValid == true && Auto_active) {
      steps = (Tack_angle / Tack_speed) * 5;
      tack_port = true;
      tack_starboard = false;
      beep(100);
    }
  }
}

void menuLongPlus1() {
  if (menuState == MENU_NORMAL) {
    if (!Auto_active) { currentNavMode = NAV_MODE_COMPASS; beep(150); }
    if (isWindValid == true && Auto_active) {
      steps = (Tack_angle / Tack_speed) * 5;
      tack_port = false;
      tack_starboard = true;
      beep(100);
    }
  }
}

void menuLongPlus10() {
  if (menuState == MENU_NORMAL) autopilotOn();
}