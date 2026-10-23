
bool beepActive = false;
bool beepOn = false;

unsigned long beepTimer = 0;

int beepDuration = 0;
int beepCount = 0;

void beep(int durationMs) {

  beepDuration = durationMs;
  beepCount++;

  // Eerste beep starten
  if (!beepActive) {

    digitalWrite(PIN_BUZZER, HIGH);

    beepOn = true;
    beepActive = true;
    beepTimer = millis();
  }
}


void handleBeep() {

  if (!beepActive)
    return;

  // --------------------------------
  // BUZZER STAAT AAN
  // --------------------------------
  if (beepOn) {

    if (millis() - beepTimer >= beepDuration) {

      digitalWrite(PIN_BUZZER, LOW);

      beepOn = false;
      beepTimer = millis();
    }
  }

  // --------------------------------
  // BUZZER STAAT UIT
  // --------------------------------
  else {

    if (millis() - beepTimer >= 80) {

      beepCount--;

      if (beepCount > 0) {

        digitalWrite(PIN_BUZZER, HIGH);

        beepOn = true;
        beepTimer = millis();
      }
      else {

        beepActive = false;
      }
    }
  }
}

