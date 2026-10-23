// ============================================================
// DISPLAY.H - CVBS display met Wind Speed Kalibratie Scherm
// ============================================================

#define NMEA_MONITOR_LINES 5
#define NMEA_MONITOR_LINE_LENGTH 82
char nmeaMonitorLines[NMEA_MONITOR_LINES][NMEA_MONITOR_LINE_LENGTH] = {};
char lastSeenNmea[NMEA_MONITOR_LINE_LENGTH] = "";
bool nmeaPaused = false;

// Function prototypes
void drawMainScreen();
void drawMainMenuBar(int x, int y, int w, int h);
void drawCompassRose(int x, int y, int radius);
void drawRudderIndicator(int x, int y);
void drawStatusBox(int x, int y, int w, int h);
const char *headingDirection16(int heading);
void drawSettingsScreen();
void drawCalibrationMenuScreen();
void drawUartCompassCalibrationScreen();
void drawNmeaMonitorScreen();
void drawWindSpeedCalibrationScreen();
void drawCalibrationScreen(const char *line1, const char *line2, const char *line3);

const float RUDDER_SCALE = 1.0f;

void setupVideo() {
  cvbs.init();
  screen.setColorDepth(8);
  screen.createSprite(320, 270);
  screen.fillScreen(TFT_BLACK);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.setTextSize(1);
  screen.setCursor(80, 105);
  screen.print("AUTOPILOT");
  screen.setCursor(108, 120);
  screen.print(SOFTWARE_VERSION);
  presentFrame();
  delay(1000);
}

void presentFrame() {
  if (flash_PAL == true) {
    screen.pushSprite(0, 0);
    cvbs.display();
  }
}

void updateDisplay() {
  static int lastMenuState = -1;
  static int lastSelection = -1;
  static bool lastFlash = false;

  if (menuState != lastMenuState) {
    lastMenuState = menuState;
    lastSelection = -1;
  }

  switch (menuState) {
    case MENU_NORMAL:
      drawMainScreen();
      break;

    case MENU_SETTINGS_SELECT:
    case MENU_SETTINGS_EDIT:
      drawSettingsScreen();
      break;

    case MENU_CALIBRATION_SELECT:
      drawCalibrationMenuScreen();
      break;

    case MENU_CAL_UART_COMPASS:
      drawUartCompassCalibrationScreen();
      break;

    case MENU_NMEA_MONITOR:
      drawNmeaMonitorScreen();
      break;

    case MENU_CAL_WIND_SPEED:
      drawWindSpeedCalibrationScreen();
      break;

    case MENU_CAL_RUDDER_START:
      {
        char currentPort[32], currentCenter[32], currentStar[32];
        snprintf(currentPort, sizeof(currentPort), "PORT   %d", Rudder_port_adc);
        snprintf(currentCenter, sizeof(currentCenter), "CENTER %d", Rudder_center_adc);
        snprintf(currentStar, sizeof(currentStar), "STBD   %d", Rudder_star_adc);

        screen.setFont(&fonts::FreeSansBold9pt7b);
        screen.fillScreen(TFT_BLACK);
        screen.setTextColor(TFT_WHITE, TFT_BLACK);
        screen.setTextSize(1);
        screen.setCursor(10, 25); screen.print("RUDDER CALIBRATION");
        screen.setCursor(10, 75); screen.print("CURRENT VALUES");
        screen.setCursor(10, 95); screen.print(currentPort);
        screen.setCursor(10, 115); screen.print(currentCenter);
        screen.setCursor(10, 135); screen.print(currentStar);
        screen.setCursor(10, 155); screen.print("LONG +10 = START");
        presentFrame();
        break;
      }

    case MENU_CAL_RUDDER_PORT:
      drawCalibrationScreen("MOVE FULL PORT", "LONG +10 = OK", "-10 = EXIT");
      presentFrame();
      break;

    case MENU_CAL_RUDDER_PORT_SAVED:
      {
        char portLine[32];
        snprintf(portLine, sizeof(portLine), "PORT   %d", Rudder_port_adc);
        drawCalibrationScreen("PORT VALUE SAVED", portLine, "LONG +10 = OK");
        presentFrame();
        break;
      }

    case MENU_CAL_RUDDER_CENTER:
      drawCalibrationScreen("MOVE CENTER", "LONG +10 = OK", "-10 = EXIT");
      presentFrame();
      break;

    case MENU_CAL_RUDDER_CENTER_SAVED:
      {
        char centerLine[32];
        snprintf(centerLine, sizeof(centerLine), "CENTER %d", Rudder_center_adc);
        drawCalibrationScreen("CENTER VALUE SAVED", centerLine, "LONG +10 = OK");
        presentFrame();
        break;
      }

    case MENU_CAL_RUDDER_STARBOARD:
      drawCalibrationScreen("MOVE FULL STARBOARD", "LONG +10 = OK", "-10 = EXIT");
      presentFrame();
      break;

    case MENU_CAL_RUDDER_SAVED:
      {
        char line1[32], line2[32], line3[32];
        snprintf(line1, sizeof(line1), "PORT   %d", Rudder_port_adc);
        snprintf(line2, sizeof(line2), "CENTER %d", Rudder_center_adc);
        snprintf(line3, sizeof(line3), "STBD   %d", Rudder_star_adc);

        screen.setFont(&fonts::FreeSansBold9pt7b);
        screen.fillScreen(TFT_BLACK);
        screen.setTextColor(TFT_WHITE, TFT_BLACK);
        screen.setTextSize(1);
        screen.setCursor(10, 25); screen.print("CALIBRATION SAVED");
        screen.setCursor(10, 75); screen.print(line1);
        screen.setCursor(10, 95); screen.print(line2);
        screen.setCursor(10, 115); screen.print(line3);
        screen.setCursor(10, 155); screen.print("SHORT -10 = EXIT");
        presentFrame();
        break;
      }
  }
}

