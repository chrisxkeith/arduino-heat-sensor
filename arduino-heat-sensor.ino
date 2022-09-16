// Please credit chris.keith@gmail.com .

#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1327_EA_W128128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

class OLEDWrapper {
  public:
    void u8g2_prepare(void) {
      u8g2.setFont(u8g2_font_inb63_mn);
      u8g2.setFontRefHeightExtendedText();
      u8g2.setDrawColor(1);
      u8g2.setFontPosTop();
      u8g2.setFontDirection(0);
    }

    void drawUTF8(String val) {
      u8g2.firstPage();
      do {
          u8g2_prepare();
          u8g2.setFont(u8g2_font_ncenB14_tr);
          u8g2.drawUTF8(0, 0, val.c_str());
      } while( u8g2.nextPage() );
    }

    void drawInt(int val) {
      u8g2.firstPage();
      do {
          u8g2_prepare();
          u8g2.drawUTF8(30, 10, String(val).c_str());
      } while( u8g2.nextPage() );
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
#include <Wire.h>
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

class Utils {
  public:
    static void publish(String s);
};

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

class OLEDDisplayer {

  public:
    void display() {
      Utils::publish(String(gridEyeSupport.mostRecentValue));

    }
};
OLEDDisplayer oledDisplayer;

int lastDisplay = 0;
const int DISPLAY_RATE_IN_MS = 150;
void display() {
    int thisMS = millis();
    if (thisMS - lastDisplay > DISPLAY_RATE_IN_MS) {
      oledDisplayer.display();
      lastDisplay = thisMS;
    }
}

const String githubHash("github hash: to be filled in after 'git push'");
const String githubRepo("https://github.com/chrisxkeith/arduino-heat-sensor");

void setup() {
  Serial.begin(57600);
  Utils::publish("Started setup...");
  Utils::publish(githubRepo);
  Utils::publish(githubHash);

  // Start your preferred I2C object
  Wire.begin();
  // Library assumes "Wire" for I2C but you can pass something else with begin() if you like

    gridEyeSupport.begin();
    int now = millis();
    gridEyeSupport.readValue();
    delay(5000);
    display();

  Utils::publish("Finished setup...");	
}

void loop() {
    gridEyeSupport.readValue();
    display();
    delay(2000);
}
