

void processRX500() {
  if (rx500.available()) {
    unsigned long code = rx500.getReceivedValue();
    unsigned int bits = rx500.getReceivedBitlength();
    unsigned int protocol = rx500.getReceivedProtocol();
    unsigned long now = millis();

    rx500.resetAvailable();

    if (code == 0)
      return;

    if (bits != 24 || protocol != 1) {
      return;
    }

    rx500LastFrameTime = now;

    if (rx500PressCode == 0) {
      rx500PressCode = code;
      rx500PressStart = now;
      rx500LongActionDone = false;
      return;
    }

    if (code != rx500PressCode) {
      rx500PressCode = code;
      rx500PressStart = now;
      rx500LongActionDone = false;
      return;
    }

    if (code == RX500_CODE_MINUS1 && !rx500LongActionDone && now - rx500PressStart >= RX500_HOLD_TIME_MS) {
      autopilotOff();
      rx500LongActionDone = true;
    }

    if (code == RX500_CODE_PLUS1 && !rx500LongActionDone && now - rx500PressStart >= RX500_HOLD_TIME_MS) {
      autopilotOn();
      rx500LongActionDone = true;
    }

    if (code == RX500_CODE_MINUS10 && !rx500LongActionDone && now - rx500PressStart >= RX500_HOLD_TIME_MS) {
      if (hmiMode == HMI_NORMAL) {
        enterSettings();
        rx500LongActionDone = true;
      }
    }

    if (code == RX500_CODE_PLUS10 && !rx500LongActionDone && now - rx500PressStart >= RX500_HOLD_TIME_MS) {
      if (hmiMode == HMI_SETTINGS_SELECT || hmiMode == HMI_SETTINGS_EDIT) {
        rx500OK = true;
        rx500LongActionDone = true;
      }
    }
  }

  if (rx500PressCode != 0 && millis() - rx500LastFrameTime >= RX500_RELEASE_TIME_MS) {
    unsigned long code = rx500PressCode;
    rx500PressCode = 0;

    if (rx500LongActionDone)
      return;

    if (code == RX500_CODE_MINUS1) {
      if (hmiMode == HMI_NORMAL) {
        normalSelection = 1;
        executeNormalFunction();
      } else {
        leftPressed();
      }
    } else if (code == RX500_CODE_PLUS1) {
      if (hmiMode == HMI_NORMAL) {
        normalSelection = 3;
        executeNormalFunction();
      } else {
        rightPressed();
      }
    } else if (code == RX500_CODE_MINUS10) {
      if (hmiMode == HMI_NORMAL) {
        normalSelection = 0;
        executeNormalFunction();
      }
    } else if (code == RX500_CODE_PLUS10) {

      if (rudderCalibrationWaiting) {
        unsigned long now = millis();

        // Twee korte +10-drukken tijdens kalibratie = EXIT.
        if (!rx500LongActionDone && now - lastPlus10ShortRelease <= DOUBLE_CLICK_MS) {

          rudderCalibrationExitRequested = true;
          rx500OK = true;
          lastPlus10ShortRelease = 0;
          return;
        }

        // Lange +10 tijdens kalibratie = bevestigen.
        if (rx500LongActionDone) {
          return;
        }

        rx500OK = true;
        lastPlus10ShortRelease = now;
        return;
      }

      if (hmiMode == HMI_NORMAL) {
        normalSelection = 4;
        executeNormalFunction();
      } else {
        unsigned long now = millis();

        if (now - lastPlus10ShortRelease <= DOUBLE_CLICK_MS) {
          hmiMode = HMI_NORMAL;
          beep(80);
          lastPlus10ShortRelease = 0;
        } else {
          okPressed();
          lastPlus10ShortRelease = now;
        }
      }
    }
  }
}

