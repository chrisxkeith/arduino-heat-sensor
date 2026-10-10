#define LOCAL_BUILD

const String unitID = "2"; //"* * * set this before compile * * *"; // 'n' for giga, 'Nano-Iot-n" for Nano 33 Io

#ifdef LOCAL_BUILD
String elapsedTime;
String gridAsString;
String maxTemperature;
boolean showGrid;
#define CLOUD_TIME time_t
class CloudWrapper {
  public:
    void setup() {}
    void loop() {}
    bool isConnected() { return false; }
    CLOUD_TIME getLocalTime() { return 0; }
};
#else
#define CLOUD_TIME CloudTime
#include "thingProperties.h"
class CloudWrapper {
  public:
    void setup() {
      initProperties();
      ArduinoCloud.begin(ArduinoIoTPreferredConnection);
      setDebugMessageLevel(2);
      ArduinoCloud.printDebugInfo();
    }
    void loop() {
      ArduinoCloud.update();
    }
    bool isConnected() {
      return ArduinoCloud.connected();
    }
    CLOUD_TIME getLocalTime() {
      return ArduinoCloud.getLocalTime();
    }
};
void onElapsedTimeChange() {}
void onGridAsStringChange() {}
void onMaxTemperatureChange() {}
#endif
CloudWrapper cloudWrapper;

#include <time.h>
class TimeSupport {
  private:
    const CLOUD_TIME    NO_TIME = 0;
    unsigned long       lastSyncMillis = 0;
    CLOUD_TIME          timeFromCloud = NO_TIME;
    void                doHandleTime();
  public:
                TimeSupport();
    CLOUD_TIME  getCurrentTime();
    String      timeStr(CLOUD_TIME t);
    String      now();
    void        handleTime();
    String      dump();
};

TimeSupport::TimeSupport() {
  doHandleTime();
}
CLOUD_TIME TimeSupport::getCurrentTime() {
  if (timeFromCloud == NO_TIME) {
    return 0;
  }
  return ((unsigned int)timeFromCloud) + (millis() - lastSyncMillis) / 1000;
}
String TimeSupport::timeStr(CLOUD_TIME t) {
  if ((timeFromCloud == NO_TIME) || (t == NO_TIME)) {
    return "unknown time, no time from cloud";
  }
  time_t rawTime = (time_t)t;
  char buffer[128];
  char* timeStr = ctime_r(&rawTime, buffer);
  // size_t written = strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", time_info); // ToDo?
  if (timeStr == NULL) {
    return "unknown time, ctime_r failed";
  }
  return String(buffer);
}

String TimeSupport::now() {
  return timeStr(timeFromCloud);
}

void TimeSupport::doHandleTime() {
  timeFromCloud = cloudWrapper.getLocalTime();
  lastSyncMillis = millis();
}

unsigned long lastAttemptMillis = 0;
void TimeSupport::handleTime() {
  if (timeFromCloud == NO_TIME) {
    if (millis() - lastAttemptMillis > 1000 * 30) { // every 30 seconds until we can connect
      if (!cloudWrapper.isConnected()) {
        lastAttemptMillis = millis();
        Serial.println("TimeSupport: Not connected to cloud, cannot get time.");
        return;
      }
      doHandleTime();
    }
  } else {
    const unsigned long ONE_DAY_IN_MILLISECONDS = 24 * 60 * 60 * 1000;
    if (millis() - lastSyncMillis > ONE_DAY_IN_MILLISECONDS) {    // If it's been a day since last sync...
                                                            // Request time synchronization from the cloud.
      doHandleTime();
    }
  }
}
String TimeSupport::dump() {
  String s("lastSyncMillis: ");
  s.concat(lastSyncMillis);
  s.concat(", timeFromCloud: ");
  s.concat((unsigned long)timeFromCloud);
  s.concat(", millis(): ");
  s.concat(millis());
  s.concat(", getCurrentTime(): ");
  s.concat((unsigned long)getCurrentTime());
  s.concat(", cloudWrapper.getLocalTime(): ");
  s.concat((unsigned long)cloudWrapper.getLocalTime());
  return s;
}
TimeSupport*    timeSupport = nullptr;