void drawMainScreen() {
  screen.setFont(nullptr);
  screen.fillScreen(TFT_BLACK);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.setTextSize(1);

  drawCompassRose(75, 87, 75);
  drawRudderIndicator(240, 15);

  screen.setTextSize(1);
  screen.setCursor(155, 106);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.print("MODE:");

  screen.setCursor(190, 106);
  if (currentNavMode == NAV_MODE_OPENCPN) {
    screen.setTextColor(TFT_WHITE, TFT_BLACK);
    screen.print("WAYPOINT");
    screen.setCursor(250, 106);
    screen.print(RX_EData.RX_targetHeading);
  } else {
    screen.setTextColor(TFT_WHITE, TFT_BLACK);
    screen.print("COMPASS");
  }

  drawStatusBox(155, 118, 162, 50);

  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.setTextSize(3);
  screen.setCursor(10, 182);

  if (Heading_valid) {
    int hdg = ((int)round(Heading) % 360 + 360) % 360;
    if (hdg < 10) screen.print("00");
    else if (hdg < 100) screen.print("0");
    screen.print(hdg);
    screen.drawCircle(68, 186, 2, TFT_WHITE);
    screen.setCursor(78, 182);
    screen.print(headingDirection16(hdg));
  } else {
    screen.print("---");
  }

  drawMainMenuBar(2, 210, 316, 46);
  presentFrame();
}

