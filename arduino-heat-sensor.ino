// Please credit chris.keith@gmail.com .
// REM: Set board type.

#include <U8g2lib.h>

class Utils {
  public:
    const static bool DO_SERIAL = true;
    static void publish(String s);
    static String toString(bool b);
};

#include <bitset>
class SuperPixelPatterns {
  public:
    const static uint16_t HORIZONTAL_COUNT = 8;
    const static uint16_t VERTICAL_COUNT = 8;
    const static uint16_t NUM_SUPER_PIXELS = HORIZONTAL_COUNT * VERTICAL_COUNT;
    const static uint16_t HORIZONTAL_SIZE = 16;
    const static uint16_t VERTICAL_SIZE = 16;
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

class Blurrer {
  public:
    int blurRadius = 0;
    int blurKernelSize = 0;
    int* blurKernel;
    int* blurMult;

    void buildBlurKernel(float r) {
      int radius = (int) (r * 3.5f);
      if (radius < 1) radius = 1;
      if (radius > 248) radius = 248;
      if (blurRadius != radius) {
        blurRadius = radius;
        blurKernelSize = (1 + blurRadius) << 1;
        blurKernel = new int[blurKernelSize];
        for (int i = 0; i < blurKernelSize; i++) {
          blurKernel[i] = 0;
        }
        blurMult = new int[blurKernelSize * 256];
        for (int i = 0; i < blurKernelSize * 256; i++) {
          blurMult[i] = 0;
        }

        int bk;
        int bki;
        int* bm;
        int* bmi;

        for (int i = 1, radiusi = radius - 1; i < radius; i++) {
          bki = radiusi * radiusi;
          blurKernel[radiusi] = bki;
          blurKernel[radius + i] = blurKernel[radiusi];
          bm = &blurMult[radius + i];
          bmi = &blurMult[radiusi--];
          for (int j = 0; j < 256; j++) {
            bmi[j] = bki * j;
            bm[j] = bmi[j];
          }
        }
        blurKernel[radius] = radius * radius;
        bk = blurKernel[radius];
        bm = &blurMult[radius];
        for (int j = 0; j < 256; j++) {
          bm[j] = bk * j;
        }
      }
    }

    void blur(float r, float* vals, int valsLength, int pixelHeight, int pixelWidth) {
      buildBlurKernel(r);
      
      int* r2 = new int[valsLength];
      int yi = 0;
      int ym = 0;
      int ymi = 0;

      for (int y = 0; y < pixelHeight; y++) {
        for (int x = 0; x < pixelWidth; x++) {
          int sum, cr;
          int read, ri, bk0;
          cr = sum = 0;
          read = x - blurRadius;
          if (read < 0) {
            bk0 = -read;
            read = 0;
          } else {
            if (read >= pixelWidth) {
              break;
            }
            bk0 = 0;
          }
          for (int i = bk0; i < blurKernelSize; i++) {
            if (read >= pixelWidth) {
              break;
            }
            int c = vals[read + yi];
            int* bm = &blurMult[i];
            cr += bm[(c & 1 /* RED_MASK */) >> 16];
            sum += blurKernel[i];
            read++;
          }
          ri = yi + x;
          r2[ri] = cr / sum;
        }
        yi += pixelWidth;
      }
      yi = 0;
      ym = -blurRadius;
      ymi = ym * pixelWidth;

      for (int y = 0; y < pixelHeight; y++) {
        for (int x = 0; x < pixelWidth; x++) {
          int sum, cr;
          int read, ri, bk0;
          cr = sum = 0;
          if (ym < 0) {
            bk0 = ri = -ym;
            read = x;
          } else {
            if (ym >= pixelHeight) {
              break;
            }
            bk0 = 0;
            ri = ym;
            read = x + ymi;
          }
          for (int i = bk0; i < blurKernelSize; i++) {
            if (ri >= pixelHeight) {
              break;
            }
            int* bm = &blurMult[i];
            cr += bm[r2[read]];
            sum += blurKernel[i];
            ri++;
            read += pixelWidth;
          }
          vals[x + yi] = 0; // 0xff000000 | (cr/sum)<<16 | (cg/sum)<<8 | (cb/sum);
        }
        yi += pixelWidth;
        ymi += pixelWidth;
        ym++;
      }
    }