#include <Wire.h>
#include <vector>
#include <set>

class Utils {
  public:
    const static bool DO_SERIAL = true;
    static String msToString(unsigned long ms);
    static void publishWithSep(String s, String sep);
    static void publish(String s);
    static String toString(bool b);
    static void waitForSomeSeconds(String msg) {
      const int WAIT_SECONDS = 3;
      Serial.print("Waiting for " + String(WAIT_SECONDS) + " seconds: " + msg);
      for (int i = 0; i < WAIT_SECONDS; i++) {
        Serial.print(".");
        delay(1000);
      }
      Serial.println("");
    }

    // Modified from https://playground.arduino.cc/Main/I2cScanner/
    static void scanI2C() {
      Wire.begin();
    
      Serial.println("I2C: Scanning for devices...");    
      std::vector<byte> foundDevices;
      String ss("I2C: Error numbers returned: ");
      for( byte address = 1; address < 127; address++ ) {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
    
        if (error == 0) {
          foundDevices.push_back(address);
        } else {
          String err(address);
          err.concat(":");
          err.concat(error);
          err.concat(", ");
          ss.concat(err);
        }    
      }
      Serial.print("I2C: Devices found at: ");
      for (byte b : foundDevices) {
        Serial.print(b);
        Serial.print(" ");
      }
      Serial.println("");
      Serial.println(ss);
    }
};

class Timer {
  private:
    String        msg;
    unsigned long start;
  public:
    Timer(String msg) {
      this->msg = msg;
      start = millis();
    }
    ~Timer() {
      unsigned long ms = millis() - start;
      unsigned long seconds = ms / 1000;
      unsigned long remainingMS = ms % 1000;
      String msg("time for ");
      msg.concat(this->msg);
      msg.concat(": ");
      char buf[100];
      sprintf(buf, "%02u.%03u", seconds, remainingMS);
      msg.concat(String(buf));
      msg.concat(" seconds");
      Serial.println(msg);
    }
};

class DisplayParams {
  private:
    const int MIN_TEMP = 80; // for production
    const int MAX_TEMP = 100;
    const int THRESHOLD = 90;
    const int TEST_MIN_TEMP = 60; // for testing with skin temperature
    const int TEST_MAX_TEMP = 100;
    const int TEST_THRESHOLD = 70;

  public:
    const bool PRODUCTION = unitID.equals("1") || unitID.equals("Nano-Iot-5");
    int minTemp;
    int maxTemp;
    int threshold;
    DisplayParams() {
      if (PRODUCTION) {
        setParams();
      } else {
        setTestParams();
      }
    }
    void setTestParams() {
      this->minTemp = TEST_MIN_TEMP;
      this->maxTemp = TEST_MAX_TEMP;
      this->threshold = TEST_THRESHOLD;
    }
    void setParams() {
      this->minTemp = MIN_TEMP;
      this->maxTemp = MAX_TEMP;
      this->threshold = THRESHOLD;
    }
};
DisplayParams displayParams;

#include <float.h>

// #define USE_128_X_128

#ifdef USE_128_X_128
#include <U8g2lib.h>
U8G2_SSD1327_EA_W128128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
class OLEDWrapper {
  private:
      const int START_BASELINE = 50;
      const int VERTICAL_SHIFT = 2;
      const int HORIZONTAL_SHIFT = 4;
      int   baseLine = START_BASELINE;
      int   leftMargin = HORIZONTAL_SHIFT;
      int getHeight() {
        return 96; // ??? why does u8g2.getHeight() return 128 ???
      }
      int getWidth() {
        return u8g2.getWidth();
      }