void drawMainMenuBar(int x, int y, int w, int h) {
  const int buttonW = w / 4;
  const uint16_t cPort = TFT_RED;
  const uint16_t cMidleButton = TFT_GRAY;
  const uint16_t cStar = TFT_GREEN;

  for (int i = 0; i < 4; i++) {
    uint16_t c = (i == 0) ? cPort : (i == 3) ? cStar : cMidleButton;
    int bx = x + i * buttonW;
    screen.fillRoundRect(bx, y, buttonW - 2, h, 5, c);
  }

  int displaySelection = 0;
  bool buttonPressed = false;

  if (rx500PressCode != 0) {
    buttonPressed = true;
    if (rx500PressCode == RX500_CODE_MINUS10) displaySelection = 1;
    else if (rx500PressCode == RX500_CODE_MINUS1) displaySelection = 0;
    else if (rx500PressCode == RX500_CODE_PLUS1) displaySelection = 3;
    else if (rx500PressCode == RX500_CODE_PLUS10) displaySelection = 2;
  }

  if (buttonPressed) {
    int selectedX = x + displaySelection * buttonW;
    screen.drawRoundRect(selectedX, y, buttonW - 2, h, 6, TFT_WHITE);
    screen.drawRoundRect(selectedX + 1, y + 1, buttonW - 4, h - 2, 5, TFT_WHITE);
    screen.drawRoundRect(selectedX + 2, y + 2, buttonW - 6, h - 4, 4, TFT_WHITE);
    screen.drawRoundRect(selectedX + 3, y + 3, buttonW - 8, h - 6, 3, TFT_WHITE);
  }

  const char *buttonText[] = {
    (Auto_active && currentNavMode == NAV_MODE_COMPASS) ? "-1" : "",
    (Auto_active && currentNavMode == NAV_MODE_COMPASS) ? "-10" : "",
    (Auto_active && currentNavMode == NAV_MODE_COMPASS) ? "+10" : "",
    (Auto_active && currentNavMode == NAV_MODE_COMPASS) ? "-1" : "",
  };

  screen.setTextSize(2);

  for (int i = 0; i < 4; i++) {
    int bx = x + i * buttonW;
    uint16_t c = (i == 0) ? cPort : (i == 3) ? cStar : cMidleButton;
    int textW = screen.textWidth(buttonText[i]);
    int tx = bx + (buttonW - 2 - textW) / 2;

    screen.setTextColor(TFT_BLACK, c);
    screen.setCursor(tx, y + 4);
    screen.print(buttonText[i]);
  }

  const char *labels[] = {
    (isWindValid && Auto_active && currentNavMode == NAV_MODE_COMPASS) ? "TACK" : (!Auto_active) ? "Wayp" : "",
    (Auto_active) ? "OFF" : "SET",
    (!Auto_active) ? "ON" : "",
    (isWindValid && Auto_active && currentNavMode == NAV_MODE_COMPASS) ? "TACK" : (!Auto_active) ? "Comp" : ""
  };

  const uint16_t labelColors[] = { cPort, cMidleButton, cMidleButton, cStar };

  for (int i = 0; i < 4; i++) {
    int bx = x + i * buttonW;
    screen.setTextColor(TFT_BLACK, labelColors[i]);
    int textW = screen.textWidth(labels[i]);
    int tx = bx + (buttonW - 2 - textW) / 2;
    screen.setCursor(tx, y + 24);
    screen.print(labels[i]);
  }
}

const char *headingDirection16(int heading) {
  static const char *dirs[] = {
    "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
  };
  int index = ((heading + 11) / 22) % 16;
  return dirs[index];
}

void drawStatusBox(int x, int y, int w, int h) {
  uint16_t statusColor;
  const char *status;

  if (Rudder_fault) {
    status = "RUDDER ERROR";
    statusColor = (flash == true) ? TFT_RED : TFT_WHITE;
  } else if (Course_fault) {
    status = "COURSE ERROR";
    statusColor = (flash == true) ? TFT_RED : TFT_WHITE;
  } else if (tack_error) {
    status = "TACK STOPPED";
    statusColor = (flash == true) ? TFT_RED : TFT_WHITE;
  } else if (Auto_active) {
    status = "   AUTO     ";
    statusColor = TFT_GREEN;
  } else {
    status = "  STANDBY   ";
    statusColor = TFT_WHITE;
  }

  screen.drawRoundRect(x, y, w, h, 6, TFT_WHITE);
  if (flash == false && Auto_active == true) statusColor = TFT_BLACK;
  screen.setTextColor(statusColor, TFT_BLACK);

  screen.setTextSize(2);
  int charWidth = 12;
  int statusX = x + (w - ((int)strlen(status) * charWidth)) / 2;
  if (statusX < x + 2) statusX = x + 2;

  screen.setCursor(statusX, y + 6);
  screen.print(status);

  if (Auto_active) {
    screen.setTextColor(TFT_WHITE, TFT_BLACK);
    screen.drawCircle(x + 148, y + 31, 2, TFT_WHITE);
  } else {
    screen.setTextColor(TFT_BLACK, TFT_BLACK);
    screen.drawCircle(x + 148, y + 31, 2, TFT_BLACK);
  }

  screen.setTextSize(2);
  screen.setCursor(x + 22, y + 27);
  screen.print("COURSE:");

  int course = ((int)round(Set_heading) % 360 + 360) % 360;
  if (course < 10) screen.print("00");
  else if (course < 100) screen.print("0");
  screen.print(course);

  screen.setTextSize(1);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);
}

