#ifndef ICM20948_COMPASS_H
#define ICM20948_COMPASS_H

// ============================================================
// ICM20948 COMPASS V7
// ESP32
// STANDALONE COMPASS TEST
//
// V2 - NMEA OUTPUT REMOVED, MAG averaging 10 samples
//
// - Magnetometer calibration from EEPROM
// - TILT COMPENSATION ACTIVE
// - MAGNETOMETER AVERAGING: 10 samples
// - Circular heading filter
// ============================================================


#include <Wire.h>
#include <ICM20948_WE.h>
#include <EEPROM.h>
#include <math.h>

// ============================================================
// MODULE INTERFACE
// ============================================================

void initCompass();
void updateCompass();
void updateCalibration(xyzFloat mag);
void calculateCalibration();
void saveCalibration();
void loadCalibration();
void clearCalibration();
void processCompassCommand(const String& command);



// ============================================================
// HARDWARE
// ============================================================

#define I2C_FREQUENCY 10000

// ------------------------------------------------------------
// ICM20948
// ------------------------------------------------------------

#define ICM20948_ADDR 0x68

#define SDA_PIN 21
#define SCL_PIN 22

ICM20948_WE myIMU = ICM20948_WE(ICM20948_ADDR);


// ============================================================
// EEPROM
// ============================================================

#define EEPROM_SIZE 128

#define EEPROM_MAGIC 0x20948ABC



// ============================================================
// CALIBRATION STRUCTURE
// ============================================================

struct CompassCalibration {

  uint32_t magic;


  // ----------------------------------------------------------
  // Magnetometer hard iron
  // ----------------------------------------------------------

  float offsetX;
  float offsetY;
  float offsetZ;


  // ----------------------------------------------------------
  // Magnetometer soft iron
  // ----------------------------------------------------------

  float scaleX;
  float scaleY;
  float scaleZ;
};


CompassCalibration cal;


// ============================================================
// MAGNETOMETER CALIBRATION VARIABLES
// ============================================================

bool calibrationMode = false;


float minX = 100000.0;
float maxX = -100000.0;

float minY = 100000.0;
float maxY = -100000.0;

float minZ = 100000.0;
float maxZ = -100000.0;




// ============================================================
// MAGNETOMETER AVERAGING
// ============================================================

const int MAG_AVG_SAMPLES = 10;

float magXBuffer[MAG_AVG_SAMPLES] = { 0 };
float magYBuffer[MAG_AVG_SAMPLES] = { 0 };

int magAvgIndex = 0;
int magAvgCount = 0;


// ============================================================
// HEADING FILTER
// ============================================================

float filteredHeading = 0;

bool headingInitialized = false;
float alpha = 0.01;

// DEBUG OUTPUT
// false = NMEA0183 only, true = full debug output
bool debugEnabled = false;



// ============================================================
// SETUP
// ============================================================

