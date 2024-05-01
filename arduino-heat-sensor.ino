// Please credit chris.keith@gmail.com .

#define USE_OLED false
#if USE_OLED
#include <U8g2lib.h>

U8G2_SSD1327_EA_W128128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

class OLEDWrapper {
  private:
      const int START_BASELINE = 50;
      int   baseLine = START_BASELINE;
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
      u8g2.firstPage();
      do {
          u8g2_prepare();
          u8g2.drawUTF8(2, this->baseLine, String(val).c_str());
          u8g2.setFont(u8g2_font_fur11_tf);
          u8g2.drawUTF8(6, this->baseLine + 20, "Fahrenheit");
      } while( u8g2.nextPage() );
    }

    void clear() {
      u8g2.firstPage();
      do {
      } while( u8g2.nextPage() );      
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
};
#else
class OLEDWrapper {
  public:
    void u8g2_prepare(void) {}    
    void drawEdge() {}    
    void drawInt(int val) {}
    void clear() {}
    void shiftDisplay(int shiftAmount) {}
    void setup_OLED() {}
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

  String getValues() {
    String ret;
    for(unsigned char i = 0; i < 8; i++) {
      ret.concat(grideye.getPixelTemperature(i));
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
    Utils::publish(gridEyeSupport.getValues());
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
        if (teststr == "?") {
          status();
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
        Serial.begin(57600);
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