void drawCompassRose(int x, int y, int r) {
  const int cx = x;
  const int cy = y;

  screen.drawCircle(cx, cy, r, TFT_WHITE);
  screen.setTextSize(2);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);

  screen.setCursor(cx - 5, cy - r + 10); screen.print("N");
  screen.setCursor(cx - 5, cy + r - 22); screen.print("S");
  screen.setCursor(cx - r + 7, cy - 6);  screen.print("W");
  screen.setCursor(cx + r - 17, cy - 6); screen.print("E");

  for (int deg = 0; deg < 360; deg += 15) {
    float a = deg * PI / 180.0;
    int tickOuter = r - 2;
    int tickInner = (deg % 45 == 0) ? r - 10 : r - 7;
    int x1 = cx + (int)(sin(a) * (float)tickOuter);
    int y1 = cy - (int)(cos(a) * (float)tickOuter);
    int x2 = cx + (int)(sin(a) * (float)tickInner);
    int y2 = cy - (int)(cos(a) * (float)tickInner);
    screen.drawLine(x1, y1, x2, y2, TFT_WHITE);
  }

  if (Heading_valid) {
    float a = Heading * PI / 180.0;
    int hx = cx + (int)(sin(a) * (float)(r - 14));
    int hy = cy - (int)(cos(a) * (float)(r - 14));
    screen.drawLine(cx, cy, hx, hy, TFT_RED);
    screen.drawLine(cx + 1, cy, hx + 1, hy, TFT_RED);
    screen.drawLine(cx, cy + 1, hx, hy + 1, TFT_RED);
  }

  if (isWindValid == true) {
    screen.setTextColor(TFT_WHITE, TFT_BLACK);
    screen.setTextSize(1);
    screen.setCursor(130, 10); screen.print("WIND");
    screen.setCursor(130, 22); screen.print(windSpeed, 1); screen.print("kn  ");

    float a = windDirection * PI / 180.0;
    float s = sin(a);
    float c = cos(a);
    int windOuter = r - 12;
    int innerDist = r / 5;
    int tipX = cx + (int)(s * innerDist);
    int tipY = cy - (int)(c * innerDist);
    int outerX = cx + (int)(s * windOuter);
    int outerY = cy - (int)(c * windOuter);

    int spread = 6;
    int ox1 = outerX + (int)(c * spread);
    int oy1 = outerY + (int)(s * spread);
    int ox2 = outerX - (int)(c * spread);
    int oy2 = outerY - (int)(s * spread);

    screen.drawLine(tipX, tipY, ox1, oy1, TFT_WHITE);
    screen.drawLine(tipX + 1, tipY, ox1 + 1, oy1, TFT_WHITE);
    screen.drawLine(tipX, tipY, ox2, oy2, TFT_WHITE);
    screen.drawLine(tipX + 1, tipY, ox2 + 1, oy2, TFT_WHITE);
  }

  if (!isnan(Set_heading)) {
    float a = Set_heading * PI / 180.0;
    int x1 = cx + (int)(sin(a) * (float)(r - 2));
    int y1 = cy - (int)(cos(a) * (float)(r - 2));
    if (flash == true && Set_heading >= 180 && Set_heading <= 359 && Auto_active == true) screen.fillCircle(x1, y1, 3, TFT_RED);
    if (flash == true && Set_heading >= 0 && Set_heading <= 180 && Auto_active == true) screen.fillCircle(x1, y1, 3, TFT_GREEN);
  }

  screen.drawCircle(cx, cy, 3, TFT_WHITE);
  screen.drawCircle(cx, cy + 1, 3, TFT_WHITE);
  screen.drawCircle(cx, cy + 2, 3, TFT_WHITE);
}

