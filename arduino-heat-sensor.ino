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
    int shuffledIndices[SUPER_PIXEL_SIZE];

    void shuffle() {
      for (int i = 0; i < SUPER_PIXEL_SIZE; i++) {
        shuffledIndices[i] = i;
      }
      for (int i = 0; i < SUPER_PIXEL_SIZE; i++) {
        int rnd = rand() % SUPER_PIXEL_SIZE;
        int tmp = shuffledIndices[i];
        shuffledIndices[i] = shuffledIndices[rnd];
        shuffledIndices[rnd] = tmp;
      }
    }
  public:
    SuperPixelPatterns() {
      shuffle();
      for (uint16_t superPixelIndex = 0; superPixelIndex < NUM_SUPER_PIXELS; superPixelIndex++) {
        for (uint16_t subPixelIndex = 0; subPixelIndex < superPixelIndex; subPixelIndex++) {
          // This should reduce flicker by only turning on one pixel when
          // going from, for example, intensity 5 to intensity 6.
          patterns[superPixelIndex][shuffledIndices[subPixelIndex]] = true;
        }
      }
    }
    bool getPixelAt(int superPixelIndex, int pixelPosition) {
      return patterns[superPixelIndex][pixelPosition];
    }
};

#include <float.h>

U8G2_SSD1327_EA_W128128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

class OLEDWrapper {
  private:
      const int START_BASELINE = 50;
      int   baseLine = START_BASELINE;
  public:
    SuperPixelPatterns superPixelPatterns;
    void u8g2_prepare(void) {
      u8g2.setFont(u8g2_font_fur49_tn);
      u8g2.setFontRefHeightExtendedText();
      u8g2.setDrawColor(1);
      u8g2.setFontDirection(0);
    }
    
     void drawInt(int val) {
      u8g2_prepare();
      u8g2.clearBuffer();
      u8g2.drawUTF8(2, this->baseLine, String(val).c_str());
      u8g2.setFont(u8g2_font_fur11_tf);
      u8g2.drawUTF8(6, this->baseLine + 20, "Fahrenheit");
      u8g2.sendBuffer();
    }

    void clear() {
      u8g2.clearBuffer();
      u8g2.sendBuffer();
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
            u8g2.drawPixel(startX + xi, startY + yi);
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
      u8g2.clearBuffer();
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
      u8g2.sendBuffer();
    }

    void startDisplay(const uint8_t *font) {
      u8g2_prepare();
      u8g2.clearBuffer();
      u8g2.setFont(font);
    }
    void display(String s, uint8_t x, uint8_t y) {
      u8g2.setCursor(x, y);
      u8g2.print(s.c_str());
    }
    void endDisplay() {
      u8g2.sendBuffer();
    }
    void display(String s) {
      startDisplay(u8g2_font_fur11_tf);
      display(s, 0, 16);
      endDisplay();
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
    for(unsigned char i = 0; i < 8; i++) {
      ret.concat(readOneSensor(i));
      ret.concat(",");
    }
    ret.concat(",...");
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

const String configs[] = {
  "Built:",
  "Wed, May 15, 2024",
  "~10:11:26 AM",
  "arduino-heat-sensor",
};

class App {
  private:
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
      oledWrapper.displayGrid(vals);
    }

    void showReferenceGrid() {
      float vals[64];
      for (int i = 0; i < 64; i++) {
        vals[i] = (float)i;
      }
      oledWrapper.displayGrid(vals);
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
          } else {
            String msg("Unknown command: ");
            msg.concat(teststr);
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
      u8g2.setColorIndex(1);
      w2 = u8g2.getWidth();
      h2 = u8g2.getHeight();
      for( y = 0; y < h2; y++ ) {
        for( x = 0; x < w2; x++ ) {
          u8g2.drawPixel(x,y);
        }
      }
    }

    void dpTest() {
      u8g2.clearBuffer();
      drawPixelTest();
      u8g2.sendBuffer();
      int colorIndex = 0;
      for (uint16_t numRuns = 0; numRuns < 8; numRuns++) {
        u8g2.setColorIndex(colorIndex);
        for (uint16_t y = 0; y < 16; y++ ) {
          for (uint16_t x = 0; x < 16; x++ ) {
              u8g2.drawPixel(x, y);
          }
        }
        u8g2.sendBuffer();
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

  public:
#define SHOW_GRID false
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
      for (String s : configs) {
        oledWrapper.display(s, 0, baseline);
        baseline += 16;
      }
      oledWrapper.endDisplay();
      delay(5000);
#if SHOW_GRID
      showGrid();
#else
      oledWrapper.drawInt(temperatureMonitor.getValue());
#endif
      Utils::publish("Finished setup...");
    }
    void loop() {
//      dpTest(); // save for later.
#if SHOW_GRID
      showGrid();
#else
      const int DISPLAY_RATE_IN_MS = 2000;
      int thisMS = millis();
      if (thisMS - lastDisplay > DISPLAY_RATE_IN_MS) {
        const int SHIFT_RATE = 1000 * 60 * 2; // Shift display every 2 minutes to avoid OLED burn-in.
        if (thisMS - lastShift > SHIFT_RATE) {
          oledWrapper.shiftDisplay(2);
          lastShift = thisMS;
        }
        oledWrapper.drawInt(temperatureMonitor.getValue());
        lastDisplay = thisMS;
      }
#endif
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