void initCompass() {

  delay(1000);

  Serial.println();
  Serial.println("==============================================");
  Serial.println(" ESP32 ICM-20948 COMPASS V7");
  Serial.println("==============================================");

  // Start I2C once, then allow the sensor to settle after power-up.
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(I2C_FREQUENCY);
  delay(100);

  EEPROM.begin(EEPROM_SIZE);
  loadCalibration();

  // The ICM-20948 and AK09916 need separate initialisation.
  // Retry both parts after repower instead of declaring the sensor defective.
  bool imuReady = false;
  bool magReady = false;

  for (int attempt = 1; attempt <= 5; attempt++) {

    Serial.print("Sensor initialisation attempt ");
    Serial.println(attempt);

    if (!imuReady) {
      imuReady = myIMU.init();
      if (imuReady)
        Serial.println("ICM20948 connected");
      else
        Serial.println("ICM20948 init failed");
    }

    delay(100);

    if (imuReady && !magReady) {
      magReady = myIMU.initMagnetometer();
      if (magReady)
        Serial.println("AK09916 connected");
      else
        Serial.println("AK09916 init failed");
    }

    if (imuReady && magReady)
      break;

    delay(500);
  }

  if (!imuReady || !magReady) {
    Serial.println("ERROR: Compass initialisation failed");
    while (1)
      delay(1000);
  }

  myIMU.setMagOpMode(AK09916_CONT_MODE_50HZ);
  delay(500);

  magAvgIndex = 0;
  magAvgCount = 0;
  headingInitialized = false;
  filteredHeading = 0;

  Serial.println();
  Serial.println("==============================================");
  Serial.println("COMMANDS");
  Serial.println("==============================================");
  Serial.println("C = Start magnetometer calibration");
  Serial.println("S = Stop magnetometer calibration");
  Serial.println("W = Write calibration to EEPROM");
  Serial.println("L = Load calibration from EEPROM");
  Serial.println("CLEARCAL = Clear calibration");
  Serial.println("DEBUGON = Debugdata");
  Serial.println("DEBUGOFF = NMEA183 output");
  Serial.println("==============================================");
  Serial.println();
  Serial.println("MAG averaging = 10 samples");
  Serial.println("Tilt compensation = ON");
  Serial.println();
  Serial.println("==============================================");

  delay(500);
}

// ============================================================
// MAIN LOOP
// ============================================================