void drawRudderIndicator(int x, int y) {
  float maxRudder = (float)Rudder_physical_max;
  if (maxRudder < 1.0f) maxRudder = 1.0f;

  int outerR = (int)(maxRudder * RUDDER_SCALE);
  int innerR = (int)(outerR * 0.78f);
  int cx = x;
  int cy = y + outerR;

  screen.fillRect(cx - outerR - 20, y, (outerR * 2) + 40, outerR + 35, TFT_BLACK);

  const int segments = 80;
  float startAngle = -90.0f - maxRudder;
  float endAngle = -90.0f + maxRudder;
  float angleStep = (endAngle - startAngle) / segments;

  for (int i = 0; i < segments; i++) {
    float angle1 = startAngle + (angleStep * i);
    float angle2 = startAngle + (angleStep * (i + 1));
    float rad1 = angle1 * DEG_TO_RAD;
    float rad2 = angle2 * DEG_TO_RAD;

    int x1o = cx + (int)(cos(rad1) * (float)outerR);
    int y1o = cy + (int)(sin(rad1) * (float)outerR);
    int x2o = cx + (int)(cos(rad2) * (float)outerR);
    int y2o = cy + (int)(sin(rad2) * (float)outerR);

    int x1i = cx + (int)(cos(rad1) * (float)innerR);
    int y1i = cy + (int)(sin(rad1) * (float)innerR);
    int x2i = cx + (int)(cos(rad2) * (float)innerR);
    int y2i = cy + (int)(sin(rad2) * (float)innerR);

    float midAngle = (angle1 + angle2) * 0.5f;
    float rudderAngle = midAngle + 90.0f;

    uint16_t colour = (rudderAngle < 0.0f) ? TFT_RED : TFT_GREEN;
    screen.fillTriangle(x1i, y1i, x1o, y1o, x2o, y2o, colour);
    screen.fillTriangle(x1i, y1i, x2o, y2o, x2i, y2i, colour);
  }

  float rudder = (float)Rudder_deg;
  if (rudder > maxRudder) rudder = maxRudder;
  if (rudder < -maxRudder) rudder = -maxRudder;

  float needleAngle = (-90.0f + rudder) * DEG_TO_RAD;
  float needleLength = outerR * 0.88f;

  int needleX = cx + (int)(cos(needleAngle) * needleLength);
  int needleY = cy + (int)(sin(needleAngle) * needleLength);

  screen.drawLine(cx, cy, needleX, needleY, TFT_WHITE);

  float arrowLength = outerR * 0.20f;
  float arrowWidth = outerR * 0.10f;
  float backX = (float)needleX - (cos(needleAngle) * arrowLength);
  float backY = (float)needleY - (sin(needleAngle) * arrowLength);
  float leftX = backX + (sin(needleAngle) * arrowWidth);
  float leftY = backY - (cos(needleAngle) * arrowWidth);
  float rightX = backX - (sin(needleAngle) * arrowWidth);
  float rightY = backY + (cos(needleAngle) * arrowWidth);

  screen.fillTriangle(needleX, needleY, (int)leftX, (int)leftY, (int)rightX, (int)rightY, TFT_WHITE);

  int centerR = max(2, (int)(outerR * 0.08f));
  screen.fillCircle(cx, cy, centerR, TFT_WHITE);
  screen.setTextSize(1);
  int maxR = (int)round(maxRudder);
  char buf[12];
  sprintf(buf, "%d", maxR);

  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.setTextDatum(MC_DATUM);
  screen.drawString(buf, cx - (int)(outerR * 0.72f), cy - (int)(outerR * 0.18f));
  screen.drawString(buf, cx + (int)(outerR * 0.72f), cy - (int)(outerR * 0.18f));
  screen.setTextDatum(TL_DATUM);
}