  public:
    // For showing hands/fingers. Stove burner temps will be different.
    static const long   MIN_TEMP_IN_F = 80;   // degrees F that will display as black superpixel.
    static const long   MAX_TEMP_IN_F = 90;   // degrees F that will display as white superpixel.

    void u8g2_prepare(void) {
      u8g2.setFont(u8g2_font_fur49_tn);
      u8g2.setFontRefHeightExtendedText();
      u8g2.setDrawColor(1);
      u8g2.setFontDirection(0);
    }
    void startup() {
      pinMode(10, OUTPUT);
      pinMode(9, OUTPUT);
      digitalWrite(10, 0);
      digitalWrite(9, 0);
      u8g2.begin();
      u8g2.setBusClock(400000);
    }
    void showMessages(String s[], int nStrings) {
      u8g2_prepare();
      u8g2.clearBuffer();
      u8g2.drawFrame(0, 0, getWidth(), getHeight());
      u8g2.setFont(u8g2_font_fur11_tf);
      for (int i = 0; i < nStrings; i++) {
        display(s[i], 0, 16 + (i * 16));
      }
      u8g2.sendBuffer();
    }
    void showTemp(int val) {
      u8g2_prepare();
      u8g2.clearBuffer();
      u8g2.drawUTF8(leftMargin, this->baseLine, String(val).c_str());
      u8g2.setFont(u8g2_font_fur11_tf);
      u8g2.drawUTF8(leftMargin + 4, this->baseLine + 20, "Fahrenheit");
      u8g2.sendBuffer();
    }
    void clear() {
      u8g2_prepare();
      u8g2.clearBuffer();
      u8g2.sendBuffer();
    }
    void setupBlurFilter() {}
    void startDisplay(const uint8_t *font) {
      u8g2_prepare();
      u8g2.clearBuffer();
      u8g2.setFont(font);
    }
    void endDisplay() {
      u8g2.sendBuffer();
    }
    void shiftDisplay() {
      baseLine += VERTICAL_SHIFT;
      leftMargin += HORIZONTAL_SHIFT;
      if (baseLine > 63) {
        baseLine = START_BASELINE;
        leftMargin = HORIZONTAL_SHIFT;
      }
    }
    void display(String s, int x, int y) {
      u8g2.drawUTF8(x, y, s.c_str());
    }
    void display(String s) {
      display(s, 0, 0);
    }
    void dump() {}
    void  displayDynamicGrid(float vals[]) {}
};
#else
#include <limits.h>

const int COLOR_WHITE = 0x65535;
const int COLOR_BLACK = 0x0;
#include "Arduino_GigaDisplay_GFX.h"
#include "Fonts/FreeSans12pt7b.h"
#include "Fonts/FreeSans18pt7b.h"
#include "Fonts/FreeSans24pt7b.h"
#include "Fonts/Org_01.h"
#include "Fonts/Picopixel.h"
#include "Fonts/Tiny3x3a2pt7b.h"
#include "Fonts/TomThumb.h"

GigaDisplay_GFX display_;

