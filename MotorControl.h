// Functies uit V28; inhoudelijk ongewijzigd.
unsigned long courseErrorStartMillis = 0;
bool courseErrorActive = false;

void courseTimeoutFault() {
  stopMotor();
  digitalWrite(PIN_CLUTCH, LOW);

  Auto_active = false;
  Rudder_fault = true;

  rudderMoveActive = false;
  courseErrorActive = false;
  courseErrorStartMillis = 0;

  beep(200);
  delay(150);
  beep(200);
}
void rudderTimeoutFault() {
  stopMotor();
  digitalWrite(PIN_CLUTCH, LOW);
  Auto_active = false;
  Rudder_fault = true;
  rudderMoveActive = false;
  beep(200);
  delay(150);
  beep(200);
}

// Explicit command: store the currently valid compass heading as SET.
// This function must only be called by the dedicated SET command/button.
void setCurrentHeading() {

  if (!Heading_valid)
    return;

  Set_heading = Heading;
}

void autopilotOn() {

  // AP may not start without an explicitly assigned SET heading.
  if (!Heading_valid || isnan(Set_heading))
    return;

  Auto_active = true;

  courseErrorActive = false;
  courseErrorStartMillis = 0;

  digitalWrite(PIN_CLUTCH, HIGH);

  beep(50);
}
void autopilotOff() {
  Auto_active = false;
  Rudder_fault = false;

  rudderMoveActive = false;
  courseErrorActive = false;
  courseErrorStartMillis = 0;

  // Motor OFF first, clutch OFF second.
  stopMotor();
  digitalWrite(PIN_CLUTCH, LOW);

  beep(80);
}

void stopMotor() {
  ledcWrite(PIN_MOTOR_RPWM, 0);
  ledcWrite(PIN_MOTOR_LPWM, 0);
}

void setMotor(int direction, int pwm) {
  if (!Auto_active || !Heading_valid) {
    stopMotor();
    digitalWrite(PIN_CLUTCH, LOW);
    return;
  }

  digitalWrite(PIN_CLUTCH, HIGH);

  if (!rudderMoveActive) {
    rudderMoveActive = true;
    rudderMoveStartMillis = millis();
  }

  if (Rudder_timeout > 0 && millis() - rudderMoveStartMillis >= (unsigned long)Rudder_timeout * 1000UL) {
    rudderTimeoutFault();
    return;
  }

  if (direction == MOTOR_DIR_STARBOARD) {
    // Only RPWM is active.
    ledcWrite(PIN_MOTOR_LPWM, 0);
    ledcWrite(PIN_MOTOR_RPWM, pwm);
  } else {
    // Only LPWM is active.
    ledcWrite(PIN_MOTOR_RPWM, 0);
    ledcWrite(PIN_MOTOR_LPWM, pwm);
  }
}

void controlAutopilot() {
  if (!Auto_active || !Heading_valid || isnan(Set_heading)) {
    stopMotor();
    digitalWrite(PIN_CLUTCH, LOW);
    return;
  }

  calculateHeadingError();

  float errorAbs = fabs(Heading_error);

  if (errorAbs <= Course_window) {
    stopMotor();
    digitalWrite(PIN_CLUTCH, HIGH);

    rudderMoveActive = false;

    // Course is back within the allowed window.
    courseErrorActive = false;
    courseErrorStartMillis = 0;

    return;
  }

  // Start the course-error timer when the course leaves
  // the permitted Course Window.
  if (!courseErrorActive) {
    courseErrorActive = true;
    courseErrorStartMillis = millis();
  }

  // Safety timeout for a persistent course error.
  if (Heading_error_timeout > 0 && millis() - courseErrorStartMillis >= (unsigned long)Heading_error_timeout * 1000UL) {

    courseTimeoutFault();
    return;
  }

  // The compass error determines the theoretical rudder angle.
  // Pilot_comp adds a fixed mechanical correction to that angle.
  Target_rudder_deg = constrain(
    Heading_error + Pilot_comp,
    -Maximum_rudder,
    Maximum_rudder);

  // Rudder_deg is the measured rudder position.  The motor runs
  // until the measured position reaches the compensated target.
  float rudderError = Target_rudder_deg - Rudder_deg;
  float rudderErrorAbs = fabs(rudderError);

  if (rudderErrorAbs <= Helm_window) {
    stopMotor();
    digitalWrite(PIN_CLUTCH, HIGH);
    rudderMoveActive = false;
    return;
  }

  float gainFactor = Pilot_gain / 100.0;

  int pwm = MOTOR_PWM_MIN + (int)((rudderErrorAbs / 160.0) * (MOTOR_PWM_MAX - MOTOR_PWM_MIN) * gainFactor);

  pwm = constrain(pwm, MOTOR_PWM_MIN, MOTOR_PWM_MAX);

  if (rudderError > 0)
    setMotor(MOTOR_DIR_STARBOARD, pwm);
  else
    setMotor(MOTOR_DIR_PORT, pwm);
}
