// V40 HMI preparation: COURSE ERROR TIME is displayed but not used by control logic yet.
#ifndef HEADING_ERROR_TIMEOUT_HMI_DEFINED
#define HEADING_ERROR_TIMEOUT_HMI_DEFINED
int Heading_error_timeout = 5; // seconds; activate in V40 control logic later
#endif

// Functies uit V28; inhoudelijk ongewijzigd.

void setupVideo() {
  // Current M5GFX contains its own Panel_CVBS implementation.
  // GPIO26 is explicitly selected above.
  cvbs.init();

  // 8-bit canvas keeps RAM use reasonable.
  screen.setColorDepth(8);
  screen.createSprite(256, 240);

  screen.fillScreen(TFT_BLACK);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.setTextSize(1);
  screen.setCursor(80, 105);
  screen.print("AUTOPILOT");
  screen.setCursor(108, 120);
  screen.print(SOFTWARE_VERSION);

  screen.pushSprite(0, 0);
  cvbs.display();

  delay(500);
}

void presentFrame() {
  screen.pushSprite(0, 0);
  cvbs.display();
}

void updateDisplay() {
  if (hmiMode == HMI_NORMAL)
    drawMainScreen();
  else
    drawSettingsScreen();
}

void drawMainScreen() {
  screen.fillScreen(TFT_BLACK);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.setTextSize(1);

  drawCompassRose();
  drawRudderIndicator();

  // Current heading.
  screen.setCursor(145, 12);
  screen.print("HDG");

  screen.setCursor(145, 24);
  if (Heading_valid)
    screen.print((int)round(Heading));
  else
    screen.print("---");

  // Set heading.
  screen.setCursor(145, 46);
  screen.print("SET");

  screen.setCursor(145, 58);
  if (!Auto_active || isnan(Set_heading))
    screen.print("---");
else
    screen.print((int)round(Set_heading));

  // Heading error.
  screen.setCursor(145, 80);
  screen.print("ERR");

  screen.setCursor(145, 92);
  screen.print((int)round(Heading_error));

  // Rudder.
  screen.setCursor(145, 114);
  screen.print("RUD");

  screen.setCursor(145, 126);
  screen.print((int)round(Rudder_deg));

  // Target rudder angle.
  screen.setCursor(145, 148);
  screen.print("TGT");

  screen.setCursor(145, 160);
  screen.print((int)round(Target_rudder_deg));

  // Status.
  screen.drawRect(140, 145, 95, 25, TFT_WHITE);
  screen.setCursor(163, 154);

  if (Rudder_fault)
    screen.print("ERROR");
  else if (Auto_active)
    screen.print("AUTO");
  else
    screen.print("STBY");

  // Movement indicator.
  if (Auto_active && fabs(Heading_error) > Course_window) {
    screen.setCursor(20, 184);

    if (Heading_error > 0)
      screen.print(">>>");
    else
      screen.print("<<<");
  }

  // Old context-sensitive virtual [Rr]emote bediening.
  const char *functions[] = { "-10", "-1", "STOP", "+1", "+10" };
  const int x[] = { 18, 60, 98, 154, 202 };
  const int sx[] = { 10, 50, 88, 142, 190 };
  const int sw[] = { 35, 35, 55, 48, 46 };

  screen.drawRect(10, 205, 236, 30, TFT_WHITE);
  screen.drawRect(sx[normalSelection], 203, sw[normalSelection], 30, TFT_WHITE);

  screen.setCursor(x[normalSelection], 214);
  screen.print(functions[normalSelection]);

  screen.setCursor(10, 192);
  screen.print("L/R SELECT  OK EXEC");

  screen.setCursor(218, 192);
  screen.print(SOFTWARE_VERSION);

  presentFrame();
}

void drawCompassRose() {
  const int cx = 62;
  const int cy = 74;
  const int r = 54;

  screen.drawCircle(cx, cy, r, TFT_WHITE);
  screen.drawCircle(cx, cy, r / 2, TFT_WHITE);

  screen.setCursor(cx - 3, cy - r + 3);
  screen.print("N");

  screen.setCursor(cx - 3, cy + r - 10);
  screen.print("S");

  screen.setCursor(cx - r + 3, cy - 4);
  screen.print("W");

  screen.setCursor(cx + r - 7, cy - 4);
  screen.print("E");

  // 30 degree tick marks.
  for (int deg = 0; deg < 360; deg += 30) {
    float a = deg * PI / 180.0;

    int x1 = cx + (int)(sin(a) * (r - 4));
    int y1 = cy - (int)(cos(a) * (r - 4));

    int x2 = cx + (int)(sin(a) * (r - 10));
    int y2 = cy - (int)(cos(a) * (r - 10));

    screen.drawLine(x1, y1, x2, y2, TFT_WHITE);
  }

  // Current heading needle.
  if (Heading_valid) {
    float a = Heading * PI / 180.0;

    int x = cx + (int)(sin(a) * (r - 15));
    int y = cy - (int)(cos(a) * (r - 15));

    screen.drawLine(cx, cy, x, y, TFT_WHITE);
  }

  // Set-heading marker.
  if (!isnan(Set_heading)) {
    float a = Set_heading * PI / 180.0;

    int x1 = cx + (int)(sin(a) * (r - 2));
    int y1 = cy - (int)(cos(a) * (r - 2));

    int x2 = cx + (int)(sin(a) * (r - 11));
    int y2 = cy - (int)(cos(a) * (r - 11));

    screen.drawLine(x1, y1, x2, y2, TFT_WHITE);
  }

  screen.fillCircle(cx, cy, 3, TFT_WHITE);
}