class OLEDWrapper {
  private:
    uint16_t currentColor = COLOR_WHITE;
    const int DEFAULT_FONT_SIZE = 3;
    const int backlightPin = 74; // D74 controls the backlight driver
    const GFXfont* font24 = &FreeSans24pt7b;
    const GFXfont* font18 = &FreeSans18pt7b;
    void display(String s, int textSize, uint16_t x, uint16_t y) {
      display(s, &FreeSans24pt7b, textSize, x, y);
    }
    void display(String s, const GFXfont* font, int textSize, uint16_t x, uint16_t y) {
      display_.setCursor(x, y);
      display_.setFont(font);
      display_.setTextSize(textSize);
      display_.print(s);
    }
    void doDisplaySmoothedDynamicGrid(uint16_t colors[], int size, int width, int height) {
      const int   FACTOR = height / 8; // 8x8 sensor grid
      const int   MASK_SIZE = FACTOR / 2;
      const int   DIV = MASK_SIZE * MASK_SIZE;
      const int   HALF_MASK_SIZE = MASK_SIZE / 2;
      const int   SUPER_PIXEL_SIZE = 32;
      const int   ROTATE_FACTOR = height - HALF_MASK_SIZE;
      int         sumR = 0;
      int         sumG = 0;
      int         sumB = 0;

      display_.startWrite();
      for (int row = HALF_MASK_SIZE; row < height - HALF_MASK_SIZE; row += SUPER_PIXEL_SIZE) {
        for (int col = HALF_MASK_SIZE; col < width - HALF_MASK_SIZE; col += SUPER_PIXEL_SIZE) {
          sumR = sumG = sumB = 0;
          for (int m = -HALF_MASK_SIZE; m <= HALF_MASK_SIZE; m++) {
            for (int n = -HALF_MASK_SIZE; n <= HALF_MASK_SIZE; n++) {
              int sensorX = (col + n) / FACTOR;
              int sensorY = (row + m) / FACTOR;
              int sensorIndex = (sensorX * 8) + sensorY;
              uint16_t color(colors[sensorIndex]);
              sumR += (color >> 8) & 0xF8;
              sumG += (color >> 3) & 0xFC;
              sumB += (color << 3) & 0xF8;
            }
          }
          int color = display_.color565(sumR / DIV, sumG / DIV, sumB / DIV);
          display_.fillRect(col, ROTATE_FACTOR - row, SUPER_PIXEL_SIZE, SUPER_PIXEL_SIZE, color);
        }
      }
      display_.endWrite();
    }
    void clampValues(int iVals[], float vals[], int nVals, int minVal, int maxVal) {
      for (int i = 0; i < nVals; i++) {
        if (vals[i] < minVal) {
          iVals[i] = minVal;
        } else {
          iVals[i] = (int)vals[i];
        }
        if (vals[i] > maxVal) {
          iVals[i] = maxVal;
        } else {
          iVals[i] = (int)vals[i];
        }
      }
    }
    uint16_t getColor(float percent) {
      const int START_RED = 173; // light blue
      const int START_GREEN = 216;
      const int START_BLUE = 230;
      const int END_RED = 255;
      const int END_GREEN = 0;
      const int END_BLUE = 0;
      return display_.color565((uint8_t)(abs(END_RED - START_RED) / percent),
                               (uint8_t)(abs(END_GREEN - START_GREEN) / percent),
                               (uint8_t)(abs(END_BLUE - START_BLUE) / percent));
    }
    void getMinMax(float vals[], int* min, int* max) {
      *min = INT_MAX;
      *max = INT_MIN;
      for (int i = 0; i < 64; i++) {
        if (vals[i] < *min) {
          *min = (int)vals[i];
        }
        if (vals[i] > *max) {
          *max = (int)vals[i];
        }
      }
    }
    void fillRectWH(int x0, int y0, int w, int h, int color) {
      display_.fillRect(x0, y0, w, h, color);
    }  
    void getTextBox(const GFXfont* font, String str, int textSize, int16_t x0, int16_t y0,
                    int16_t* x, int16_t* y, uint16_t* w, uint16_t* h) {
      display_.setFont(font);
      display_.setTextSize(textSize);
      display_.getTextBounds(str, x0, y0, x, y, w, h);
      // https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/issues/43
      *w += (textSize * 2);
      *h += (textSize * 2);
    }
    void displayAtXY_(String s, int textSize, uint16_t x0, uint16_t y0) {
      int16_t   x;
      int16_t   y;
      uint16_t  w;
      uint16_t  h;

      getTextBox(font24, s, 1, x0, y0, &x, &y, &w, &h);
      display_.fillRect(x0, y0, w < getWidth() ? w : getWidth(), h, COLOR_BLACK);
      display_.setTextColor(COLOR_WHITE);
      display_.setCursor(x0, y0 + h);
      display_.setFont(font24);
      display_.setTextSize(textSize);
      display_.print(s);
    }
    void clear_(String s, int textSize, uint16_t x0, uint16_t y0) {
      int16_t   x;
      int16_t   y;
      uint16_t  w;
      uint16_t  h;

      getTextBox(font24, s, 1, x0, y0, &x, &y, &w, &h);
      display_.fillRect(x0, y0, w < getWidth() ? w : getWidth(), h, COLOR_BLACK);
    }
    void displayNextToGrid_(String s, bool rightJustified) {
      fillRectWH(getHeight() + 1, 0, getWidth() - getHeight(), getHeight(), COLOR_BLACK);
      int16_t   x;
      int16_t   y;
      uint16_t  w;
      uint16_t  h;

      getTextBox(font24, "00:00:00", 1, 0, 0, &x, &y, &w, &h);
      display_.setTextColor(COLOR_WHITE);
      if (rightJustified) {
        display(s, font24, 1, getWidth() - w - 20, h); // right-justified
      } else {
        display(s, font24, 1, getHeight(), h); // left-justified
      }
    }
    void displayGridValues_(float vals[]) {
      int min;
      int max;
      getMinMax(vals, &min, &max);
      
      int16_t   x0;
      int16_t   y0;
      uint16_t  w;
      uint16_t  h;

      getTextBox(font18, "1", 1, 0, 0, &x0, &y0, &w, &h);
      for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
          int rotatedX = y;
          int rotatedY = 7 - x;
          int x0 = rotatedX * 64;
          int y0 = rotatedY * 64;
          int index = (y * 8) + x;
          uint16_t color = getColor((vals[index] - min) / (max - min));
          // display_.setTextColor(color);
          display(String((int)vals[index]), font18, 1, x0, y0 + h);
        }
      }
    }

    enum GridType { SMOOTHED, NOT_SMOOTHED, VALUES };
    String previousTemp;

  public:
    void clear() {
      display_.fillScreen(COLOR_BLACK);
    }
    void startup() {
      pinMode(backlightPin, OUTPUT);
      delay(1000);
      display_.begin(); //init library
      clear();
      display_.setRotation(1);
    }
    void turnBackLightOn() {
      digitalWrite(backlightPin, HIGH);
    }
    void turnBackLightOff() {
      digitalWrite(backlightPin, LOW);
    }
    void display(String s) {
      display(s, DEFAULT_FONT_SIZE, 10, 10);
    }
    void displayAtXY(String s, int textSize, uint16_t x0, uint16_t y0) {
      displayAtXY_(s, textSize, x0, y0);
    }
    void clear(String s, int textSize, uint16_t x0, uint16_t y0) {
      clear_(s, textSize, x0, y0);
    }
    void displayNextToGrid(String s, bool rightJustified) {
      displayNextToGrid_(s, rightJustified);
    }
    void displaySmoothedDynamicGrid(float vals[]) {
      int iVals[64];
      clampValues(iVals, vals, 64, displayParams.minTemp, displayParams.maxTemp);
      uint16_t colors[64];
      for (int i = 0; i < 64; i++) {
        int val = map(iVals[i], displayParams.minTemp, displayParams.maxTemp, 0, 255);
        colors[i] = display_.color565(val, 0, 0);
      }      
      doDisplaySmoothedDynamicGrid(colors, 64, getHeight(), getHeight());
    }
    void displayUnsmoothedDynamicGrid(float vals[]) {
      int min;
      int max;
      getMinMax(vals, &min, &max);
      display_.startWrite();
      for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
          int index = (y * 8) + x;
          int val = map(vals[index], min, max, 0, 255);
          int color = display_.color565(val, 0, 0);
          int rotatedX = y;
          int rotatedY = 7 - x;
          int blockWidth = getHeight() / 8;
          int x0 = rotatedX * blockWidth;
          int y0 = rotatedY * blockWidth;
          for (int i = 0; i < blockWidth; i++) {
            for (int j = 0; j < blockWidth; j++) {
              display_.drawPixel(x0 + j, y0 + i, color);
            }
          }
        }
      }
      display_.endWrite();
    }
    void displayGridValues(float vals[]) {
      displayGridValues_(vals);
    }
    void displayDynamicGrid(float vals[]) {
      GridType gridType = NOT_SMOOTHED;
      switch (gridType) {
        case SMOOTHED:
          displaySmoothedDynamicGrid(vals);
          break;
        case NOT_SMOOTHED:
          displayUnsmoothedDynamicGrid(vals);
          break;
        case VALUES:
          displayGridValues(vals);
          break;
      }
    }
    int getHeight() {
      return display_.height();
    }
    int getWidth() {
      return display_.width();
    }
    void showTemp(int temp) {
      String s(temp);
      s.concat(" f");
      if (previousTemp.length() > 0) {
        clear(previousTemp, 1, 0, 0);
      }
      previousTemp = s;
      displayAtXY(s, 1, 0, 0);
    }
};
#endif
OLEDWrapper oledWrapper;