void leftPressed() {
  if (hmiMode == HMI_NORMAL) {
    normalSelection--;
    if (normalSelection < 0)
      normalSelection = 4;
  } else if (hmiMode == HMI_SETTINGS_SELECT) {
    settingsSelection--;
    if (settingsSelection < 0)
      settingsSelection = 11;
  } else {
    switch (settingsSelection) {
      case 0: Pilot_gain -= 10; break;
      case 1: Course_window -= 1; break;
      case 2: Helm_speed -= 5; break;
      case 3: Helm_window -= 1; break;
      case 4: Minimum_rudder -= 1; break;
      case 5: Maximum_rudder -= 1; break;
      case 6: Low_speed -= 5; break;
      case 7: Pilot_comp -= 0.1; break;
      case 8: Tack_angle -= 10; break;
      case 9: Tack_speed -= 1; break;
    }
  }

  Pilot_gain = constrain(Pilot_gain, 0, 500);
  Course_window = constrain(Course_window, 0.5, 30);
  Helm_speed = constrain(Helm_speed, 0, 255);
  Helm_window = constrain(Helm_window, 0.5, 20);
  Minimum_rudder = constrain(Minimum_rudder, 0, 80);
  Maximum_rudder = constrain(Maximum_rudder, 1, 90);
  Low_speed = constrain(Low_speed, 0, 255);
  Pilot_comp = constrain(Pilot_comp, -10, 10);
  Tack_angle = constrain(Tack_angle, 10, 180);
  Tack_speed = constrain(Tack_speed, 0, 255);
}

void rightPressed() {
  if (hmiMode == HMI_NORMAL) {
    normalSelection++;
    if (normalSelection > 4)
      normalSelection = 0;
  } else if (hmiMode == HMI_SETTINGS_SELECT) {
    settingsSelection++;
    if (settingsSelection > 11)
      settingsSelection = 0;
  } else {
    switch (settingsSelection) {
      case 0: Pilot_gain += 10; break;
      case 1: Course_window += 1; break;
      case 2: Helm_speed += 5; break;
      case 3: Helm_window += 1; break;
      case 4: Minimum_rudder += 1; break;
      case 5: Maximum_rudder += 1; break;
      case 6: Low_speed += 5; break;
      case 7: Pilot_comp += 0.1; break;
      case 8: Tack_angle += 10; break;
      case 9: Tack_speed += 1; break;
    }

    Pilot_gain = constrain(Pilot_gain, 0, 500);
    Course_window = constrain(Course_window, 0.5, 30);
    Helm_speed = constrain(Helm_speed, 0, 255);
    Helm_window = constrain(Helm_window, 0.5, 20);
    Minimum_rudder = constrain(Minimum_rudder, 0, 80);
    Maximum_rudder = constrain(Maximum_rudder, 1, 90);
    Low_speed = constrain(Low_speed, 0, 255);
    Pilot_comp = constrain(Pilot_comp, -10, 10);
    Tack_angle = constrain(Tack_angle, 10, 180);
    Tack_speed = constrain(Tack_speed, 0, 255);
  }
}

void okPressed() {
  if (hmiMode == HMI_SETTINGS_SELECT) {
    hmiMode = HMI_SETTINGS_EDIT;
    beep(60);
  } else if (hmiMode == HMI_SETTINGS_EDIT) {
    editSetting();
  }
}

void executeNormalFunction() {
  if (!Heading_valid) {
    beep(200);
    return;
  }

  switch (normalSelection) {
    case 0:
      if (!isnan(Set_heading))
        Set_heading = normalize360(Set_heading - 10);
      break;

    case 1:
      if (!isnan(Set_heading))
        Set_heading = normalize360(Set_heading - 1);
      break;

    case 2:
      autopilotOff();
      return;

    case 3:
      if (!isnan(Set_heading))
        Set_heading = normalize360(Set_heading + 1);
      break;

    case 4:
      if (!isnan(Set_heading))
        Set_heading = normalize360(Set_heading + 10);
      break;
  }

  // A short heading command must never switch the pilot on.
  // The pilot is switched on exclusively by a long +1 command.
  calculateHeadingError();
  beep(35);
}

void enterSettings() {
  if (Auto_active)
    autopilotOff();

  hmiMode = HMI_SETTINGS_SELECT;
  beep(60);
}

void editSetting() {
  if (settingsSelection == 10) {
    calibrateRudder();
    hmiMode = HMI_SETTINGS_SELECT;
    return;
  }

  if (settingsSelection == 11) {
    restoreFactorySettings();
    hmiMode = HMI_SETTINGS_SELECT;
    beep(120);
    return;
  }

  saveSettings();
  hmiMode = HMI_SETTINGS_SELECT;
  beep(60);
}
