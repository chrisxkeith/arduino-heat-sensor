// Please credit chris.keith@gmail.com .
// REM: Set board type.

// Thank you: https://github.com/ragnraok/android-image-filter
#include "./GaussianBlurFilter.h"

#include <U8g2lib.h>

class Utils {
  public:
    const static bool DO_SERIAL = true;
    static void publish(String s);
    static String toString(bool b);
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

// #define USE_128_X_128

#ifdef USE_128_X_128
U8G2_SSD1327_EA_W128128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
#else
#include <Wire.h>
#include <SparkFun_Qwiic_OLED.h> //http://librarymanager/All#SparkFun_Qwiic_OLED
#include <res/qw_fnt_8x16.h>
Qwiic1in3OLED u8g2; // 128x64
#endif

#ifdef USE_128_X_128
class OLEDWrapper {
  private:
      const int START_BASELINE = 50;
      int   baseLine = START_BASELINE;
  public:
    // For showing hands/fingers. Stove burner temps will be different.
    static const long   MIN_TEMP_IN_F = 80;   // degrees F that will display as black superpixel.
    static const long   MAX_TEMP_IN_F = 90;   // degrees F that will display as white superpixel.

    SuperPixelPatterns superPixelPatterns;

    void drawSuperPixel(uint16_t superPixelIndex, uint16_t startX, uint16_t startY) {
      for (uint16_t xi = 0; xi < SuperPixelPatterns::HORIZONTAL_SIZE; xi++) {
        for (uint16_t yi = 0; yi < SuperPixelPatterns::VERTICAL_SIZE; yi++) {
          if (superPixelPatterns.getPixelAt(superPixelIndex, xi + yi * SuperPixelPatterns::VERTICAL_COUNT)) {       
            u8g2.pixel(startX + xi, startY + yi);
          }
        }
      }
    }

    void displayGrid(float vals[SuperPixelPatterns::NUM_SUPER_PIXELS]) {
      float min = FLT_MAX;
      float max = -FLT_MAX;
      for (int i = 0; i < SuperPixelPatterns::NUM_SUPER_PIXELS; i++) {
        if (vals[i] < min) {
          min = vals[i];
        }
        if (vals[i] > max) {
          max = vals[i];
        }        
      }
      float range = max - min;
      clear();
      u8g2_prepare();
      u8g2.erase();
      u8g2.setDrawColor(1);
      for (int i = 0; i < SuperPixelPatterns::NUM_SUPER_PIXELS; i++) {
        uint16_t superPixelIndex = (uint16_t)round((vals[i] - min) / range * 
                                    SuperPixelPatterns::NUM_SUPER_PIXELS);
        uint16_t startX = (i % SuperPixelPatterns::HORIZONTAL_COUNT) * 
                                    SuperPixelPatterns::HORIZONTAL_SIZE;
        uint16_t startY = (i / SuperPixelPatterns::VERTICAL_COUNT) *
                                    SuperPixelPatterns::VERTICAL_SIZE;
        drawSuperPixel(superPixelIndex, startX, startY);
      }
      u8g2.display();
    }
};
#else
class OLEDWrapper {
  private:
      const int START_BASELINE = 20;
      int   baseLine = START_BASELINE;
      GaussianBlurFilter* gaussianBlurFilter = NULL;
  public:
    static const long   MIN_TEMP_IN_F = 80;   // degrees F that will display as black superpixel.
    static const long   MAX_TEMP_IN_F = 90;   // degrees F that will display as white superpixel.
    
