

void setupPreferences() {
  prefs.begin("autopilot", false);
}

void loadSettings() {
  Pilot_gain = prefs.getFloat("gain", 100.0);
  Course_window = prefs.getFloat("cw", 4.0);
  Helm_speed = prefs.getInt("hspeed", 100);
  Helm_window = prefs.getFloat("hwin", 3.0);
  Minimum_rudder = prefs.getFloat("minrud", 0.0);
  Maximum_rudder = prefs.getFloat("maxrud", 80.0);
  Low_speed = prefs.getInt("lowspeed", 60);
  Pilot_comp = prefs.getFloat("comp", 0.0);
  Tack_angle = prefs.getFloat("tack", 100.0);
  Tack_speed = prefs.getInt("tackspeed", 10);

  Rudder_port_deg = prefs.getFloat("portdeg", -80.0);
  Rudder_center_deg = prefs.getFloat("centerdeg", 0.0);
  Rudder_star_deg = prefs.getFloat("stardeg", 80.0);
}

void saveSettings() {
  prefs.putFloat("gain", Pilot_gain);
  prefs.putFloat("cw", Course_window);
  prefs.putInt("hspeed", Helm_speed);
  prefs.putFloat("hwin", Helm_window);
  prefs.putFloat("minrud", Minimum_rudder);
  prefs.putFloat("maxrud", Maximum_rudder);
  prefs.putInt("lowspeed", Low_speed);
  prefs.putFloat("comp", Pilot_comp);
  prefs.putFloat("tack", Tack_angle);
  prefs.putInt("tackspeed", Tack_speed);

  prefs.putFloat("portdeg", Rudder_port_deg);
  prefs.putFloat("centerdeg", Rudder_center_deg);
  prefs.putFloat("stardeg", Rudder_star_deg);
}

void restoreFactorySettings() {
  Pilot_gain = 100.0;
  Course_window = 4.0;
  Helm_speed = 100;
  Helm_window = 3.0;
  Minimum_rudder = 0.0;
  Maximum_rudder = 80.0;
  Low_speed = 60;
  Pilot_comp = 0.0;
  Tack_angle = 100.0;
  Tack_speed = 10;

  saveSettings();
}
