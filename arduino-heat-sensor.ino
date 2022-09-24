// Please credit chris.keith@gmail.com .

#include <U8g2lib.h>
#include <Wire.h>

class Utils {
  public:
    static void publish(String s);
};

// #define USE_FULL_BUFFER
#ifdef USE_FULL_BUFFER
  U8G2_SSD1327_EA_W128128_F_SW_I2C u8g2(U8G2_R0, 15, 4, U8X8_PIN_NONE);
#else
  U8G2_SSD1327_EA_W128128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
#endif

class OLEDWrapper {
  public:
    void u8g2_prepare(void) {
      u8g2.setFont(u8g2_font_fur49_tn);
      u8g2.setFontRefHeightExtendedText();
      u8g2.setDrawColor(1);
      u8g2.setFontDirection(0);
    }

    void drawUTF8_fullBuffer(String val) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_ncenB14_tr);
      u8g2.drawUTF8(0, 0, val.c_str());
      u8g2.sendBuffer();
    }

    void drawUTF8(String val) {
#ifdef USE_FULL_BUFFER
      drawUTF8_fullBuffer(val);
#else
      u8g2.firstPage();
      do {
          u8g2_prepare();
          u8g2.setFont(u8g2_font_ncenB14_tr);
          u8g2.drawUTF8(0, 0, val.c_str());
      } while( u8g2.nextPage() );
#endif
    }

    void drawEdge() {
      u8g2.drawLine(0, 0, 0, 95);
      u8g2.drawLine(0, 95, 127, 95);  
      u8g2.drawLine(127, 95, 127, 0);  
      u8g2.drawLine(127, 0, 0, 0);  
    }
    
    void drawInt(int val) {
#ifdef USE_FULL_BUFFER
      drawUTF8_fullBuffer(String(val).c_str());
#else
      u8g2.firstPage();
      do {
          u8g2_prepare();
          drawEdge();
          u8g2.drawUTF8(2, 90, String(val).c_str());
      } while( u8g2.nextPage() );
#endif
    }

    void clear() {
#ifdef USE_FULL_BUFFER
      u8g2.clearBuffer();
      u8g2.sendBuffer();
#else
      u8g2.firstPage();
      do {
      } while( u8g2.nextPage() );
#endif      
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

class GridEyeSupport {
private:
  GridEYE grideye;

public:
  int     mostRecentValue = INT_MIN;

  void begin() {
    grideye.begin();
  }

  // This will take 15-20 seconds if the GridEye isn't connected.
  int readValue() {
      float total = 0;
      for (int i = 0; i < 64; i++) {
        int t = (int)(grideye.getPixelTemperature(i) * 9.0 / 5.0 + 32.0);
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
      this->buzzOn();
      delay(nSeconds * 1000);
      this->buzzOff();
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
    
    void checkTimeAndTemp() {
      int   now = millis();
      int   currentTemp = gridEyeSupport.mostRecentValue;
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

const String githubRepo("https://github.com/chrisxkeith/arduino-heat-sensor");
const String githubHash("github commit: to come");

void doDisplay() {
  gridEyeSupport.readValue();
  Utils::publish(String(gridEyeSupport.mostRecentValue));
  oledWrapper.drawInt(gridEyeSupport.mostRecentValue);  
}

void setup() {
  Serial.begin(57600);
  Utils::publish("Started setup...");
  Utils::publish(githubRepo);
  Utils::publish(githubHash);

  Wire.begin();
  gridEyeSupport.begin();
  oledWrapper.setup_OLED();
  delay(3000);
  doDisplay();
  buzzer.buzzForSeconds(2);
  Utils::publish("Finished setup...");	
}


int lastDisplay = 0;
void loop() {
  temperatureMonitor.checkTimeAndTemp();
  const int DISPLAY_RATE_IN_MS = 2000;
  int thisMS = millis();
  if (thisMS - lastDisplay > DISPLAY_RATE_IN_MS) {
    doDisplay();
    lastDisplay = thisMS;
  } else if (temperatureMonitor.whenCrossedThreshold > 0) {
    oledWrapper.clear();
    delay(1000);
    doDisplay();
  }
}