#include <SparkFun_GridEYE_Arduino_Library.h>

class GridEyeSupport {
public:
  GridEYE grideye;
  int     mostRecentValue = INT_MIN;

  void begin() {
    grideye.begin();
  }

  float readOneSensor(int i) {
    return (grideye.getPixelTemperature(i) * 9.0 / 5.0 + 32.0);
  }

  String getRowAsString(int row) {
    String ret;
    for (int i = 0; i < 8; i++) {
      int val = (int)(readOneSensor((row * 8) + i));
      char buf[80];
      int len = snprintf(buf, 80, (val < 100) ? " %2d" : "%3d", val);
      if (len < 0) {
        ret.concat(" --");
      } else if (val > 0) {
        ret.concat(String(buf));
      } else {
        ret.concat(" --");
      }
    }
    return ret;
  }

  // This will timeout after 15-20 seconds if the GridEye isn't connected.
  int getMax() {
    int theMax = INT_MIN;
    for (int i = 0; i < 64; i++) {
      int t = (int)(readOneSensor(i));
      if (t > theMax) {
        theMax = t;
      }
    }
    return theMax;
  }
};
GridEyeSupport gridEyeSupport;

String Utils::msToString(unsigned long ms) {
  int totalSeconds = ms / 1000;
  int secs = totalSeconds % 60;
  int minutes = (totalSeconds / 60) % 60;
  int hours = (totalSeconds / 60) / 60;

  char buf[100];
  sprintf(buf, "%02u:%02u:%02u", hours, minutes, secs);
  return String(buf);
}
void Utils::publishWithSep(String s, String sep) {
  if (DO_SERIAL) {
    String s1(msToString(millis()));
    s1.concat(sep);
    s1.concat(s);
    Serial.println(s1);
  }
}
void Utils::publish(String s) {
  publishWithSep(s, " ");
}
String Utils::toString(bool b) {
  if (b) {
    return "true";
  }
  return "false";
}