const char *settingDescriptions[] = {
  "Steering sensitivity.\nPercentage 0-300%",
  "Course deadband angle.\nDegrees ",
  "Max. motor drive speed.\nMax. motor speed in percentage.\n0-100%",
  "Min. motor drive speed.\nMin. motor speed in percentage.\n0-100%",
  "Max rudder movement time.\nEnter in seconds",
  "Max course error time.\nEnter in seconds",
  "Minimum helm angle\nMin. rudder to change heading\nDegrees",
  "Maximum helm angle\nMax. rudder to change heading\nDegrees",
  "Physical rudder limit.\nDegrees",
  "Rudder center deadband.\nLow values=more corrections",
  "Backlash correction.\nDegrees",
  "Tack angle in degrees.",
  "Tack steering speed.\nDegrees per second",
  "Response span.\nDegrees of rudder error for\nmaximum motor output.",
  "Open calibration menu.\nRudder & UARTCOMPAS",
  "Factory reset settings.\nHandle with care!"
};

void drawSettingsScreen() {
  screen.setFont(&fonts::FreeSansBold9pt7b);
  screen.fillScreen(TFT_BLACK);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.setTextSize(1);

  screen.setCursor(5, 12); screen.print("AUTOPILOT SETTINGS");
  screen.setCursor(270, 12); screen.print(SOFTWARE_VERSION);
  screen.setCursor(5, 46); screen.print(settingNames[settingsSelection]);
  screen.setCursor(250, 46);

  if (flash && menuState == MENU_SETTINGS_EDIT) {
    screen.setTextColor(TFT_BLACK, TFT_BLACK);
  } else {
    screen.setTextColor(TFT_WHITE, TFT_BLACK);
  }

  switch (settingsSelection) {
    case 0: screen.print((int)Pilot_gain); break;
    case 1: screen.print((int)Course_window); break;
    case 2: screen.print(Helm_speed); break;
    case 3: screen.print(Low_speed); break;
    case 4: screen.print(Rudder_timeout); break;
    case 5: screen.print(Heading_error_timeout); break;
    case 6: screen.print((int)Minimum_rudder); break;
    case 7: screen.print((int)Maximum_rudder); break;
    case 8: screen.print((int)Rudder_physical_max); break;
    case 9: screen.print((int)Helm_window); break;
    case 10: screen.print(Pilot_comp, 1); break;
    case 11: screen.print((int)Tack_angle); break;
    case 12: screen.print(Tack_speed); break;
    case 13: screen.print((int)Response_Span); break;
    case 14: screen.print("OPEN"); break;
    case 15: screen.print("RESET"); break;
  }

  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.drawRoundRect(2, 40, 315, 24, 4, TFT_WHITE);
  screen.setCursor(5, 74);
  if (menuState == MENU_SETTINGS_SELECT) screen.print("SELECT VALUE = -1 +1  EDIT = +10");
  else screen.print("CHANGE VALUE  -1 +1");

  screen.setCursor(5, 92);
  if (settingsSelection == 14) screen.print("+10 = CALIBRATION MENU");
  else if (settingsSelection == 15) screen.print("+10 = FACTORY SETTINGS");
  else if (menuState == MENU_SETTINGS_EDIT) screen.print("+10 = SAVE");
  else screen.print("-10 = BACK");

  screen.drawRoundRect(2, 160, 315, 80, 4, TFT_WHITE);
  screen.setTextColor(TFT_YELLOW, TFT_BLACK);
  screen.setCursor(8, 142); screen.print("INFO:");
  screen.setTextColor(TFT_WHITE, TFT_BLACK);

  String desc = settingDescriptions[settingsSelection];
  int startY = 162, lineHeight = 18, currentIndex = 0, lineCount = 0;

  while (true) {
    int nextNewline = desc.indexOf('\n', currentIndex);
    String currentLine = (nextNewline != -1) ? desc.substring(currentIndex, nextNewline) : desc.substring(currentIndex);
    if (lineCount < 3) {
      screen.setCursor(8, startY + (lineCount * lineHeight));
      screen.print(currentLine);
      lineCount++;
    }
    if (nextNewline == -1) break;
    currentIndex = nextNewline + 1;
  }
  presentFrame();
}

