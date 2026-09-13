// Functies uit V28; inhoudelijk ongewijzigd.

void readRudder() {
  int adc = analogRead(PIN_RUDDER);

  if (!rudderAdcInitialized) {
    for (int i = 0; i < RUDDER_AVG_SAMPLES; i++)
      rudderAdcBuffer[i] = adc;

    rudderAdcInitialized = true;
  }

  rudderAdcBuffer[rudderAdcIndex] = adc;
  rudderAdcIndex++;

  if (rudderAdcIndex >= RUDDER_AVG_SAMPLES)
    rudderAdcIndex = 0;

  float adcAverage = 0.0;

  for (int i = 0; i < RUDDER_AVG_SAMPLES; i++)
    adcAverage += rudderAdcBuffer[i];

  adcAverage /= RUDDER_AVG_SAMPLES;

  Rudder_deg = rudderAdcToDegrees((int)round(adcAverage));

  Rudder_deg = constrain(Rudder_deg,
                         Rudder_port_deg,
                         Rudder_star_deg);
}

float rudderAdcToDegrees(int adc) {
  if (Rudder_port_adc == Rudder_center_adc || Rudder_center_adc == Rudder_star_adc)
    return 0.0;

  if (Rudder_port_adc < Rudder_center_adc) {
    if (adc <= Rudder_center_adc) {
      float f = (float)(adc - Rudder_center_adc) / (float)(Rudder_port_adc - Rudder_center_adc);
      return Rudder_center_deg + f * (Rudder_port_deg - Rudder_center_deg);
    }
  } else {
    if (adc >= Rudder_center_adc) {
      float f = (float)(adc - Rudder_center_adc) / (float)(Rudder_port_adc - Rudder_center_adc);
      return Rudder_center_deg + f * (Rudder_port_deg - Rudder_center_deg);
    }
  }

  float f = (float)(adc - Rudder_center_adc) / (float)(Rudder_star_adc - Rudder_center_adc);
  return Rudder_center_deg + f * (Rudder_star_deg - Rudder_center_deg);
}




bool waitForOK() {
  
  rudderCalibrationWaiting = true;
  rx500OK = false;

  while (!rx500OK) {
    processRX500();
    delay(10);

    if (rudderCalibrationExitRequested) {
      rudderCalibrationExitRequested = false;
      rudderCalibrationWaiting = false;
      rx500OK = false;
      return false;
    }
  }

  rx500OK = false;
  rudderCalibrationWaiting = false;
  return true;
}

void calibrateRudder() {
  autopilotOff();
  rudderCalibrationExitRequested = false;


//-----------------------rudder port calibration -----------------
  drawCalibrationScreen(
    "MOVE FULL PORT",
    "LONG +10 = OK",
    "");

  if (!waitForOK()) {
    hmiMode = HMI_SETTINGS_SELECT;
    return;
  }

  Rudder_port_adc = analogRead(PIN_RUDDER);

  char portLine[32];
  snprintf(portLine, sizeof(portLine), "PORT   %d", Rudder_port_adc);
  drawCalibrationScreen("PORT VALUE SAVED", portLine, "LONG +10 = OK");

//-----------------------rudder center calibration -----------------
  drawCalibrationScreen(
    "MOVE CENTER",
    "LONG +10 = OK",
    "");

  if (!waitForOK()) {
    hmiMode = HMI_SETTINGS_SELECT;
    return;
  }

  Rudder_center_adc = analogRead(PIN_RUDDER);


  char centerLine[32];
  snprintf(centerLine, sizeof(centerLine), "CENTER %d", Rudder_center_adc);
  drawCalibrationScreen("CENTER VALUE SAVED", centerLine, "LONG +10 = OK");


//-----------------------rudder starboard calibration -----------------
  drawCalibrationScreen(
    "MOVE FULL STARBOARD",
    "LONG +10 = OK",
    "");

  if (!waitForOK()) {
  hmiMode = HMI_SETTINGS_SELECT;
  return;
}

Rudder_star_adc = analogRead(PIN_RUDDER);

  char starLine[32];
  snprintf(starLine, sizeof(starLine), "STBD   %d", Rudder_star_adc);
  drawCalibrationScreen("STARBOARD VALUE SAVED", starLine, "LONG +10 = OK");
// -------------------------------------------------------------------------
  saveRudderCalibration();

  char line1[32];
  char line2[32];
  char line3[32];

  snprintf(line1, sizeof(line1), "PORT   %d", Rudder_port_adc);
  snprintf(line2, sizeof(line2), "CENTER %d", Rudder_center_adc);
  snprintf(line3, sizeof(line3), "STBD   %d", Rudder_star_adc);

  drawCalibrationScreen(
    "CALIBRATION SAVED",
    line1,
    line2);

  screen.setCursor(10, 135);
  screen.print(line3);
  screen.setCursor(10, 155);
  screen.print("DOUBLE +10 = EXIT");
  presentFrame();

  beep(80);
  delay(1000);
  hmiMode = HMI_SETTINGS_SELECT;
}

void saveRudderCalibration() {
  prefs.putInt("rport", Rudder_port_adc);
  prefs.putInt("rcenter", Rudder_center_adc);
  prefs.putInt("rstar", Rudder_star_adc);
}

void loadRudderCalibration() {
  Rudder_port_adc =
    prefs.getInt("rport", 1000);

  Rudder_center_adc =
    prefs.getInt("rcenter", 2048);

  Rudder_star_adc =
    prefs.getInt("rstar", 3000);
}
