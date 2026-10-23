// ============================================================
// HEADING / AUTOPILOT UART compass
// ============================================================

bool compassCalibrationActive();
void headingReceived(float newHeading);
void calculateHeadingError();
float normalize360(float value);
float signedHeadingError(float target, float actual);



void headingReceived(float newHeading) {
  Heading = newHeading;
  lastHeadingMillis = millis();
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
