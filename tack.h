// ============================================================
// TACK FUNCTIONS
// ============================================================

// TACK wind safety sector.


bool tackHeadingWindSafe(float heading)
{
  static float lastValidWind = 290.0; // Veilige startwaarde

  // 1. Check of de binnenkomende data überhaupt geldig is
  if (!isnan(RX_EData.RX_windDirection)) {
    float rawWind = normalize360(RX_EData.RX_windDirection);
    
    // 2. Als de genormaliseerde wind NIET rond de 51.0 ligt, pas dan pas akkoord geven!
    if (fabs(rawWind - 51.0) > 1.0) {
      lastValidWind = RX_EData.RX_windDirection; // of rawWind
    }
  }

  float wind = normalize360(lastValidWind);
  float forbiddenCenter = normalize360(wind + 180.0);
  float course = normalize360(heading);

  float difference = fabs(signedHeadingError(course, forbiddenCenter));

  // Debug output
  Serial.print("Wind (Gebruikt): "); Serial.print(wind);
  Serial.print(" | Forbidden: "); Serial.print(forbiddenCenter);
  Serial.print(" | Course: "); Serial.print(course);
  Serial.print(" | Diff: "); Serial.println(difference);

  return difference > 10.0;
}

void abortTack()
{
  tack_port = false;
  tack_starboard = false;
  steps = 0;
  tackStepDone = false;
  tack_error = true;
}

void change_heading(float change)
{
  Set_heading = normalize360(Set_heading + change);
}
