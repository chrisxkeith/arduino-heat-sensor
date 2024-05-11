// Please credit chris.keith@gmail.com .
// REM: Set board type.

#include <U8g2lib.h>

#include <bitset>
class SuperPixelPatterns {
  private:
    const static int NUM_SUPER_PIXELS = 64;
    const static int SUPER_PIXEL_SIZE = 64;
    const static int POSSIBLE_PIXEL_VALUES = SUPER_PIXEL_SIZE;
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
      uint16_t i = 0;
      for (std::bitset<SUPER_PIXEL_SIZE> superPixel : patterns) {
        for (uint16_t c = 0; c < i; c++) {
          // This should reduce flicker by only turning on one pixel when
          // going from, for example, intensity 5 to intensity 6.
          superPixel[shuffledIndices[c]] = true;
        }
        i++;
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
      SuperPixelPatterns superPixelPatterns;
  public:
    void u8g2_prepare(void) {
      u8g2.setFont(u8g2_font_fur49_tn);
      u8g2.setFontRefHeightExtendedText();
      u8g2.setDrawColor(1);
      u8g2.setFontDirection(0);
    }
    
    void drawEdge() {
      u8g2.drawLine(0, 0, 0, 95);
      u8g2.drawLine(0, 95, 127, 95);  
      u8g2.drawLine(127, 95, 127, 0);  
      u8g2.drawLine(127, 0, 0, 0);  
    }
    
    void drawInt(int val) {
      u8g2_prepare();
      u8g2.clearBuffer();
      u8g2.drawUTF8(2, this->baseLine, String(val).c_str());
      u8g2.setFont(u8g2_font_fur11_tf);
      u8g2.drawUTF8(6, this->baseLine + 20, "* Fahrenheit *");
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

    void displayGrid(float vals[64]) {
      float min = FLT_MAX;
      float max = -FLT_MAX;
      for (int i = 0; i < 64; i++) {
        if (vals[i] < min) {
          min = vals[i];
        }
        if (vals[i] > max) {
          max = vals[i];
        }        
      }
      clear();
      for (int i = 0; i < 64; i++) {
        int index = (int)round((vals[i] - min) / 64);
        // displaySuperPixelAt(i / 64, i % 64, index);
      }
      // oled->display();
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

class Utils {
  public:
    const static bool DO_SERIAL = true;
    static void publish(String s) {
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
    static String toString(bool b) {
      if (b) {
        return "true";
      }
      return "false";
    }
};

class TemperatureMonitor {
  public:
    int getValue() {
      gridEyeSupport.readValue();
      return gridEyeSupport.mostRecentValue;
    }
   
};
TemperatureMonitor temperatureMonitor;

const String githubRepo("https://github.com/chrisxkeith/arduino-heat-sensor");

void doDisplay() {
  oledWrapper.drawInt(temperatureMonitor.getValue());
  oledWrapper.drawEdge();
}

int lastSend = 0;
const int VALUES_SEND_INTERVAL = 5000;
void printValues() {
  int now = millis();
  if (now - lastSend > VALUES_SEND_INTERVAL) {
    Utils::publish(gridEyeSupport.getValuesAsString());
    lastSend = now;
  }
}

class App {
  private:
    int lastDisplay = 0;
    int lastShift = 0;

    void status() {
      Utils::publish(githubRepo);
    }

    void showGrid() {
      float vals[64];
      for (int i = 0; i < 64; i++) {
        vals[i] = gridEyeSupport.readOneSensor(i);
      }
      oledWrapper.displayGrid(vals);
      delay(10000);
    }

    void checkSerial() {
      if (Utils::DO_SERIAL) {
        int now = millis();
        while (Serial.available() == 0) {
          if (millis() - now > 500) {
            return;
          }
        }
        String teststr = Serial.readString();  //read until timeout
        teststr.trim();                        // remove any \r \n whitespace at the end of the String
        if (teststr.equals("?")) {
          status();
        } else if (teststr.equals("grid")) {
          showGrid();
        } else {
          String msg("Unknown command: ");
          msg.concat(teststr);
          Serial.println(msg);
        }
      }
    }
  public:
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
      doDisplay();
      Utils::publish("Finished setup...");	
    }
    void loop() {
      const int DISPLAY_RATE_IN_MS = 2000;
      int thisMS = millis();
      if (thisMS - lastDisplay > DISPLAY_RATE_IN_MS) {
        const int SHIFT_RATE = 1000 * 60 * 2; // Shift display every 2 minutes to avoid OLED burn-in.
        if (thisMS - lastShift > SHIFT_RATE) {
          oledWrapper.shiftDisplay(2);
          lastShift = thisMS;
        }
        doDisplay();
        lastDisplay = thisMS;
      }
      printValues();
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