void drawCalibrationMenuScreen() {
  screen.setFont(&fonts::FreeSansBold9pt7b);
  screen.fillScreen(TFT_BLACK);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.setTextSize(1);

  screen.setCursor(5, 12); screen.print("CALIBRATION");

  screen.setCursor(20, 42); screen.print(calibrationSelection == 0 ? "> RUDDER" : "  RUDDER");
  screen.setCursor(20, 62); screen.print(calibrationSelection == 1 ? "> UART COMPASS" : "  UART COMPASS");
  screen.setCursor(20, 82); screen.print(calibrationSelection == 2 ? "> NMEA MONITOR" : "  NMEA MONITOR");
  screen.setCursor(20, 102); screen.print(calibrationSelection == 3 ? "> WIND SPEED" : "  WIND SPEED"); // 4e Optie!

  screen.setCursor(5, 127); screen.print("L/R SELECT  +10 ENTER");
  screen.setCursor(5, 147); screen.print("-10 = SAVE & EXIT");

  presentFrame();
}

void drawUartCompassCalibrationScreen() {
  screen.setFont(&fonts::FreeSansBold9pt7b);
  screen.setTextSize(1);
  screen.fillScreen(TFT_BLACK);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);

  screen.setCursor(10, 18); screen.print("UART 20948 COMPASS");
  screen.setCursor(10, 50); screen.print("COMMAND:");
  screen.setCursor(130, 50); screen.print(compassTerminalCommands[compassTerminalSelection]);

  screen.drawRoundRect(1, 90, 319, 120, 6, TFT_WHITE);
  screen.setTextColor(TFT_YELLOW, TFT_BLACK);
  for (int i = 0; i < COMPASS_TERMINAL_LINES; i++) {
    screen.setCursor(10, 100 + (i * 18));
    screen.print(compassTerminalLines[i]);
  }

  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.setCursor(10, 212);
  screen.print(" -1/+1 = CMD +10=SEND -10=EXIT");
  presentFrame();
}

