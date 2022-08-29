# 1 "c:\\Users\\chris\\Documents\\Github\\arduino-heat-sensor\\arduino-heat-sensor.ino"
// Please credit chris.keith@gmail.com .

const String githubHash("_to be filled in after 'git push'");
const String githubRepo("https://github.com/chrisxkeith/arduino-heat-sensor");

void to_serial(String s) {
  char buf[12];
  sprintf(buf, "%10u", millis());
  String s1(buf);
  s1.concat(" ");
  s1.concat(s);
  Serial.println(s1);
}

void setup() {
  Serial.begin(57600);
  to_serial("Started setup...");
  to_serial(githubRepo);
  to_serial(githubHash);
  to_serial("Finished setup...");
}

void loop() {

}