void drawRudderIndicator() {
  const int cx = 205;
  const int cy = 55;

  screen.setCursor(177, 5);
  screen.print("RUDDER");

  screen.setCursor(175, 62);
  screen.print("P");

  screen.setCursor(231, 62);
  screen.print("S");

  screen.drawCircle(cx, cy, 27, TFT_WHITE);
  screen.drawCircle(cx, cy, 18, TFT_WHITE);

  screen.drawLine(cx, cy - 30, cx, cy - 18, TFT_WHITE);

  float normalized = 0.0;

  if (Maximum_rudder > 0.0)
    normalized = constrain(Rudder_deg / Maximum_rudder, -1.0, 1.0);

  float angleDeg = normalized * 30.0;
  float a = angleDeg * PI / 180.0;

  int x = cx + (int)(sin(a) * 25);
  int y = cy - (int)(cos(a) * 25);

  screen.drawLine(cx, cy, x, y, TFT_WHITE);
  screen.fillCircle(cx, cy, 3, TFT_WHITE);
}

void drawSettingsScreen() {
  screen.fillScreen(TFT_BLACK);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.setTextSize(1);

  screen.setCursor(5, 5);
  screen.print("AUTOPILOT SETTINGS");

  const char *names[] = {
    "GAIN",
    "COURSE WIN",
    "HELM SPEED",
    "HELM WIN",
    "RUDDER TIMEOUT",
    "COURSE ERROR TIME",
    "MIN RUDDER",
    "MAX RUDDER",
    "LOW SPEED",
    "PILOT COMP",
    "TACK ANGLE",
    "TACK SPEED",
    "RUDDER CAL",
    "FACTORY RESET"
  };

  screen.setCursor(5, 28);
  screen.print(names[settingsSelection]);

  screen.setCursor(150, 28);

  switch (settingsSelection) {
    case 0: screen.print((int)Pilot_gain); break;
    case 1: screen.print((int)Course_window); break;
    case 2: screen.print(Helm_speed); break;
    case 3: screen.print((int)Helm_window); break;
    case 4: screen.print(Rudder_timeout); break;
    case 5: screen.print(Heading_error_timeout); break;
    case 6: screen.print((int)Minimum_rudder); break;
    case 7: screen.print((int)Maximum_rudder); break;
    case 8: screen.print(Low_speed); break;
    case 9: screen.print(Pilot_comp, 1); break;
    case 10: screen.print((int)Tack_angle); break;
    case 11: screen.print(Tack_speed); break;
    case 12: screen.print("START"); break;
    case 13: screen.print("RESET"); break;
  }

  screen.drawLine(5, 48, 250, 48, TFT_WHITE);

  screen.setCursor(5, 60);

  if (hmiMode == HMI_SETTINGS_SELECT)
    screen.print("L/R SELECT  OK EDIT");
  else
    screen.print("L/R CHANGE  OK SAVE");

  screen.setCursor(5, 78);

  if (settingsSelection == 12)
    screen.print("OK = 3 POINT RUDDER CAL");
  else if (settingsSelection == 13)
    screen.print("OK = FACTORY SETTINGS");
  else
    screen.print("DOUBLE OK = EXIT");

  screen.setCursor(5, 105);
  screen.print("HDG ");

  if (Heading_valid)
    screen.print((int)round(Heading));
  else
    screen.print("---");

  screen.print("  RUD ");
  screen.print((int)round(Rudder_deg));

  screen.setCursor(5, 125);
  screen.print("SIM HDG / MOTOR TEST");

  if (Heading_valid)
    screen.print(" OK");
  else
    screen.print(" WAIT");

  presentFrame();
}

void drawCalibrationScreen(const char *line1,
                           const char *line2,
                           const char *line3) {
  screen.fillScreen(TFT_BLACK);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.setTextSize(1);

  screen.setCursor(10, 25);
  screen.print("RUDDER CALIBRATION");

  screen.setCursor(10, 75);
  screen.print(line1);

  screen.setCursor(10, 95);
  screen.print(line2);

  screen.setCursor(10, 115);
  screen.print(line3);

  presentFrame();
}