void drawNmeaMonitorScreen() {
  static bool initialized = false;
  static char lastDrawnLines[NMEA_MONITOR_LINES][NMEA_MONITOR_LINE_LENGTH];
  static bool lastPausedState = false;
  static unsigned long lastPressTime = 0;
  static int previousMenuState = -1;

  bool needsRedraw = false;
  if (menuState != previousMenuState) {
    previousMenuState = menuState;
    needsRedraw = true;
  }

  if (rx500PressCode == RX500_CODE_PLUS1) {
    if (millis() - lastPressTime > 300) {
      nmeaPaused = !nmeaPaused;
      lastPressTime = millis();
      needsRedraw = true;
    }
    rx500PressCode = 0;
  }

  if (nmeaPaused != lastPausedState) {
    lastPausedState = nmeaPaused;
    needsRedraw = true;
  }

  if (!nmeaPaused) {
    if (strlen(RX_EData.RX_NMEA) > 0 && strcmp(RX_EData.RX_NMEA, lastSeenNmea) != 0) {
      strncpy(lastSeenNmea, RX_EData.RX_NMEA, sizeof(lastSeenNmea) - 1);
      lastSeenNmea[sizeof(lastSeenNmea) - 1] = '\0';
      for (int i = 0; i < NMEA_MONITOR_LINES - 1; i++) {
        strcpy(nmeaMonitorLines[i], nmeaMonitorLines[i + 1]);
      }
      strncpy(nmeaMonitorLines[NMEA_MONITOR_LINES - 1], lastSeenNmea, NMEA_MONITOR_LINE_LENGTH - 1);
      nmeaMonitorLines[NMEA_MONITOR_LINES - 1][NMEA_MONITOR_LINE_LENGTH - 1] = '\0';
      needsRedraw = true;
    }
  }

  for (int i = 0; i < NMEA_MONITOR_LINES; i++) {
    if (strcmp(nmeaMonitorLines[i], lastDrawnLines[i]) != 0) {
      needsRedraw = true;
      break;
    }
  }

  if (initialized && !needsRedraw) return;

  for (int i = 0; i < NMEA_MONITOR_LINES; i++) {
    strcpy(lastDrawnLines[i], nmeaMonitorLines[i]);
  }
  initialized = true;

  screen.fillScreen(TFT_BLACK);
  screen.setFont(&fonts::FreeSansBold9pt7b);
  screen.setCursor(10, 18);
  if (nmeaPaused) {
    screen.setTextColor(TFT_YELLOW, TFT_BLACK);
    screen.print("NMEA MONITOR [PAUSED]");
  } else {
    screen.setTextColor(TFT_WHITE, TFT_BLACK);
    screen.print("NMEA MONITOR");
  }

  screen.drawFastHLine(0, 38, 320, TFT_WHITE);
  screen.drawFastHLine(0, 208, 320, TFT_WHITE);
  screen.setFont(&fonts::Font2);
  screen.setTextSize(1);

  for (int i = 0; i < NMEA_MONITOR_LINES; i++) {
    screen.setCursor(10, 44 + (i * 30));
    if (strlen(nmeaMonitorLines[i]) > 0) {
      screen.setTextColor(TFT_WHITE, TFT_BLACK);
      screen.print(nmeaMonitorLines[i]);
    } else {
      screen.setTextColor(TFT_DARKGREY, TFT_BLACK);
      screen.print("Wachten op NMEA data...");
    }
  }

  screen.setFont(&fonts::FreeSansBold9pt7b);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.setCursor(10, 235);
  screen.print("+1=PAUSE  -10=EXIT");
  presentFrame();
}

// ------------------------------------------------------------
// HIER TEKENEN WE HET NIEUWE WIND SPEED KALIBRATIESCHERM
// ------------------------------------------------------------
void drawWindSpeedCalibrationScreen() {
  screen.setFont(&fonts::FreeSansBold9pt7b);
  screen.setTextSize(1);
  screen.fillScreen(TFT_BLACK);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);

  screen.setCursor(10, 18);
  screen.print("WIND SPEED CALIBRATION");

  screen.setCursor(10, 50);
  screen.print("SET GAIN: Value 0.05 - 5.00");
  screen.setCursor(120, 50);
  

  screen.drawRoundRect(1, 90, 319, 100, 6, TFT_WHITE);

  screen.setTextColor(TFT_YELLOW, TFT_BLACK);
  screen.setCursor(10, 110);
  screen.print("WIND NODE RESP:");

  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.setCursor(10, 140);
  
  // We tonen nu direct de terminalResponse, en als die leeg is, de laatste regel uit de compass terminal array!
  if (strlen(RX_EData.RX_terminalResponse) > 0) {
    screen.print(RX_EData.RX_terminalResponse);
  } else {
    // Fallback naar de laatst opgeslagen terminal regel in de AP
    screen.print(compassTerminalLines[COMPASS_TERMINAL_LINES - 1]);
  }

  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.setCursor(10, 212);
  screen.print("-1/+1=GAIN  +10=SAVE  -10=EXIT");

  presentFrame();
}

void drawCalibrationScreen(const char *line1, const char *line2, const char *line3) {
  screen.setFont(&fonts::FreeSansBold9pt7b);
  screen.fillScreen(TFT_BLACK);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.setTextSize(1);
  screen.setCursor(10, 25); screen.print("RUDDER CALIBRATION");
  screen.setCursor(10, 75); screen.print(line1);
  screen.setCursor(10, 95); screen.print(line2);
  screen.setCursor(10, 115); screen.print(line3);
  presentFrame();
}