
// ============================================================
// UARTLAN.H - EasyTransfer UART Communication Node
// ============================================================

unsigned long lastRxTime = 0;
const unsigned long RING_TIMEOUT_MS = 200;  // Kickstart na 0.2s stilte
unsigned long lastValidWindTime = 0;

void UpdateUARTlan() {
  bool realDataReceived = ETin.receiveData();
  bool hasData = realDataReceived;

  if (realDataReceived) {
    lastRxTime = millis();  // Reset de timer bij echte geldige ontvangst
  }
  // RING STILVAL-BEVEILIGING:
  // Als er 200ms lang NIETS is ontvangen, geeft deze node de ring 1 slinger
  else if (millis() - lastRxTime > RING_TIMEOUT_MS) {
    hasData = true;
    lastRxTime = millis();
  }

  // ===================================================================
  // ALTIJD DRAAIEN: ACHTERGROND TIMEOUT CHECK VOOR DE WIND
  // ===================================================================
  if (millis() - lastValidWindTime > 1000) {
    Serial.println("error: Wind node not valid");
    Serial.println(Heading);
    isWindValid = false;
  }

  // ===================================================================
  // ALTIJD DRAAIEN: ACHTERGROND TIMEOUT CHECK VOOR HET KOMPAS
  // ===================================================================
  if (millis() - lastValidCompassTime > 1000) {
    Heading_valid = false;

    // ALS DE AP ACTIEF WAS EN HET KOMPAS VALT WEG: DIRECT INGRIJPEN
    if (Auto_active) {
      Auto_active = false;
      stopMotor();                    // Motor uit
      digitalWrite(PIN_CLUTCH, LOW);  // Koppeling los

      Serial.println("CRITICAL: Compass lost! AP forced to Standby.");

      // Alarm piepjes
      beep(200);
      beep(200);
      beep(200);
    }
  }

  // VERWERK HEARTBEATS ALLEEN BIJ ECHTE UART ONTVANGST (Geen cache via kickstart!)
  if (realDataReceived) {
    if (RX_EData.RX_windDirection == 411.00) {
      lastValidWindTime = millis();
      lastValidWindDir = windDirection;
      lastValidWindSpeed = windSpeed;
      isWindValid = true;
    }

    if (RX_EData.RX_heading == 512.00) {
      lastValidCompassTime = millis();
      Heading_valid = true;
    }
  }

  // Zend uit of verwerk doorsturen bij inkomende data of na een timeout kickstart
  if (hasData) {
    // Overige velden overnemen
    TX_EData.TX_speed = RX_EData.RX_speed;
    TX_EData.TX_depth = RX_EData.RX_depth;
    if (isWindValid == true) {

      TX_EData.TX_windDirection = lastValidWindDir;
      TX_EData.TX_windSpeed = lastValidWindSpeed;
    } else {
      TX_EData.TX_windDirection = 0;
      TX_EData.TX_windSpeed = 0;
    }



    // NMEA transparant doorsturen
    strncpy(TX_EData.TX_NMEA, RX_EData.RX_NMEA, sizeof(TX_EData.TX_NMEA) - 1);
    TX_EData.TX_NMEA[sizeof(TX_EData.TX_NMEA) - 1] = '\0';  // Altijd veilig afsluiten met een nul-karakter


    TX_EData.TX_rudder = Rudder_deg;

    lastOpenCPNTargetMillis = millis();

    // ------------------------------------------------------------------
    // TARGET HEADING AFHANDELING
    // ------------------------------------------------------------------
    if (pendingTargetHeading > 0.0f) {
      TX_EData.TX_targetHeading = pendingTargetHeading;
    } else {
      TX_EData.TX_targetHeading = RX_EData.RX_targetHeading;
    }

    // ------------------------------------------------------------------
    // HEADING & COMPASS COMMANDS
    // ------------------------------------------------------------------
    if (pendingCompassCommand > 360) {
      TX_EData.TX_heading = pendingCompassCommand;
      pendingCompassCommand = 0;  // Send command once
    } else {
      TX_EData.TX_heading = RX_EData.RX_heading;
    }

    // ------------------------------------------------------------------
    // WIND COMMANDS
    // ------------------------------------------------------------------
    if (pendingWindCommand > 360) {
      TX_EData.TX_windSpeed = pendingWindCommand;
      pendingWindCommand = 0;  // Send command once
    } else {
      TX_EData.TX_windSpeed = RX_EData.RX_windSpeed;
    }

    // ------------------------------------------------------------------
    // TERMINAL RESPONSE VERWERKING (HMI Node - "Sender Drops Frame")
    // ------------------------------------------------------------------
    if (RX_EData.RX_terminalResponse[0] != '\0') {
      
      if (strncmp(RX_EData.RX_terminalResponse, "CMP:", 4) == 0 || strncmp(RX_EData.RX_terminalResponse, "PNG:", 4) == 0 || strncmp(RX_EData.RX_terminalResponse, "WND:", 4) == 0)

      {
        addCompassTerminalLine(RX_EData.RX_terminalResponse);
      }

      if (strncmp(RX_EData.RX_terminalResponse, "AP:", 3) == 0) {
        TX_EData.TX_terminalResponse[0] = '\0';
      } else {
        strncpy(TX_EData.TX_terminalResponse,
                RX_EData.RX_terminalResponse,
                sizeof(TX_EData.TX_terminalResponse) - 1);
        TX_EData.TX_terminalResponse[sizeof(TX_EData.TX_terminalResponse) - 1] = '\0';
      }

      RX_EData.RX_terminalResponse[0] = '\0';
    } else {
      TX_EData.TX_terminalResponse[0] = '\0';
    }
    // Slimme debug: print alleen wanneer er een EC-zin in RX of TX zit
    String rxStr = String(RX_EData.RX_NMEA);
    String txStr = String(TX_EData.TX_NMEA);

    if (rxStr.indexOf("EC") != -1 || txStr.indexOf("EC") != -1) {
      Serial.print("[EC-FILTER] RX: ");
      Serial.println(rxStr);
      Serial.print("[EC-FILTER] TX: ");
      Serial.println(txStr);
      Serial.println("----------------------------------------");
    }

    ETout.sendData();  // Verstuur data over EasyTransfer
  }
}