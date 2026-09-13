// ============================================================
// HEADING / AUTOPILOT - integrated ICM20948 version
// ============================================================

void processCompassCommand(const String& command);
bool compassCalibrationActive();

void headingReceived(float newHeading);
void calculateHeadingError();
float normalize360(float value);
float signedHeadingError(float target, float actual);

void processSerialCommands() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toUpperCase();

  if (cmd == "C" || cmd == "W" || cmd == "L" || cmd == "CLEARCAL" || cmd == "DEBUGON" || cmd == "DEBUGOFF") {
    processCompassCommand(cmd);
    return;
  }

  if (cmd == "S") {
    if (compassCalibrationActive()) {
      processCompassCommand(cmd);
      return;
    }
    autopilotOff();
    return;
  }

  if (cmd == "A") {
    if (Heading_valid) autopilotOn();
    return;
  }

  // Hxxx simulator commands intentionally removed.
}

void headingReceived(float newHeading) {
  Heading = normalize360(newHeading);
  Heading_valid = true;
  lastHeadingMillis = millis();

  if (isnan(Set_heading) && !Auto_active)
    Set_heading = Heading;

  calculateHeadingError();

  
}

void calculateHeadingError() {
  if (!Heading_valid || isnan(Set_heading)) {
    Heading_error = 0;
    return;
  }
  Heading_error = signedHeadingError(Set_heading, Heading);
}

float normalize360(float value) {
  while (value < 0) value += 360.0;
  while (value >= 360.0) value -= 360.0;
  return value;
}

float signedHeadingError(float target, float actual) {
  float error = normalize360(target) - normalize360(actual);
  if (error > 180.0) error -= 360.0;
  if (error < -180.0) error += 360.0;
  return error;
}
