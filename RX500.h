// ============================================================
// RX500.H - RX500 input layer
// ============================================================

void rx500SetEvent(RX500Event event) {
  if (rx500Event == RX500_EVENT_NONE)
    rx500Event = event;
}

void processRX500() {
  unsigned long now = millis();

  // ------------------------------------------------------------
  // Een enkele +10 wordt vertraagd zodat een tweede +10 een dubbele klik kan worden.
  // ------------------------------------------------------------
  if (rx500Plus10Pending && now - rx500Plus10PendingTime > DOUBLE_CLICK_MS) {
    rx500Plus10Pending = false;
    rx500SetEvent(RX500_EVENT_SHORT_PLUS10);
  }

  // ------------------------------------------------------------
  // KNOP LOSLATEN DETECTIE
  // ------------------------------------------------------------
  if (rx500PressCode != 0 && now - rx500LastFrameTime >= RX500_RELEASE_TIME_MS) {
    unsigned long releasedCode = rx500PressCode;
    unsigned long releaseTime = now;

    rx500PressCode = 0;

    // Als er al een LONG actie is geweest, negeren we de loslaat-actie om herrie/dubbele events te voorkomen.
    if (rx500LongActionDone) {
      rx500LongActionDone = false;
      return;
    }

    if (releasedCode == RX500_CODE_MINUS1) {
      rx500SetEvent(RX500_EVENT_SHORT_MINUS1);
      return;
    }

    if (releasedCode == RX500_CODE_PLUS1) {
      rx500SetEvent(RX500_EVENT_SHORT_PLUS1);
      return;
    }

    if (releasedCode == RX500_CODE_MINUS10) {
      rx500SetEvent(RX500_EVENT_SHORT_MINUS10);
      return;
    }

    if (releasedCode == RX500_CODE_PLUS10) {
      if (rx500Plus10Pending && releaseTime - rx500Plus10PendingTime <= DOUBLE_CLICK_MS) {
        rx500Plus10Pending = false;
        rx500SetEvent(RX500_EVENT_DOUBLE_PLUS10);
      } else {
        rx500Plus10Pending = true;
        rx500Plus10PendingTime = releaseTime;
      }
      return;
    }
  }

  if (!rx500.available())
    return;

  unsigned long code = rx500.getReceivedValue();
  unsigned int bits = rx500.getReceivedBitlength();
  unsigned int protocol = rx500.getReceivedProtocol();

  rx500.resetAvailable();

  if (code == 0 || bits != 24 || protocol != 1)
    return;

  now = millis();
  rx500LastFrameTime = now;

  // Start van een nieuwe knopdruk
  if (rx500PressCode == 0) {
    rx500PressCode = code;
    rx500PressStart = now;
    rx500LongActionDone = false;
    beep(15);
    return;
  }

  // Andere knop ingedrukt
  if (code != rx500PressCode) {
    rx500PressCode = code;
    rx500PressStart = now;
    rx500LongActionDone = false;
    return;
  }

  // ------------------------------------------------------------
  // LONG press detectie.
  // ------------------------------------------------------------
  if (!rx500LongActionDone && now - rx500PressStart >= RX500_HOLD_TIME_MS) {
    if (code == RX500_CODE_MINUS1) {
      rx500SetEvent(RX500_EVENT_LONG_MINUS1);
      rx500LongActionDone = true;
    }
    else if (code == RX500_CODE_PLUS1) {
      rx500SetEvent(RX500_EVENT_LONG_PLUS1);
      rx500LongActionDone = true;
    }
    else if (code == RX500_CODE_MINUS10) {
      rx500SetEvent(RX500_EVENT_LONG_MINUS10);
      rx500LongActionDone = true;
    }
    else if (code == RX500_CODE_PLUS10) {
      rx500Plus10Pending = false;
      rx500SetEvent(RX500_EVENT_LONG_PLUS10);
      rx500LongActionDone = true;
    }
  }
}