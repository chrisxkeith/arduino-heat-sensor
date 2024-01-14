// Please credit chris.keith@gmail.com .

#include <U8g2lib.h>
#include <Wire.h>

#define USE_THERMISTOR false

class Utils {
  public:
    const static bool DO_SERIAL = false;
    static void publish(String s);
    static String toString(bool b) {
      if (b) {
        return "true";
      }
      return "false";
    }
};

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
OLEDWrapper oledWrapper;

#include <SparkFun_GridEYE_Arduino_Library.h>
#include <limits.h>

class Sensor {
  private:
    int     lastVal;
  protected:
    int     pin;
    String  name;
    double  factor; // apply to get human-readable values, e.g., degrees F

  public:
    Sensor(int pin, String name, double factor) {
        this->pin = pin;
        this->name = name;
        this->factor = factor;
        this->lastVal = INT_MIN;
        pinMode(pin, INPUT);
    }
    
    String getName() { return name; }

    void sample() {
        if (pin >= A0 && pin <= A5) {
            lastVal = analogRead(pin);
        } else {
            lastVal = digitalRead(pin);
        }
    }
    
    int applyFactor(int val) {
        return val * factor;
    }

    int getValue() {
        return applyFactor(lastVal);
    }
};

Sensor thermistorSensor(A0, "Stove", 0.036);

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
    for(unsigned char i = 0; i < 64; i++) {
      ret.concat(readOneSensor(i));
      ret.concat(",");
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

class Buzzer {
  private:
    const int PIEZO_PIN = 6;
    const bool ENABLED = false;
  public:
    Buzzer() {
        pinMode(PIEZO_PIN, OUTPUT);
    }
    void buzzOn() {
      for (int hz = 440; hz < 1000; hz += 25) {
        tone(PIEZO_PIN, hz, 50);
        delay(5);
        for (int i = 3; i <= 7; i++);
      }    
    }
    void buzzOff() {
        noTone(PIEZO_PIN);
    }
    void buzzForSeconds(int nSeconds) {
      if (ENABLED) {
        this->buzzOn();
        delay(nSeconds * 1000);
        this->buzzOff();
      }
    }
};
Buzzer buzzer;

// #define TEST_DATA
class TemperatureMonitor {
  private:
#ifdef TEST_DATA
    const int THRESHOLD = 50;               // farenheit
#else
    const int THRESHOLD = 100;              // hit threshold immediately for testing
#endif
    int       lastBuzzTime = 0;             // milliseconds
  public:
    int       whenCrossedThreshold = 0;     // milliseconds

    int getValue() {
#if USE_THERMISTOR
      return thermistorSensor.getValue();
#else
      gridEyeSupport.readValue();
      return gridEyeSupport.mostRecentValue;
#endif
    }
    
    void checkTimeAndTemp() {
      int   now = millis();
      int   currentTemp = this->getValue();
      if (currentTemp < this->THRESHOLD) {
        this->whenCrossedThreshold = 0;
      } else {
        if (this->whenCrossedThreshold == 0) {
          this->whenCrossedThreshold = now;
        } else {
#ifdef TEST_DATA
          int hoursOverThreshold = ((now - this->whenCrossedThreshold) / 1000); // use seconds for testing
#else
          int hoursOverThreshold = (((now - this->whenCrossedThreshold) / 1000) / 60) / 60;
#endif
          int silentInterval = 0; // seconds, zero is flag for don't buzz.
          switch (hoursOverThreshold) {
            case 0:  silentInterval = 0;      break;
            case 1:  silentInterval = 0;      break;
            case 2:  silentInterval = 5 * 60; break;
            case 3:  silentInterval = 60;     break;
            default: silentInterval = 15;     break;
          }
          if (silentInterval > 0) {
#ifdef TEST_DATA
            silentInterval = max(silentInterval / 60, 5); // speed up testing
#endif
            if (this->lastBuzzTime + (silentInterval * 1000) < now) {
              buzzer.buzzForSeconds(2);
              this->lastBuzzTime = millis();
              Utils::publish("Buzzed at...");
            }
          }
        }
      }
    }
};
TemperatureMonitor temperatureMonitor;

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
      String v("USE_THERMISTOR: ");
      v.concat(Utils::toString(USE_THERMISTOR));
      delay(1000);
      Utils::publish(v);
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

      Wire.begin();
      gridEyeSupport.begin();
      oledWrapper.setup_OLED();
      delay(1000);
      doDisplay();
      Utils::publish("Finished setup...");	
    }
    void loop() {
      temperatureMonitor.checkTimeAndTemp();
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