    void dumpFilter() {
      String s("gaussianBlurFilter->sigma: ");
      s.concat(gaussianBlurFilter->sigma);
      Utils::publish(s);
      s.remove(0);
      s.concat("gaussianBlurFilter->kernelSum: ");
      s.concat(gaussianBlurFilter->kernelSum);
      Utils::publish(s);
      s.remove(0);
      s.concat("gaussianBlurFilter->maskSize: ");
      s.concat(gaussianBlurFilter->maskSize);
      Utils::publish(s);
      for (int x = 0; x < gaussianBlurFilter->maskSize; x++) {
        s.remove(0);
        for (int y = 0; y < gaussianBlurFilter->maskSize; y++) {
          s.concat(gaussianBlurFilter->kernel[x + y * gaussianBlurFilter->maskSize]);
          s.concat(" ");
        }
        Utils::publish(s);
      }
    }
    void setupBlurFilter() {
      {
        Timer t1("setupBlurFilter()");
        GaussianBlurOptions gbh(2.0);
        gaussianBlurFilter = new GaussianBlurFilter(NULL,
                                SuperPixelPatterns::HORIZONTAL_COUNT * SuperPixelPatterns::HORIZONTAL_SIZE,
                                SuperPixelPatterns::VERTICAL_COUNT * SuperPixelPatterns::VERTICAL_SIZE,
                                gbh);
      }
      dumpFilter();
    }
    void setup_OLED() {
      Wire.begin();
      delay(2000); // try to avoid u8g2.begin() failure.
      if (!u8g2.begin()) {
        Serial.println("u8g2.begin() failed! Stopping");
        while (true) { ; }
      }
      clear();
    }
    void drawInt(int val) {
      display(String(val));
    }
    void clear() {
      u8g2.erase();
      u8g2.display();
    }
    void shiftDisplay(int shiftAmount) {
        baseLine += shiftAmount;
        if (baseLine > 63) {
          baseLine = START_BASELINE;
        }
    }
    void superPixel(int xStart, int yStart, int pixelVal, int pixelIndex, int* bitMap) {
      if (pixelVal < 0) {
        pixelVal = 0;
      } else if (pixelVal >= SuperPixelPatterns::SUPER_PIXEL_SIZE) {
        pixelVal = SuperPixelPatterns::SUPER_PIXEL_SIZE - 1;
      }
      for (int xi = xStart; xi < xStart + SuperPixelPatterns::HORIZONTAL_SIZE; xi++) {
        for (int yi = yStart; yi < yStart + SuperPixelPatterns::VERTICAL_SIZE; yi++) {
            int drawColor;
            int r = (rand() % (SuperPixelPatterns::SUPER_PIXEL_SIZE - 2)) + 1;
            if (r < pixelVal) {
              drawColor = COLOR_WHITE;
            } else {
              drawColor = COLOR_BLACK;
            }
            if (bitMap == NULL) {
              u8g2.pixel(xi, yi, drawColor);
            } else {
              int v;
              if (drawColor == COLOR_WHITE) {
                v = 0;
              } else {
                v = 255;
              }
              int indexInBitmap = yi * SuperPixelPatterns::VERTICAL_SIZE + xi;
              bitMap[indexInBitmap++] = 255;
              bitMap[indexInBitmap++] = v;
              bitMap[indexInBitmap++] = v;
              bitMap[indexInBitmap] = v;
            }
        }
      }
    }
    void displayArray(int pixelVals[SuperPixelPatterns::NUM_SUPER_PIXELS]) {
      u8g2.erase();
      for (int i = 0; i < SuperPixelPatterns::NUM_SUPER_PIXELS; i++) {
        int x = (i % SuperPixelPatterns::HORIZONTAL_COUNT) * SuperPixelPatterns::HORIZONTAL_SIZE;
        int y = (i / SuperPixelPatterns::VERTICAL_COUNT) * SuperPixelPatterns::VERTICAL_SIZE;
        superPixel(y, x, pixelVals[i], i, NULL);
      }
      u8g2.display();
    }
    void displayBlurredArray(int pixelVals[SuperPixelPatterns::NUM_SUPER_PIXELS]) {
      unsigned long then = millis();
      int bitMap[ SuperPixelPatterns::HORIZONTAL_COUNT * SuperPixelPatterns::HORIZONTAL_SIZE *
                  SuperPixelPatterns::VERTICAL_COUNT * SuperPixelPatterns::VERTICAL_SIZE *
                  4 // AlphaRGB
      ];
      for (int i = 0; i < SuperPixelPatterns::NUM_SUPER_PIXELS; i++) {
        int x = (i % SuperPixelPatterns::HORIZONTAL_COUNT) * SuperPixelPatterns::HORIZONTAL_SIZE;
        int y = (i / SuperPixelPatterns::VERTICAL_COUNT) * SuperPixelPatterns::VERTICAL_SIZE;
        superPixel(y, x, pixelVals[i], i, bitMap);
      }
      blur(bitMap);
      u8g2.erase();
      for (int xi = 0; xi < SuperPixelPatterns::HORIZONTAL_COUNT * SuperPixelPatterns::HORIZONTAL_SIZE; xi++) {
        for (int yi = 0; yi < SuperPixelPatterns::VERTICAL_SIZE * SuperPixelPatterns::VERTICAL_SIZE; yi++) {
          int indexInBitmap = yi * SuperPixelPatterns::VERTICAL_SIZE + xi + 1;
          int totalColorSaturation = bitMap[indexInBitmap] + bitMap[indexInBitmap + 1] + bitMap[indexInBitmap + 2];
          int drawColor;
          if (totalColorSaturation > 768 / 2) {
            drawColor = COLOR_WHITE;
          } else {
            drawColor = COLOR_BLACK;
          }
          u8g2.pixel(xi, yi, drawColor);
        }
      }
      u8g2.display();
      unsigned long ms = millis() - then;
      String msg("milliseconds for displayBlurredArray: ");
      msg.concat(ms);
      Serial.println(msg);
    }