    void diagnostics() {
      String s("blurRadius: ");
      s.concat(blurRadius);
      Utils::publish(s);
      String s2("blurKernelSize: ");
      s2.concat(blurKernelSize);
      Utils::publish(s2);

      int blurMultSize = blurKernelSize * 256;
      String s9("blurMultSize:");
      s9.concat(blurMultSize);
      Utils::publish(s9);

      String s3("blurKernel: ");
      for (int i = 0; i < blurKernelSize; i++ ) {
        s3.concat(blurKernel[i]);
        s3.concat(" ");
      }
      Utils::publish(s3);
/*      String s4("blurMult: ");
      for (int i = 0; i < blurKernelSize; i++ ) {
        s3.concat(blurMult[i]);
        s3.concat(" ");
      }
      Utils::publish(s3); */
    }
  private:
    float accessPixel(float * arr, int col, int row, int k, int width, int height) {
        float kernel[3][3] = {  1, 2, 1,
                                2, 4, 2,
                                1, 2, 1 };
        float sum = 0;
        float sumKernel = 0;

        for (int j = -1; j <= 1; j++) {
            for (int i = -1; i <= 1; i++) {
                if ((row + j) >= 0 && (row + j) < height && (col + i) >= 0 && (col + i) < width) {
                    float color = arr[(row + j) * 3 * width + (col + i) * 3 + k];
                    sum += color * kernel[i + 1][j + 1];
                    sumKernel += kernel[i + 1][j + 1];
                }
            }
        }
        return sum / sumKernel;
    }
  public:
    void guassian_blur2D(float * arr, float * result, int width, int height) {
        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width; col++) {
                for (int k = 0; k < 3; k++) {
                    result[3 * row * width + 3 * col + k] = accessPixel(arr, col, row, k, width, height);
                }
            }
        }
    }
};

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
    void u8g2_prepare(void) {
      u8g2.setFont(u8g2_font_fur49_tn);
      u8g2.setFontRefHeightExtendedText();
      u8g2.setDrawColor(1);
      u8g2.setFontDirection(0);
    }
    
     void drawInt(int val) {
      u8g2_prepare();
      u8g2.erase();
      u8g2.drawUTF8(2, this->baseLine, String(val).c_str());
      u8g2.setFont(u8g2_font_fur11_tf);
      u8g2.drawUTF8(6, this->baseLine + 20, "Fahrenheit");
      u8g2.display();
    }

    void clear() {
      u8g2.erase();
      u8g2.display();
    }

    void shiftDisplay(int shiftAmount) {
      this->baseLine += shiftAmount;
      if (this->baseLine > 70) {
        this->baseLine = START_BASELINE;
      }
    }

    void setup_OLED() {
      pinMode(10, OUTPUT);
      pinMode(9, OUTPUT);
      digitalWrite(10, 0);
      digitalWrite(9, 0);
      u8g2.begin();
      u8g2.setBusClock(400000);
    }

    void drawSuperPixel(uint16_t superPixelIndex, uint16_t startX, uint16_t startY) {
      for (uint16_t xi = 0; xi < SuperPixelPatterns::HORIZONTAL_SIZE; xi++) {
        for (uint16_t yi = 0; yi < SuperPixelPatterns::VERTICAL_SIZE; yi++) {
          if (superPixelPatterns.getPixelAt(superPixelIndex, xi + yi * SuperPixelPatterns::VERTICAL_COUNT)) {       
            u8g2.pixel(startX + xi, startY + yi);
          }
        }
      }
    }

    void superPixel(int xStart, int yStart, int pixelVal, int pixelIndex) {
      if (pixelVal < 0) {
        pixelVal = 0;
      } else if (pixelVal >= SuperPixelPatterns::SUPER_PIXEL_SIZE) {
        pixelVal = SuperPixelPatterns::SUPER_PIXEL_SIZE - 1;
      }
      int pixelIndexInSuperPixel = 0;
      for (int xi = xStart; xi < xStart + SuperPixelPatterns::HORIZONTAL_SIZE; xi++) {
        for (int yi = yStart; yi < yStart + SuperPixelPatterns::VERTICAL_SIZE; yi++) {
            int drawColor;
            int r = (rand() % (SuperPixelPatterns::SUPER_PIXEL_SIZE - 2)) + 1; // TODO: try influencing by values of neighboring superpixels
            if (r < pixelVal) {
              drawColor= 1;
            } else {
              drawColor= 0;
            }
            u8g2.setDrawColor(drawColor);
            u8g2.pixel(xi, yi);
        }
      }
    }

    void displayArray(int pixelVals[SuperPixelPatterns::NUM_SUPER_PIXELS]) {
      u8g2_prepare();
      u8g2.erase();
      for (int i = 0; i < SuperPixelPatterns::NUM_SUPER_PIXELS; i++) {
        int x = (i % SuperPixelPatterns::HORIZONTAL_COUNT) * SuperPixelPatterns::HORIZONTAL_SIZE;
        int y = (i / SuperPixelPatterns::VERTICAL_COUNT) * SuperPixelPatterns::VERTICAL_SIZE;
        superPixel(y, x, pixelVals[i], i);
      }
      u8g2.display();
    }

    void displayBlurredArray(int pixelVals[SuperPixelPatterns::NUM_SUPER_PIXELS]) {
      float vals[ SuperPixelPatterns::NUM_SUPER_PIXELS * 
                  SuperPixelPatterns::HORIZONTAL_SIZE * 
                  SuperPixelPatterns::VERTICAL_SIZE];
      float blurredVals[SuperPixelPatterns::NUM_SUPER_PIXELS * 
                        SuperPixelPatterns::HORIZONTAL_SIZE * 
                        SuperPixelPatterns::VERTICAL_SIZE];
      // Fill local buffer
      Blurrer* b = new Blurrer;
      b->guassian_blur2D(vals, blurredVals,
                                SuperPixelPatterns::HORIZONTAL_COUNT * 
                                    SuperPixelPatterns::HORIZONTAL_SIZE, 
                                SuperPixelPatterns::VERTICAL_COUNT * 
                                    SuperPixelPatterns::VERTICAL_SIZE);
      u8g2_prepare();
      u8g2.erase();
      int drawColor;
      long threshold = (long)round((MAX_TEMP_IN_F - MIN_TEMP_IN_F) / 2 + MIN_TEMP_IN_F);
      for (int xi = 0; xi < SuperPixelPatterns::HORIZONTAL_SIZE * 
                      SuperPixelPatterns::HORIZONTAL_COUNT; xi++) {
        for (int yi = 0; yi < SuperPixelPatterns::VERTICAL_SIZE *
                      SuperPixelPatterns::VERTICAL_COUNT; yi++) {
          long t = (long)round(blurredVals[xi * yi]);
          long r = map(t, MIN_TEMP_IN_F, MAX_TEMP_IN_F, 0, 
                                SuperPixelPatterns::SUPER_PIXEL_SIZE);
          if (r > threshold) {
            drawColor = 1;
          } else {
            drawColor = 0;
          }
          u8g2.setDrawColor(drawColor);
          u8g2.pixel(xi, yi);
        }
      }
      u8g2.display();
    }

    void displayDynamicGrid(float vals[SuperPixelPatterns::NUM_SUPER_PIXELS]) {
      int pixelVals[SuperPixelPatterns::NUM_SUPER_PIXELS];
      for (int i = 0; i < SuperPixelPatterns::NUM_SUPER_PIXELS; i++) {
        long t = (long)round(vals[i]);
        pixelVals[i] = map(t, MIN_TEMP_IN_F, MAX_TEMP_IN_F, 0, SuperPixelPatterns::SUPER_PIXEL_SIZE);
        
      }
      displayArray(pixelVals);
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

    void startDisplay(const uint8_t *font) {
      u8g2_prepare();
      u8g2.erase();
      u8g2.setFont(font);
    }
    void display(String s, uint8_t x, uint8_t y) {
      u8g2.setCursor(x, y);
      u8g2.print(s.c_str());
    }
    void endDisplay() {
      u8g2.display();
    }
    void display(String s) {
      startDisplay(u8g2_font_fur11_tf);
      display(s, 0, 16);
      endDisplay();
    }
};
#else
class OLEDWrapper {
  public:
    static const long   MIN_TEMP_IN_F = 80;   // degrees F that will display as black superpixel.
    static const long   MAX_TEMP_IN_F = 90;   // degrees F that will display as white superpixel.
    
