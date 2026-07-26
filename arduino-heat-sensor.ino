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

    // Modified from https://playground.arduino.cc/Main/I2cScanner/
    static void scanI2C() {
      Wire.begin();
    
      Serial.println("I2C: Scanning for devices...");    
      std::vector<byte> foundDevices;
      std::set<byte> errors;
      for( byte address = 1; address < 127; address++ ) {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
    
        if (error == 0) {
          foundDevices.push_back(address);
        } else {
          errors.insert(error);
        }    
      }
      Serial.print("I2C: Devices found at: ");
      for (byte b : foundDevices) {
        Serial.print("0x");
        if (b < 16) {
          Serial.print("0");
        }
        Serial.print(b, HEX);
        Serial.print(" ");
      }
      Serial.println("");
      String ss("I2C: Error numbers returned: ");
      std::set<byte>::iterator itr;
      for (itr = errors.begin(); itr != errors.end(); itr++) {
        ss.concat(*itr);
        ss.concat(" ");
      }
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
      String msg("milliseconds for ");
      msg.concat(this->msg);
      msg.concat(": ");
      msg.concat(ms);
      Serial.println(msg);
    }
};

#include <SparkFun_Qwiic_OpenLog_Arduino_Library.h>

class DataLogger {
  private:
     OpenLog    openLog;
     String     fileName;
  public:
    DataLogger(String fileName) {
      Wire.begin();
      openLog.begin();
      this->fileName = fileName;
    }
    void writeFile(std::vector<String> lines) {
      if (openLog.size(this->fileName) > 0) {
        uint32_t retCode = openLog.removeFile(this->fileName); 
        if (retCode != 1) {
          String s("Unable to remove: ");
          s.concat(this->fileName);
          s.concat(", return code: ");
          s.concat(retCode);
          Serial.println(s);
          return;
        };
      }
      if (!openLog.append(this->fileName)) {
          String s("Unable to append: ");
          s.concat(this->fileName);
          Serial.println(s);
          return;
      }
      for (const String &s : lines) {
        if (s.length() > 0) {
          if (openLog.println(s) == 0) {
            String err("Error writing string: ");
            err.concat(s);
            Serial.println(err);
            break;
          }
        }
      }
      openLog.syncFile();
    }
    void readFile(std::vector<String*>* lines) {
      long sizeOfFile = openLog.size(this->fileName);
      if (sizeOfFile <= 0) {
        String s("openLog.size returned: ");
        s.concat(sizeOfFile);
        s.concat(" for file: ");
        s.concat(this->fileName);
        Serial.println(s);
        return;
      }
      byte* buf = new byte[sizeOfFile];
      if (buf == nullptr) {
        String s("Failed to alloc buf of size: ");
        s.concat(sizeOfFile);
        Serial.println(s);
        return;
      }
      openLog.read(buf, (uint16_t)sizeOfFile, this->fileName);
      const byte CR = 13;
      const byte LF = 10;
      String line;
      for (int x = 0; x < sizeOfFile ; x++) {
        byte b = buf[x];
        if (b == CR) {
          continue;
        }
        if (b == LF) {
          lines->push_back(new String(line));
          line.remove(0);
          continue;
        }
        line.concat((char)b);      
      }
      delete buf;
    }
    void test() {
      randomSeed(analogRead(0));
      String lineToWrite("Some junk text to test the DataLogger - ");
      lineToWrite.concat(random(10000));
      std::vector<String>  example;
      example.push_back(lineToWrite);
      this->fileName = "test.txt";
      String m("About to write line: ");
      m.concat(lineToWrite);
      Serial.println(m);
      writeFile(example);
      delay(5000);
      std::vector<String*> linesRead;
      readFile(&linesRead);
      String msg("Read the line: ");
      if (linesRead.size() == 0) {
        msg.concat("[no line!]");
      } else {
        msg.concat(*linesRead.at(0));
      }
      Serial.println(msg);
    }
};
DataLogger* datalogger = nullptr;

#include <bitset>
class SuperPixelPatterns {
  public:
    const static uint16_t HORIZONTAL_COUNT = 8;
    const static uint16_t VERTICAL_COUNT = 8;
    const static uint16_t NUM_SUPER_PIXELS = HORIZONTAL_COUNT * VERTICAL_COUNT;
    const static uint16_t HORIZONTAL_SIZE = 8;
    const static uint16_t VERTICAL_SIZE = 8;
    const static uint16_t SUPER_PIXEL_SIZE = HORIZONTAL_SIZE * VERTICAL_SIZE;
  private:
    std::bitset<SUPER_PIXEL_SIZE> patterns[NUM_SUPER_PIXELS];