#ifdef LOCAL_BUILD
const int DATA_ROWS = 821; // get this value from the output of the python script
String gridEyeRows[DATA_ROWS * 2] = {
#include "/home/ck/Documents/github/arduino-heat-sensor/data.txt"
};
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
class SensorData {
public:
  String    theTime;
  float     gridEyeValues[64];
  SensorData() {
    for (int i = 0; i < 64; i++) {
      gridEyeValues[i] = 0;
    }
  }
};
class DataProvider {
  private:
    void getItemAt(unsigned int index, SensorData& sensorData) {
      if (index >= DATA_ROWS) {
        Serial.println("index out of range: " + String(index) + " (max: " + String(DATA_ROWS - 1) + ")");
        return;
      }
      String t = gridEyeRows[index * 2];
      sensorData.theTime = t.substring(0, t.indexOf("."));
      String vals = gridEyeRows[index * 2 + 1];
      int charIndex = 1;
      for (int j = 0; j < 64; j++) {
        String valStr(vals[charIndex++]);
        valStr.concat(vals[charIndex++]);
        sensorData.gridEyeValues[j] = valStr.toFloat();
        charIndex++;
      }
    }
  public:
    void run() {
      oledWrapper.turnBackLightOn();
      for (unsigned int i = 0; i < DATA_ROWS; i++) {
        SensorData sensorData;
        getItemAt(i, sensorData);
        oledWrapper.displayUnsmoothedDynamicGrid(sensorData.gridEyeValues);
        oledWrapper.displayNextToGrid(sensorData.theTime, false);
        delay(2000);
      }
    }
};
#else
class DataProvider {
  public:
    void run() {}
};
#endif