    void quickBlurOneBit(int targetBitmap[64][64], int bitX, int bitY, float factors[16], int sourceBitmap[64][64]) {
      // add vertical factors
      float accum = 0.0f;
      int minIndex = max(bitY - 16, 0);
      int maxIndex = min(bitY + 16, 63);
      for (int i = minIndex; i <= maxIndex; i++) {
        if (sourceBitmap[ bitX ][ i ] == 1) {
          accum += factors[ i ];
        }
      }
      // add horizontal factors
      minIndex = max(bitX - 16, 0);
      maxIndex = min(bitX + 16, 63);
      for (int i = minIndex; i <= maxIndex; i++) {
        if (sourceBitmap[ i ][ bitY ] == 1) {
          accum += factors[ i ];
        }
      }
      // Maximum value is 1's all the way across.
      float v = accum / gaussianBlurFilter->maskSize;
      if (v > 0.5) {
        targetBitmap[ bitX ][ bitY ] = 1;
      } else {
        targetBitmap[ bitX ][ bitY ] = 0;
      }
    }
    void blur(int* bitMap) {
      Timer t2("blur(int* bitMap)");
      gaussianBlurFilter->setPixels(bitMap,
                              SuperPixelPatterns::HORIZONTAL_COUNT * SuperPixelPatterns::HORIZONTAL_SIZE,
                              SuperPixelPatterns::VERTICAL_COUNT * SuperPixelPatterns::VERTICAL_SIZE
                              );
      gaussianBlurFilter->procImage();
    }
    void displayDynamicGrid(float vals[SuperPixelPatterns::NUM_SUPER_PIXELS]) {
      int pixelVals[SuperPixelPatterns::NUM_SUPER_PIXELS];
      for (int i = 0; i < SuperPixelPatterns::NUM_SUPER_PIXELS; i++) {
        long t = (long)round(vals[i]);
        pixelVals[i] = map(t, MIN_TEMP_IN_F, MAX_TEMP_IN_F, 0, SuperPixelPatterns::SUPER_PIXEL_SIZE);
        
      }
      displayArray(pixelVals);
    }
    void display(String s, uint8_t x, uint8_t y) {
      u8g2.setFont(QW_FONT_8X16);
      u8g2.text(x, y, s);
    }
    void endDisplay() {
      u8g2.display();
    }
    void display(String s) {
      u8g2.setFont(QW_FONT_8X16);
      u8g2.text(48, 0, s);
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

void Utils::publish(String s) {
  if (DO_SERIAL) {
    char buf[100];
    int totalSeconds = millis() / 1000;
    int secs = totalSeconds % 60;
    int minutes = (totalSeconds / 60) % 60;
    int hours = (totalSeconds / 60) / 60;
    sprintf(buf, "%02u:%02u:%02u", hours, minutes, secs);
    String s1(buf);
    s1.concat(" ");
    s1.concat(s);
    Serial.println(s1);
  }
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
#define SHOW_GRID true
    String configs[6] = {
      String(OLEDWrapper::MIN_TEMP_IN_F),
      String(OLEDWrapper::MAX_TEMP_IN_F),
      "Build 2024Sep23",
      "https://github.com/chrisxkeith/arduino-heat-sensor",
#if SHOW_GRID
      "showing grid",
#else
      "showing temp",
#endif
#ifdef USE_128_X_128
      "USE_128_X_128"
#else
      "not USE_128_X_128"
#endif
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
      oledWrapper.displayDynamicGrid(vals);
    }

    void displayRef() {
      int vals[64];
      for (int i = 0; i < 64; i++) {
        vals[i] = i;
      }
      oledWrapper.displayArray(vals);
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
      oledWrapper.displayBlurredArray(ref);
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
        oledWrapper.display(testDataNames[i]);
        delay(3000);
        oledWrapper.displayDynamicGrid(testData[i]);
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
          } else if (teststr.equals("ref")) {
            displayRef();
          } else if (teststr.equals("grid")) {
            displayGrid();
          } else if (teststr.equals("temp")) {
            oledWrapper.drawInt(temperatureMonitor.getValue());
          } else if (teststr.equals("values")) {
            Utils::publish(gridEyeSupport.getValuesAsString());
          } else if (teststr.equals("testgrids")) {
            displayTestGrids();
          } else if (teststr.equals("contrastgrid")) {
            displayContrastGrid();
          } else {
            String msg("Unknown command: ");
            msg.concat(teststr);
            msg.concat(". Expected ref, grid, temp, values, testgrids, contrastgrid");
            Utils::publish(msg);
            return;
          }
          delay(5000);
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
      oledWrapper.drawInt(temperatureMonitor.getValue());
#endif
    }
  public:
    App() {
      configs[0].remove(0);
      configs[0].concat("min: ");
      configs[0].concat(OLEDWrapper::MIN_TEMP_IN_F);
      configs[1].remove(0);
      configs[1].concat("max: ");
      configs[1].concat(OLEDWrapper::MAX_TEMP_IN_F);
    }
    void setup() {
      if (Utils::DO_SERIAL) {
        Serial.begin(115200);
        delay(1000);
      }
      Utils::publish("Started setup...");
      status();

      gridEyeSupport.begin();
      oledWrapper.setup_OLED();
      oledWrapper.setupBlurFilter();
      delay(1000);
      uint16_t baseline = 16;
      oledWrapper.display(configs[2], 0, baseline);
      oledWrapper.endDisplay();
      delay(5000);
      oledWrapper.clear();
      Utils::publish("Finished setup...");
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
        if (thisMS - lastShift > SHIFT_RATE) {
          oledWrapper.shiftDisplay(2);
          lastShift = thisMS;
        }
        display();
        lastDisplay = thisMS;
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
