// Please credit chris.keith@gmail.com .

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

#include <float.h>
#include <U8g2lib.h>

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
    bool doSmoothing = true;
    void clear() {
      display_.fillScreen(COLOR_BLACK);
    }
    void startup() {
      delay(1000);
      display_.begin(); //init library
      clear();
      display_.setRotation(1);
    }
    void display(String s, const GFXfont* font, int textSize, uint8_t x, uint8_t y) {
      display_.setCursor(x, y);
      display_.setFont(font);
      display_.setTextSize(textSize);
      display_.print(s);
    }
    void display(String s, int textSize, uint8_t x, uint8_t y) {
      display(s, nullptr, textSize, x, y);
    }
    void display(String s) {
      display(s, DEFAULT_FONT_SIZE, 10, 10);
    }
    void display(String s[], int nStrings) {
      for (int i = 0; i < nStrings; i++) {
        display(s[i], DEFAULT_FONT_SIZE, 10, 32 + (i * 32));
      }
    }
    void doDisplaySmoothedDynamicGrid(uint16_t colors[], int size, int width, int height) {
      const int   FACTOR = height / 8; // 8x8 sensor grid
      const int   MASK_SIZE = FACTOR / 2;
      const int   DIV = MASK_SIZE * MASK_SIZE;
      const int   HALF_MASK_SIZE = MASK_SIZE / 2;
      const int   SUPER_PIXEL_SIZE = 16;
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
    const int MIN_TEMP = 60;
    const int MAX_TEMP = 100;
    void displaySmoothedDynamicGrid(float vals[]) {
      int iVals[64];
      clampValues(iVals, vals, 64, MIN_TEMP, MAX_TEMP);
      uint16_t colors[64];
      for (int i = 0; i < 64; i++) {
        int val = map(iVals[i], MIN_TEMP, MAX_TEMP, 0, 255);
        colors[i] = display_.color565(val, 0, 0);
      }      
      doDisplaySmoothedDynamicGrid(colors, 64, getHeight(), getHeight());
    }
    void displayUnsmoothedDynamicGrid(float vals[]) {
      int iVals[64];
      clampValues(iVals, vals, 64, MIN_TEMP, MAX_TEMP);
      display_.startWrite();
      for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
          int index = (y * 8) + x;
          int val = map(vals[index], MIN_TEMP, MAX_TEMP, 0, 255);
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

  String getValuesAsString() {
    String ret;
    for(unsigned char i = 0; i < 64; i++) {
      char buf[80];
      int len = snprintf(buf, 80, "%2.0f", readOneSensor(i));
      if (len < 0) {
        ret.concat("error formatting value");
        break;
      } else {
        ret.concat(String(buf));
        if (i < 63) {
          ret.concat(",");
        }
      }
    }
    return ret;
  }

  // This will timeout after 15-20 seconds if the GridEye isn't connected.
  int readValue() {
    float total = 0;
    for (int i = 0; i < 64; i++) {
      int t = (int)(readOneSensor(i));
      total += t;
    }
    mostRecentValue = (int)(total / 64);
    return mostRecentValue;
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

class TemperatureMonitor {
  public:
    int getValue() {
      gridEyeSupport.readValue();
      return gridEyeSupport.mostRecentValue;
    }
   
};
TemperatureMonitor temperatureMonitor;

class App {
  private:
    const int THRESHOLD = 0; // degrees F
#define SHOW_GRID true
    String configs[5] = {
      "~Sun Aug 23 01:28:55 PM PDT 2026",
      "arduino-heat-sensor",
#if SHOW_GRID
      "showing grid",
#else
      "showing temp",
#endif
      "Using GigaDisplay_GFX",
      "placeholder for threshold"
    };

    int lastDisplay = 0;
    int lastShift = 0;

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

    // Read, blur and display should be < 400 ms.
    void displayGrid() {
      // Timer t("displayGrid()");
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
          } else if (teststr.equals("temp")) {
            oledWrapper.showTemp(temperatureMonitor.getValue());
          } else if (teststr.equals("unsmooth")) {
            oledWrapper.doSmoothing = false;
            oledWrapper.dump();
          } else if (teststr.equals("values")) {
            Utils::publish(gridEyeSupport.getValuesAsString());
          } else {
            String msg("Unknown command: '");
            msg.concat(teststr);
            msg.concat("'. Expected ?, smooth, scan, temp, unsmooth, or values");
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
#if SHOW_GRID
      displayGrid();
#else
      oledWrapper.showTemp(temperatureMonitor.getValue());
#endif
    }
  public:
    App() {
    }
    void setup() {
      Wire.begin();
      if (Utils::DO_SERIAL) {
        Serial.begin(115200);
        delay(1000);
      }
      gridEyeSupport.begin();
      String thresholdStr("Threshold: ");
      thresholdStr.concat(THRESHOLD);
      thresholdStr.concat(" F");
      configs[4] = thresholdStr;
      status();
      oledWrapper.startup();
      String initialMsgs[3] = { configs[0], configs[1], configs[4] };
      oledWrapper.clear();
    }
    void loop() {
#if SHOW_GRID
      const int DISPLAY_RATE_IN_MS = 1;
#else
      const int DISPLAY_RATE_IN_MS = 2000;
#endif
      int thisMS = millis();
      if (thisMS - lastDisplay > DISPLAY_RATE_IN_MS) {
        const int SHIFT_RATE = 1000 * 60 * 2; // Shift display every 2 minutes to avoid OLED burn-in.
        // const int SHIFT_RATE = 1000 * 2; // Shift display every 2 seconds for debugging.
        if (thisMS - lastShift > SHIFT_RATE) {
          oledWrapper.shiftDisplay();
          lastShift = thisMS;
        }
        if (temperatureMonitor.getValue() >= THRESHOLD) {
          display();
          lastDisplay = thisMS;
        } else {
          oledWrapper.clear();
        }
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