  public:
    SuperPixelPatterns() {
      for (int superPixelIndex = 0; superPixelIndex < NUM_SUPER_PIXELS; superPixelIndex++) {
        for (int pixelPosition = 0; pixelPosition < SUPER_PIXEL_SIZE; pixelPosition++) {
          bool bitValue = (rand() % NUM_SUPER_PIXELS) < superPixelIndex;
          patterns[superPixelIndex][pixelPosition] = bitValue;
        }
      }
    }
    bool getPixelAt(int superPixelIndex, int pixelPosition) {
      return patterns[superPixelIndex][pixelPosition];
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
    void setupBlurFilter() {
    }
    void shiftDisplay() {
    }
    void dump() {
      String s("OLEDWrapper: getHeight(): ");
      s.concat(getHeight());
      s.concat(", getWidth(): ");
      s.concat(getWidth());
      Utils::publish(s);
    }
    void test2() {
      for (int r = 0; r < 4; r++) {
        clear();
        display_.setRotation(r);
        for (int i = 0; i < 24; i++) {
          int x = 0;
          int y = i * 20;
          String s(i);
          s.concat(",");
          s.concat(x);
          s.concat(",");
          s.concat(y);
          s.concat(",");
          s.concat(getWidth());
          s.concat(",");
          s.concat(getHeight());
          display(s, 2, x, y);
          Utils::publish(s);
          delay(1000);
        }
        delay(5000);
      }
    }
    void oneFontTest(const GFXfont* font, String fontName) {
      int16_t x1;
      int16_t y1;
      uint16_t w;
      uint16_t h;

      clear();
      setFont(font);
      display_.setTextSize(1);
      display_.getTextBounds(fontName, 0, 0, &x1, &y1, &w, &h);
      String s(fontName);
      s.concat(", w: ");
      s.concat(w);
      s.concat(", h: ");
      s.concat(h);
      display(s, 1, 10, 10);
      Utils::publish(s);
      delay(10000);
    }
    void fontTest() {
      oneFontTest(&Org_01, "Org_01");
      oneFontTest(&Picopixel, "Picopixel");
      oneFontTest(&Tiny3x3a2pt7b, "Tiny3x3a2pt7b");
      oneFontTest(&TomThumb, "TomThumb");
      oneFontTest(&FreeSans18pt7b, "FreeSans18pt7b");
      setFont(nullptr);
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

#include <map>
class SavedValues {
  private:
    const static int      NUM_SAVED_VALS = 60 * 6; // 6 hours @ one every minute
    std::map<String, int> savedValues;
    unsigned long         lastSavedTimeStamp = 0;
  public:
    void doSaveValue() {
      unsigned long now = millis();
      savedValues[Utils::msToString(now)] = gridEyeSupport.readValue();
      lastSavedTimeStamp = now;
    }
    void saveValue() {
      if (savedValues.size() < NUM_SAVED_VALS) {
        unsigned long now = millis();
        if (now - lastSavedTimeStamp > 1000 * 60) { // every minute, roughly
          doSaveValue();
        }
      }
    }
    void dumpHistory() {
      for(std::map<String, int>::iterator it = savedValues.begin();
          it != savedValues.end();
          it++) {
        String s(it->first);
        s.concat(",");
        s.concat(it->second);
        Serial.println(s);
      }
    }
};
SavedValues savedValues;

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
    const int THRESHOLD = 85; // degrees F
#define SHOW_GRID false
    String configs[5] = {
      "~2025Nov25:16:45", // date +"%Y%b%d:%H:%M"
      "arduino-heat-sensor",
#if SHOW_GRID
      "showing grid",
#else
      "showing temp",
#endif
#ifdef USE_128_X_128
      "Using 128x128",
#else
      "Using 128x64",
#endif
      "placeholder for threshold"
    };

    int lastDisplay = 0;
    int lastShift = 0;

    void status() {
      for (String s : configs) {
        Utils::publish(s);
      }
    }

    // Read, blur and display should be < 400 ms.
    void displayGrid() {
      // Timer t("displayGrid()");
      float vals[64];
      for (int i = 0; i < 64; i++) {
        vals[i] = gridEyeSupport.readOneSensor(i);
      }
//      oledWrapper.displayDynamicGrid(vals);
    }

    void displayRef() {
      int vals[64];
      for (int i = 0; i < 64; i++) {
        vals[i] = i;
      }
//      oledWrapper.displayArray(vals);
    }

    void displayContrastGrid() {
      int ref[] = { 0, 1, 2, 3, 4, 5, 6, 7,
                    7, 6, 5, 4, 3, 2, 1, 0,
                    1, 2, 3, 4, 5, 6, 7, 0,
                    7, 6, 5, 4, 3, 2, 1, 0,
                    2, 3, 4, 5, 6, 7, 0, 1,
                    7, 6, 5, 4, 3, 2, 1, 0,
                    3, 4, 5, 6, 7, 0, 1, 2,
                    7, 6, 5, 4, 3, 2, 1, 0
                  };
      for (int i = 0; i < 64; i++) {
        ref[i] *= 8;
      }
//      oledWrapper.displayBlurredArray(ref);
    }

    void displayTestGrids() {
      const int NUM_TESTDATA = 1;
      float d1[] = {80,81,80,81,84,96,98,90,81,82,82,83,86,135,133,94,83,83,83,85,87,98,107,91,87,90,90,87,90,100,95,90,95,134,144,101,113,146,132,95,98,137,131,105,123,137,128,95,90,98,104,92,98,116,100,92,86,89,86,86,87,90,89,88};
      float* testData[NUM_TESTDATA] = {
        d1
      };
      String testDataNames[NUM_TESTDATA] = {
        "3burners"
      };
      for (int i = 0; i < NUM_TESTDATA; i++) {
//        oledWrapper.display(testDataNames[i]);
        delay(3000);
//        oledWrapper.displayDynamicGrid(testData[i]);
        delay(3000);
      }
    }

    void checkSerial() {
      if (Utils::DO_SERIAL) {
        if (Serial.available() > 0) {
          String teststr = Serial.readString();  //read until timeout
          teststr.trim();                        // remove any \r \n whitespace at the end of the String
          if (teststr.equals("?")) {
            status();
          } else if (teststr.equals("contrastgrid")) {
            displayContrastGrid();
          } else if (teststr.equals("grid")) {
            displayGrid();
          } else if (teststr.equals("history")) {
            savedValues.dumpHistory();
          } else if (teststr.equals("ref")) {
            displayRef();
          } else if (teststr.equals("scan")) {
            Utils::scanI2C();
          } else if (teststr.equals("temp")) {
            oledWrapper.showTemp(temperatureMonitor.getValue());
          } else if (teststr.equals("testgrids")) {
            displayTestGrids();
          } else if (teststr.equals("values")) {
            Utils::publish(gridEyeSupport.getValuesAsString());
          } else {
            String msg("Unknown command: '");
            msg.concat(teststr);
            msg.concat("'. Expected contrastgrid, grid, history, ref, scan, temp, testgrids or values");
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
    void extraSetupStart() {
      Utils::publish("Started setup...");
      status();
//      datalogger = new DataLogger("heatdata.txt");
    }
    void extraSetupFinish() {
/*      oledWrapper.setupBlurFilter();
      delay(1000);
      oledWrapper.startup();
      uint16_t baseline = 16;
      for (String s : configs) {
        oledWrapper.display(s, 0, baseline);
        baseline += 16;
      }
      oledWrapper.endDisplay();
      delay(5000);
      oledWrapper.clear();
      savedValues.doSaveValue();
      Utils::scanI2C();
//      datalogger->test();
      Utils::publish("Finished setup...");
*/   }
  public:
    App() {
    }
    void setup() {
      if (Utils::DO_SERIAL) {
        Serial.begin(115200);
        delay(1000);
      }
      Utils::scanI2C();
      // extraSetupStart();
  /*    gridEyeSupport.begin();
      String thresholdStr("Threshold: ");
      thresholdStr.concat(THRESHOLD);
      thresholdStr.concat(" F");
      configs[4] = thresholdStr;
      status();
      oledWrapper.setup_OLED();
      String initialMsgs[3] = { configs[0], configs[1], configs[4] };
      oledWrapper.showMessages(initialMsgs, 3);
      delay(4000);
      oledWrapper.clear();
*/      // extraSetupFinish();
    }
    void loop() {
/*
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
      savedValues.saveValue();
*/      checkSerial();
    }
};
App app;

void setup() {
  app.setup();
}

void loop() {
  app.loop();
}