class ButtonHandler {
  private:
    const int           BUTTON_PIN = 2;
    bool                lastButtonState = LOW;
    unsigned long       lastTimeButtonStateChanged = 0;
    const unsigned long DEBOUNCE_DURATION = 50; // milliseconds
  public:
    ButtonHandler() {
      pinMode(BUTTON_PIN, INPUT);
    }
    bool isPressed() {
      if (millis() - lastTimeButtonStateChanged > DEBOUNCE_DURATION) {
        byte buttonState = digitalRead(BUTTON_PIN);
        if (buttonState != lastButtonState) {
          lastTimeButtonStateChanged = millis();
          lastButtonState = buttonState;
          if (buttonState == HIGH) {
            return true;
          }
        }
      }
      return false;
    }
};
ButtonHandler buttonHandler;

class App {
  private:
    String configs[5] = {
      "Unit ID: " + unitID,
      "~Fri Oct  2 08:32:41 PM PDT 2026",
      "arduino-heat-sensor",
#ifdef USE_128_X_128
      "Using 128_X_128 OLED",
#else
      "Using GigaDisplay_GFX",
#endif
      "Production: " + Utils::toString(displayParams.PRODUCTION)
    };

    void status() {
      for (String s : configs) {
        Utils::publish(s);
      }
    }

    const float DELTA = 3.0; // degrees F
    float previousVals[64] = {-1.0};
    bool changed(float vals[64]) {
      if (vals[0] < 0.0) {
        for (int i = 0; i < 64; i++) {
          previousVals[i] = vals[i];
        }
        return false;
      }
      bool changed = false;
      for (int i = 0; i < 64; i++) {
        if (abs(vals[i] - previousVals[i]) > DELTA) {
          return true;
        }
      }
      return false;
    }
    void getVals(float vals[64]) {
      for (int i = 0; i < 64; i++) {
        vals[i] = gridEyeSupport.readOneSensor(i);
      }
    }
    void displayGrid(float vals[64]) {
      oledWrapper.displayDynamicGrid(vals);
    }
    void runDemoData() {
      DataProvider dataProvider;
      dataProvider.run();
    }
    void checkSerial() {
      if (Utils::DO_SERIAL) {
        if (Serial.available() > 0) {
          String teststr = Serial.readString();  //read until timeout
          teststr.trim();                        // remove any \r \n whitespace at the end of the String
          if (teststr.equals("?")) {
            status();
          } else if (teststr.equals("scan")) {
            Utils::scanI2C();
          } else if (teststr.equals("startTest")) {
            displayParams.setTestParams();
          } else if (teststr.equals("stopTest")) {
            displayParams.setParams();
          } else if (teststr.equals("runDemoData")) {
            runDemoData();
          } else {
            String msg("Unknown command: '");
            msg.concat(teststr);
            msg.concat("'. Expected ?, smooth, scan, startTest, stopTest, temp, unsmooth, or values");
            Utils::publish(msg);
            return;
          }
          String msg("Command done: ");
          msg.concat(teststr);
          Utils::publish(msg);
         }
      }
    }
    unsigned long mostRecentDisplayTime = 0;
    void displayElapsed() {
      if (mostRecentDisplayTime > 0) {
        unsigned long elapsed = millis() - mostRecentDisplayTime;
        oledWrapper.displayNextToGrid(Utils::msToString(elapsed), true);
      }
    }
    void display() {
      float vals[64];
      getVals(vals);
      if (changed(vals)) {
#ifdef USE_128_X_128
        oledWrapper.showTemp(gridEyeSupport.getMax());
#else
        oledWrapper.turnBackLightOn();
        if (showGrid) {
          displayGrid(vals);
        } else {
          oledWrapper.showTemp(gridEyeSupport.getMax());
        }
#endif
        for (int i = 0; i < 64; i++) {
          previousVals[i] = vals[i];
        }
      }
      displayElapsed();
    }
    void publishValuesAsString() {
      for (int i = 0; i < 8; i++) {
        Utils::publish(gridEyeSupport.getRowAsString(i));
      }
    }
    String getGridAsString() {
      String ret;
      for (int i = 0; i < 8; i++) {
        ret.concat(gridEyeSupport.getRowAsString(i));
        if (i < 7) {
          ret.concat(" ");
        }
      }
      return ret;
    }
    class CloudData {
      public:
        const unsigned long CLOUD_PUBLISH_RATE_IN_MS = 5 * 1000; // every 5 seconds
        unsigned long       lastCloudPublish = 0;