void updateCompass() {



  // ----------------------------------------------------------
  // SENSOR VARIABLES
  // ----------------------------------------------------------

  xyzFloat mag;
  xyzFloat accel;


  // ----------------------------------------------------------
  // READ SENSOR
  // ----------------------------------------------------------

  myIMU.readSensor();

  myIMU.getMagValues(&mag);

  myIMU.getGValues(&accel);



  // ----------------------------------------------------------
  // MAGNETOMETER CALIBRATION MODE
  // ----------------------------------------------------------

  if (calibrationMode) {

    updateCalibration(mag);


    Serial.print("CAL X:");
    Serial.print(mag.x, 1);

    Serial.print(" Y:");
    Serial.print(mag.y, 1);

    Serial.print(" Z:");
    Serial.println(mag.z, 1);


    delay(50);

    return;
  }


  // ==========================================================
  // APPLY MAGNETOMETER CALIBRATION
  // THEN CONVERT AK09916 AXES TO ICM20948 AXES
  // ==========================================================

  float mx = (mag.x - cal.offsetX) * cal.scaleX;

  float my = (mag.y - cal.offsetY) * cal.scaleY;

  float mz = (mag.z - cal.offsetZ) * cal.scaleZ;


  // AK09916 -> ICM20948 coordinate system, Read the TDK manual!!!! Axes are reversed!
  my = -my;
  mz = -mz;


  // ==========================================================
  // ACCELEROMETER NORMALIZATION
  // ==========================================================

  float ax = accel.x;
  float ay = accel.y;
  float az = accel.z;

  float accelLength = sqrt(ax * ax + ay * ay + az * az);

  if (accelLength > 0.001) {

    ax /= accelLength;
    ay /= accelLength;
    az /= accelLength;
  }


  // ==========================================================
  // ROLL
  // ==========================================================

  float roll = atan2(ay, az);


  // ==========================================================
  // PITCH
  // ==========================================================

  float pitch = atan2(-ax, sqrt(ay * ay + az * az));


  // ==========================================================
  // TILT COMPENSATION
  // ==========================================================

  float sinRoll = sin(roll);
  float cosRoll = cos(roll);

  float sinPitch = sin(pitch);
  float cosPitch = cos(pitch);


  // Eerst pitch corrigeren
  float mxPitch = mx * cosPitch + mz * sinPitch;

  float mzPitch = -mx * sinPitch + mz * cosPitch;


  // Daarna roll corrigeren
  float mxHorizontal = mxPitch;

  float myHorizontal = my * cosRoll - mzPitch * sinRoll;


  // ==========================================================
  // MAGNETOMETER AVERAGING
  // ==========================================================

  magXBuffer[magAvgIndex] = mxHorizontal;
  magYBuffer[magAvgIndex] = myHorizontal;

  magAvgIndex++;

  if (magAvgIndex >= MAG_AVG_SAMPLES) {
    magAvgIndex = 0;
  }

  if (magAvgCount < MAG_AVG_SAMPLES) {
    magAvgCount++;
  }


  float avgMagX = 0.0;
  float avgMagY = 0.0;

  for (int i = 0; i < magAvgCount; i++) {
    avgMagX += magXBuffer[i];
    avgMagY += magYBuffer[i];
  }

  avgMagX /= magAvgCount;
  avgMagY /= magAvgCount;


  // ==========================================================
  // RAW HEADING
  // ==========================================================

  float heading = atan2(avgMagY, avgMagX);

  heading = heading * 180.0 / PI;


  // ==========================================================
  // 0 ... 360
  // ==========================================================

  if (heading < 0) {

    heading += 360.0;
  }

  if (heading >= 360.0) {

    heading -= 360.0;
  }
  // ==========================================================
  // CIRCULAR HEADING FILTER
  // ==========================================================


  if (!headingInitialized) {

    filteredHeading = heading;

    headingInitialized = true;
  }


  else {

    float difference = heading - filteredHeading;


    // --------------------------------------------------------
    // Handle 360 -> 0 transition
    // --------------------------------------------------------

    if (difference > 180.0) {

      difference -= 360.0;
    }


    if (difference < -180.0) {

      difference += 360.0;
    }

    // --------------------------------------------------------
    // Adaptive alpha
    // --------------------------------------------------------



    float absDifference = abs(difference);

    if (absDifference > 10.0) {

      alpha = 0.20;

    } else if (absDifference > 5.0) {

      alpha = 0.12;

    } else if (absDifference > 2.0) {

      alpha = 0.07;

    } else if (absDifference > 1.0) {

      alpha = 0.02;

      /*} else {

      alpha = 0.01;
    */
    }


    filteredHeading += alpha * difference;

    //filteredHeading = heading;  alpha off



    // --------------------------------------------------------
    // Normalize
    // --------------------------------------------------------

    if (filteredHeading < 0) {

      filteredHeading += 360.0;
    }


    if (filteredHeading >= 360.0) {

      filteredHeading -= 360.0;
    }
  }


  headingReceived(filteredHeading);

  // ==========================================================
  // DEBUG OUTPUT
  // ==========================================================

  if (debugEnabled) {

    /*Serial.print(" TIME: ");
    Serial.print(millis() / 1000.0);

    Serial.print(" MAG_X:");
    Serial.printf("%05.1f", mx);
    Serial.print(" Y:");
    Serial.printf("%05.1f", my);
    Serial.print(" Z:");
    Serial.printf("%05.1f", mz);

    Serial.print(" HMAG_X:");
    Serial.printf("%05.1f", mxHorizontal);
    Serial.print(" Y:");
    Serial.printf("%05.1f", myHorizontal);

    Serial.print(" AVG_X:");
    Serial.printf("%05.1f", avgMagX);
    Serial.print(" Y:");
    Serial.printf("%05.1f", avgMagY);

    Serial.print(" Roll:");
    Serial.printf("%05.1f", roll * 180.0 / PI);
    Serial.print(" Pitch:");
    Serial.printf("%05.1f", pitch * 180.0 / PI);

    Serial.print(" alpha:");
    Serial.print(alpha, 2);*/

    Serial.print(" RAW:");
    Serial.print(heading);

    Serial.print("  HDG:");
    Serial.println(filteredHeading);
  }


  delay(100);
}

