#define LOCAL_BUILD

const String unitID = "* * * set this before compile * * *"; // 'n' for giga, 'Nano-Iot-n" for Nano 33 IoT

#ifdef LOCAL_BUILD
String elapsedTime;
String gridAsString;
class CloudWrapper {
  public:
    void setup() {}
    void loop() {}
};
#define CLOUD_TIME time_t
#else
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
};
void onElapsedTimeChange() {}
void onGridAsStringChange() {}
#define CLOUD_TIME CloudTime
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
#ifndef LOCAL_BUILD
  timeFromCloud = ArduinoCloud.getLocalTime();
#endif
  lastSyncMillis = millis();
}

unsigned long lastAttemptMillis = 0;
void TimeSupport::handleTime() {
  if (timeFromCloud == NO_TIME) {
    if (millis() - lastAttemptMillis > 1000 * 30) { // every 30 seconds until we can connect
      if (!ArduinoCloud.connected()) {
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
  s.concat(getCurrentTime());
  s.concat(", ArduinoCloud.getLocalTime(): ");
  s.concat(ArduinoCloud.getLocalTime());
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
    const int TEST_MIN_TEMP = 70; // for testing with skin temperature
    const int TEST_MAX_TEMP = 90;
    const int TEST_THRESHOLD = 80;

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
    bool  doSmoothing = false;
};
#else
const int COLOR_WHITE = 0x65535;
const int COLOR_BLACK = 0x0;
#include "Arduino_GigaDisplay_GFX.h"
#include "Fonts/FreeSans18pt7b.h"
#include "Fonts/Org_01.h"
#include "Fonts/Picopixel.h"
#include "Fonts/Tiny3x3a2pt7b.h"
#include "Fonts/TomThumb.h"

GigaDisplay_GFX display_;

class OLEDWrapper {
  private:
    uint16_t currentColor = COLOR_WHITE;
    const int DEFAULT_FONT_SIZE = 3;
  public:
    bool doSmoothing = false;
    void clear() {
      display_.fillScreen(COLOR_BLACK);
    }
    void startup() {
      delay(1000);
      display_.begin(); //init library
      clear();
      display_.setRotation(1);
    }
    void display(String s, const GFXfont* font, int textSize, uint16_t x, uint16_t y) {
      display_.setCursor(x, y);
      display_.setFont(font);
      display_.setTextSize(textSize);
      display_.print(s);
    }
    void display(String s, int textSize, uint16_t x, uint16_t y) {
      display(s, nullptr, textSize, x, y);
    }
    void display(String s) {
      display(s, DEFAULT_FONT_SIZE, 10, 10);
    }
    void displayNextToGrid(String s[], int nStrings) {
      int x0 = getHeight() + 10;
      fillRectWH(x0, 0, getWidth() - x0, getHeight(), COLOR_BLACK);
      for (int i = 0; i < nStrings; i++) {
        display(s[i], &FreeSans18pt7b, 1, x0 + 20, 32 + (i * 32));
      }
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
      int iVals[64];
      clampValues(iVals, vals, 64, displayParams.minTemp, displayParams.maxTemp);
      display_.startWrite();
      for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
          int index = (y * 8) + x;
          int val = map(vals[index], displayParams.minTemp, displayParams.maxTemp, 0, 255);
          int color = display_.color565(val, 0, 0);
          int rotatedX = y;
          int rotatedY = 7 - x;
          int x0 = rotatedX * 64;
          int y0 = rotatedY * 64;
          for (int i = 0; i < 64; i++) {
            for (int j = 0; j < 64; j++) {
              display_.drawPixel(x0 + j, y0 + i, color);
            }
          }
        }
      }
      display_.endWrite();
    }
    void displayDynamicGrid(float vals[]) {
      if (doSmoothing) {
        displaySmoothedDynamicGrid(vals);
      } else {
        displayUnsmoothedDynamicGrid(vals);
      }
    }
    void setDrawColor(int color) {
      currentColor = color;
    }
    void setFont(const GFXfont* font) {
      display_.setFont(font);
    }
    void getTextBoundsWH(String string, const GFXfont* font, int textSize,
                          int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
      display_.setFont(font);
      display_.setTextSize(textSize);
      display_.getTextBounds(string, x, y, x1, y1, w, h);
    }
    void getTextBounds(String string, const GFXfont* font, int textSize,
                          int16_t* x1, int16_t* y1, uint16_t* x2, uint16_t* y2) {
      uint16_t w;
      uint16_t h;
      getTextBoundsWH(string, font, textSize, 0, 0, x1, y1, &w, &h);
      *x2 = *x1 + w;
      *y2 = *y1 + h;
    }
    void drawLine(int x0, int y0, int x1, int y1) {
      // Rotate not happening automatically?
      display_.drawLine(y0, x0, y1, x1, currentColor);
    }
    void fillRect(int x0, int y0, int x1, int y1, int color) {
      display_.fillRect(x0, y0, x1 - x0, y1 - y0, color); // is there an off-by-one error here?
    }  
    void fillRectWH(int x0, int y0, int w, int h, int color) {
      display_.fillRect(x0, y0, w, h, color);
    }  
    int getHeight() {
      return display_.height();
    }
    int getWidth() {
      return display_.width();
    }
    void showTemp(int temp) {
      clear();
      String s("Temp: ");
      s.concat(temp);
      s.concat(" F");
      display(s, 3, 10, 32);
    }
    void shiftDisplay() {
    }
    void dump() {
      String s("OLEDWrapper: getHeight(): ");
      s.concat(getHeight());
      s.concat(", getWidth(): ");
      s.concat(getWidth());
      s.concat(", doSmoothing: ");
      s.concat(doSmoothing);
      Utils::publish(s);
    }
};
#endif
OLEDWrapper oledWrapper;

#include <SparkFun_GridEYE_Arduino_Library.h>
#include <limits.h>

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

class App {
  private:
    String configs[5] = {
      "Unit ID: " + unitID,
      "~Sat Sep 19 01:31:36 PM PDT 2026",
      "arduino-heat-sensor",
#ifdef USE_128_X_128
      "Using 128_X_128 OLED",
#else
      "Using GigaDisplay_GFX",
#endif
      "Production: " + Utils::toString(displayParams.PRODUCTION)
    };

    unsigned long lastDisplay = 0;
    int lastShift = 0;
    unsigned long mostRecentDisplayTime = 0;

    void status() {
      for (String s : configs) {
        Utils::publish(s);
      }
    }

    float previousVals[64] = {-1.0};
    bool changed(float vals[64]) {
      if (vals[0] < 0.0) {
        for (int i = 0; i < 64; i++) {
          previousVals[i] = vals[i];
        }
        return false;
      }
      const float DELTA = 3.0; // degrees F
      bool changed = false;
      for (int i = 0; i < 64; i++) {
        if (abs(vals[i] - previousVals[i]) > DELTA) {
          return true;
        }
      }
      return false;
    }
    void displayGrid() {
      float vals[64];
      for (int i = 0; i < 64; i++) {
        vals[i] = gridEyeSupport.readOneSensor(i);
      }
      if (changed(vals)) {
        oledWrapper.displayDynamicGrid(vals);
        for (int i = 0; i < 64; i++) {
          previousVals[i] = vals[i];
        }
      }
    }
    void checkSerial() {
      if (Utils::DO_SERIAL) {
        if (Serial.available() > 0) {
          String teststr = Serial.readString();  //read until timeout
          teststr.trim();                        // remove any \r \n whitespace at the end of the String
          if (teststr.equals("?")) {
            status();
            oledWrapper.dump();
          } else if (teststr.equals("smooth")) {
            oledWrapper.doSmoothing = true;
            oledWrapper.dump();
          } else if (teststr.equals("scan")) {
            Utils::scanI2C();
          } else if (teststr.equals("startTest")) {
            displayParams.setTestParams();
          } else if (teststr.equals("stopTest")) {
            displayParams.setParams();
          } else if (teststr.equals("unsmooth")) {
            oledWrapper.doSmoothing = false;
            oledWrapper.dump();
          } else if (teststr.equals("values")) {
            publishValuesAsString();
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
    void display() {
#ifdef USE_128_X_128
      oledWrapper.showTemp(gridEyeSupport.getMax());
#else
      displayGrid();
      if (mostRecentDisplayTime > 0) {
        unsigned long elapsed = millis() - mostRecentDisplayTime;
        elapsedTime = Utils::msToString(elapsed);
        String sArray[1];
        sArray[0] = elapsedTime;
        oledWrapper.displayNextToGrid(sArray, 1);
      }
#endif
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

  public:
    App() {
    }
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
    }
    void loop() {
      cloudWrapper.loop();
      timeSupport->handleTime();
      const int DISPLAY_RATE_IN_MS = 500;
      unsigned long thisMS = millis();
      if (thisMS - lastDisplay > DISPLAY_RATE_IN_MS) {
        const int SHIFT_RATE = 1000 * 60 * 2; // Shift display every 2 minutes to avoid OLED burn-in.
        // const int SHIFT_RATE = 1000 * 2; // Shift display every 2 seconds for debugging.
        if (thisMS - lastShift > SHIFT_RATE) {
          oledWrapper.shiftDisplay();
          lastShift = thisMS;
        }
        bool doDisplay = false;
        for (int i = 0; i < 64; i++) {
          if (gridEyeSupport.readOneSensor(i) >= displayParams.threshold) {
            doDisplay = true;
            break;
          }
        }
        if (doDisplay) {
          display();
  //        gridAsString = getGridAsString();
          lastDisplay = thisMS;
          if (mostRecentDisplayTime == 0) {
            mostRecentDisplayTime = thisMS;
          }
        } else {
          oledWrapper.clear();
          mostRecentDisplayTime = 0;
          if (!elapsedTime.equals("--:--:--")) {
            elapsedTime = "--:--:--";
          }
/*          if (!gridAsString.equals(" -- -- -- -- -- -- -- --")) {
            gridAsString = " -- -- -- -- -- -- -- --";
          }
*/        }
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