    void setup_OLED() {
      Wire.begin();
      if (!u8g2.begin()) {
        Serial.println("u8g2.begin() failed! Stopping");
        while (true) { ; }
      }
      clear();
    }
    void drawInt(int val) {
    }
    void clear() {
      u8g2.erase();
      u8g2.display();
    }
    void shiftDisplay(int shiftAmount) {
    }
    void drawSuperPixel(uint16_t superPixelIndex, uint16_t startX, uint16_t startY) {
    }
    void superPixel(int xStart, int yStart, int pixelVal, int pixelIndex) {
    }
    void displayArray(int pixelVals[SuperPixelPatterns::NUM_SUPER_PIXELS]) {
    }
    void displayBlurredArray(int pixelVals[SuperPixelPatterns::NUM_SUPER_PIXELS]) {
    }
    void displayDynamicGrid(float vals[SuperPixelPatterns::NUM_SUPER_PIXELS]) {
    }
    void displayGrid(float vals[SuperPixelPatterns::NUM_SUPER_PIXELS]) {
    }
    void startDisplay(const uint8_t *font) {
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

  // This will take 15-20 seconds if the GridEye isn't connected.
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
      "Build:2024Sep02",
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

    void showGrid() {
      float vals[64];
      for (int i = 0; i < 64; i++) {
        vals[i] = gridEyeSupport.readOneSensor(i);
      }
      oledWrapper.displayDynamicGrid(vals);
    }

    void showReferenceGrid() {
      float vals[64];
      for (int i = 0; i < 64; i++) {
        vals[i] = (float)i;
      }
      oledWrapper.displayGrid(vals);
    }

    void showTestGrids() {
      const int NUM_TESTDATA = 2;
      float d1[] = {75,76,77,77,76,77,77,77,76,76,76,76,76,77,77,77,75,75,75,76,76,76,77,77,76,76,76,76,75,76,77,78,76,75,76,76,76,76,77,77,76,76,76,76,76,76,77,77,75,75,75,76,76,76,76,77,76,74,76,76,76,76,76,78};
      float d2[] = {80,81,80,81,84,96,98,90,81,82,82,83,86,135,133,94,83,83,83,85,87,98,107,91,87,90,90,87,90,100,95,90,95,134,144,101,113,146,132,95,98,137,131,105,123,137,128,95,90,98,104,92,98,116,100,92,86,89,86,86,87,90,89,88};
      float* testData[NUM_TESTDATA] = {
        d1, d2
      };
      String testDataNames[NUM_TESTDATA] = {
        "base", "3burners"
      };
      for (int i = 0; i < NUM_TESTDATA; i++) {
        oledWrapper.display(testDataNames[i]);
        delay(3000);
        oledWrapper.displayGrid(testData[i]);
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
          } else if (teststr.equals("refgrid")) {
            showReferenceGrid();
          } else if (teststr.equals("grid")) {
            showGrid();
          } else if (teststr.equals("temp")) {
            oledWrapper.drawInt(temperatureMonitor.getValue());
          } else if (teststr.equals("values")) {
            Utils::publish(gridEyeSupport.getValuesAsString());
          } else if (teststr.equals("test")) {
            showTestGrids();
          } else {
            String msg("Unknown command: ");
            msg.concat(teststr);
            msg.concat(", expected refgrid, grid, temp, values");
            Utils::publish(msg);
          }
          delay(5000);
          String msg("Command done: ");
          msg.concat(teststr);
          Utils::publish(msg);
         }
      }
    }

    void drawPixelTest(void) {
      uint16_t x, y, w2, h2;
      u8g2.setColor(1);
      w2 = u8g2.getWidth();
      h2 = u8g2.getHeight();
      for( y = 0; y < h2; y++ ) {
        for( x = 0; x < w2; x++ ) {
          u8g2.pixel(x,y);
        }
      }
    }

    void dpTest() {
      u8g2.erase();
      drawPixelTest();
      u8g2.display();
      int colorIndex = 0;
      for (uint16_t numRuns = 0; numRuns < 8; numRuns++) {
        u8g2.setColor(colorIndex);
        for (uint16_t y = 0; y < 16; y++ ) {
          for (uint16_t x = 0; x < 16; x++ ) {
              u8g2.pixel(x, y);
          }
        }
        u8g2.display();
        String s("colorIndex: ");
        s.concat(colorIndex);
        Utils::publish(s);
        colorIndex++;
        if (colorIndex > 2) {
          colorIndex = 0;
        }
        delay(10000);
      }
    }
    void display() {
#if SHOW_GRID
      showGrid();
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
      delay(1000);
      oledWrapper.startDisplay(u8g2_font_fur11_tf);
      uint16_t baseline = 16;
      oledWrapper.display(configs[2], 0, baseline);
      oledWrapper.endDisplay();
      delay(5000);
      oledWrapper.clear();
      Utils::publish("Finished setup...");
/*      Blurrer b;
      b.buildBlurKernel(2.0);
      b.diagnostics();
*/    }
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