void updateCalibration(
  xyzFloat mag) {

  if (mag.x < minX)
    minX = mag.x;

  if (mag.x > maxX)
    maxX = mag.x;


  if (mag.y < minY)
    minY = mag.y;

  if (mag.y > maxY)
    maxY = mag.y;


  if (mag.z < minZ)
    minZ = mag.z;

  if (mag.z > maxZ)
    maxZ = mag.z;
}


// ============================================================
// CALCULATE MAGNETOMETER CALIBRATION
// ============================================================

void calculateCalibration() {


  // ----------------------------------------------------------
  // HARD IRON
  // ----------------------------------------------------------

  cal.offsetX = (maxX + minX) / 2.0;


  cal.offsetY = (maxY + minY) / 2.0;


  cal.offsetZ = (maxZ + minZ) / 2.0;


  // ----------------------------------------------------------
  // AXIS RADIUS
  // ----------------------------------------------------------

  float radiusX = (maxX - minX) / 2.0;


  float radiusY = (maxY - minY) / 2.0;


  float radiusZ = (maxZ - minZ) / 2.0;


  // ----------------------------------------------------------
  // AVERAGE RADIUS
  // ----------------------------------------------------------

  float averageRadius = (radiusX + radiusY + radiusZ) / 3.0;


  // ----------------------------------------------------------
  // SCALE X
  // ----------------------------------------------------------

  if (radiusX > 0.001) {

    cal.scaleX = averageRadius / radiusX;
  }

  else {

    cal.scaleX = 1.0;
  }


  // ----------------------------------------------------------
  // SCALE Y
  // ----------------------------------------------------------

  if (radiusY > 0.001) {

    cal.scaleY = averageRadius / radiusY;
  }

  else {

    cal.scaleY = 1.0;
  }


  // ----------------------------------------------------------
  // SCALE Z
  // ----------------------------------------------------------

  if (radiusZ > 0.001) {

    cal.scaleZ = averageRadius / radiusZ;
  }

  else {

    cal.scaleZ = 1.0;
  }


  // ----------------------------------------------------------
  // RESULT
  // ----------------------------------------------------------

  Serial.println();
  Serial.println("==============================================");
  Serial.println("MAGNETOMETER CALIBRATION RESULT");
  Serial.println("==============================================");


  Serial.print("Offset X = ");
  Serial.println(
    cal.offsetX,
    5);


  Serial.print("Offset Y = ");
  Serial.println(
    cal.offsetY,
    5);


  Serial.print("Offset Z = ");
  Serial.println(
    cal.offsetZ,
    5);


  Serial.println();


  Serial.print("Scale X = ");
  Serial.println(
    cal.scaleX,
    5);


  Serial.print("Scale Y = ");
  Serial.println(
    cal.scaleY,
    5);


  Serial.print("Scale Z = ");
  Serial.println(
    cal.scaleZ,
    5);


  Serial.println();


  delay(5000);


  Serial.println(
    "Calibration calculated.");


  Serial.println(
    "Press W to SAVE to EEPROM.");


  delay(5000);
}


// ============================================================
// SAVE CALIBRATION
// ============================================================

void saveCalibration() {


  cal.magic = EEPROM_MAGIC;

  EEPROM.put(
    0,
    cal);


  EEPROM.commit();


  Serial.println();
  Serial.println("==============================================");
  Serial.println("CALIBRATION SAVED");
  Serial.println("==============================================");


  Serial.println(
    "Magnetometer calibration written to EEPROM.");

  delay(5000);
}


// ============================================================
// LOAD CALIBRATION
// ============================================================

void loadCalibration() {


  EEPROM.get(
    0,
    cal);


  if (
    cal.magic == EEPROM_MAGIC) {


    Serial.println(
      "Calibration loaded from EEPROM");
  }


  else {


    Serial.println(
      "No valid calibration found");


    cal.magic = EEPROM_MAGIC;


    cal.offsetX = 0;
    cal.offsetY = 0;
    cal.offsetZ = 0;


    cal.scaleX = 1.0;
    cal.scaleY = 1.0;
    cal.scaleZ = 1.0;
  }
}


