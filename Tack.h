// ============================================================
// TACK.H - Overstag (Tack) logica en veiligheidscontroles
// ============================================================

void abortTack() {
  tack_port = false;
  tack_starboard = false;
  steps = 0;
  tack_error = true;
  stopMotor();
  digitalWrite(PIN_CLUTCH, LOW);
}

bool tackHeadingWindSafe(float targetHeading) {
  // Als er geen winddata binnenkomt, slaan we deze specifieke windveiligheidscheck over
  if (RX_EData.RX_windSpeed == 0.0 && RX_EData.RX_windDirection == 0.0) {
    return true;
  }

  // Hier kun je eventueel extra logica toevoegen om te controleren 
  // of de windhoek na de overstag manoeuvre binnen veilige marges valt.
  // Momenteel retourneert deze standaard true zodat de tack door kan gaan.
  return true;
}

void change_heading(float amount) {
  if (!Auto_active || !Heading_valid) return;
  
  Set_heading = normalize360(Set_heading + amount);
  calculateHeadingError();
}