        String previousElapsedTime;
        String previousGridAsString;
        String previousMaxTemperature;
        void setCloudValues(String elapsedTime_, String gridAsString_, String maxTemperature_) {
          if (millis() - lastCloudPublish > CLOUD_PUBLISH_RATE_IN_MS) {
            lastCloudPublish = millis();
            if (!elapsedTime_.equals(previousElapsedTime)) {
              previousElapsedTime = elapsedTime_;
              elapsedTime = elapsedTime_;
            }
            if (!gridAsString_.equals(previousGridAsString)) {
              previousGridAsString = gridAsString_;
              gridAsString = gridAsString_;
            }
            if (!maxTemperature_.equals(previousMaxTemperature)) {
              previousMaxTemperature = maxTemperature_;
              maxTemperature = maxTemperature_;
            }
          }
        }
    } cloudData;
    bool thresholdReached() {
      for (int i = 0; i < 64; i++) {
        if (gridEyeSupport.readOneSensor(i) >= displayParams.threshold) {
          return true;
        }
      }
      return false;
    }
    void stopDisplay() {
      oledWrapper.clear();
      oledWrapper.turnBackLightOff();
      mostRecentDisplayTime = 0;
      cloudData.setCloudValues("--:--:--", " -- -- -- -- -- -- -- --", "--- f");
    }
  public:
    void setup() {
      cloudWrapper.setup();
      Wire.begin();
      if (Utils::DO_SERIAL) {
        Serial.begin(115200);
        delay(1000);
      }
      gridEyeSupport.begin();
      status();
      oledWrapper.startup();
      oledWrapper.clear();
      if (displayParams.PRODUCTION) {
        oledWrapper.display("Unit ID: " + unitID);
        delay(3000);
        oledWrapper.clear();
      }
      timeSupport = new TimeSupport();
      showGrid = false;
    }
    void loop() {
      cloudWrapper.loop();
      timeSupport->handleTime();
      if (thresholdReached()) {
        unsigned long thisMS = millis();
        if (mostRecentDisplayTime == 0) {
          mostRecentDisplayTime = thisMS;
        }
        display();
        unsigned long elapsed = thisMS - mostRecentDisplayTime;
        String maxT(gridEyeSupport.getMax());
        maxT.concat(" f");
        cloudData.setCloudValues(Utils::msToString(elapsed), getGridAsString(), maxT);
      } else {
        stopDisplay();
      }
      checkSerial();
    }
};
App app;

void setup() {
  app.setup();
}

void loop() {
  app.loop();
}

void onShowGridChange()  {
  oledWrapper.clear();
}