// ============================================================
// CLEAR CALIBRATION
// ============================================================

void clearCalibration() {


  cal.magic = 0;


  cal.offsetX = 0;
  cal.offsetY = 0;
  cal.offsetZ = 0;


  cal.scaleX = 1.0;
  cal.scaleY = 1.0;
  cal.scaleZ = 1.0;

  EEPROM.put(
    0,
    cal);


  EEPROM.commit();


  Serial.println();
  Serial.println("==============================================");
  Serial.println("CALIBRATION CLEARED");
  Serial.println("==============================================");


  Serial.println(
    "EEPROM calibration invalidated.");


  delay(5000);
}


// ============================================================
// SERIAL COMMANDS
// ============================================================
void processCompassCommand(const String& input) {

  String command = input;
  command.trim();
  command.toUpperCase();


  // ==========================================================
  // DEBUG ON
  // ==========================================================

  if (command == "DEBUGON") {

    debugEnabled = true;
    Serial.println("DEBUG ON");
  }


  // ==========================================================
  // DEBUG OFF
  // ==========================================================

  if (command == "DEBUGOFF") {

    debugEnabled = false;
    Serial.println("DEBUG OFF");
  }


  // ==========================================================
  // START MAGNETOMETER CALIBRATION
  // ==========================================================

  if (
    command == "C") {


    calibrationMode = true;


    minX = 100000.0;
    maxX = -100000.0;

    minY = 100000.0;
    maxY = -100000.0;

    minZ = 100000.0;
    maxZ = -100000.0;


    Serial.println();
    Serial.println("==============================================");
    Serial.println("CALIBRATION STARTED");
    Serial.println("==============================================");


    delay(5000);


    Serial.println(
      "Rotate the sensor slowly in ALL directions.");

    Serial.println(
      "Make several complete rotations.");

    Serial.println(
      "Tilt and rotate over all 3 axes.");

    Serial.println(
      "Collecting calibration data...");

    Serial.println(
      "Press S when finished.");


    delay(5000);
  }


  // ==========================================================
  // STOP MAGNETOMETER CALIBRATION
  // ==========================================================

  if (
    command == "S") {


    if (calibrationMode) {


      calibrationMode = false;


      Serial.println();
      Serial.println("==============================================");
      Serial.println("STOPPING CALIBRATION");
      Serial.println("==============================================");


      delay(5000);


      calculateCalibration();


      Serial.println();
      Serial.println(
        "Calibration finished.");

      Serial.println(
        "Press W to save calibration to EEPROM.");


      delay(5000);
    }
  }


  // ==========================================================
  // WRITE EEPROM
  // ==========================================================

  if (
    command == "W") {

    saveCalibration();
  }


  // ==========================================================
  // LOAD EEPROM
  // ==========================================================

  if (
    command == "L") {


    loadCalibration();


    Serial.println();
    Serial.println(
      "Calibration loaded.");


    Serial.println(
      "Current calibration values:");


    Serial.print(
      "Offset X = ");

    Serial.println(
      cal.offsetX,
      5);


    Serial.print(
      "Offset Y = ");

    Serial.println(
      cal.offsetY,
      5);


    Serial.print(
      "Offset Z = ");

    Serial.println(
      cal.offsetZ,
      5);


    Serial.print(
      "Scale X = ");

    Serial.println(
      cal.scaleX,
      5);


    Serial.print(
      "Scale Y = ");

    Serial.println(
      cal.scaleY,
      5);


    Serial.print(
      "Scale Z = ");

    Serial.println(
      cal.scaleZ,
      5);

    delay(5000);
  }


  // ==========================================================
  // CLEAR EEPROM
  // ==========================================================

  if (
    command == "CLEARCAL") {

    clearCalibration();
  }
}



bool compassCalibrationActive() {
  return calibrationMode;
}

#endif  // ICM20948_COMPASS_H
