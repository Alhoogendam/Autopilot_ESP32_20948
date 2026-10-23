#define RUDDER_AVG_SAMPLES 5

float rudderAdcBuffer[RUDDER_AVG_SAMPLES] = { 0 };
int rudderAdcIndex = 0;
bool rudderAdcInitialized = false;

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

void calibrateRudder() {
  // Start de kalibratie in de eerste state via de state-machine
  menuState = MENU_CAL_RUDDER_START;
}

void saveRudderCalibration() {
  prefs.putInt("rport", Rudder_port_adc);
  prefs.putInt("rcenter", Rudder_center_adc);
  prefs.putInt("rstar", Rudder_star_adc);
}

void loadRudderCalibration() {
  Rudder_port_adc = prefs.getInt("rport", 1000);
  Rudder_center_adc = prefs.getInt("rcenter", 2048);
  Rudder_star_adc = prefs.getInt("rstar", 3000);
}