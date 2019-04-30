#include <TheThingsNetwork.h>
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

const char *appEui = "";
const char *appKey = "";

volatile bool TTN_BUTTON_PRESSED = false;
char *filename = "HOME.MP3";

int count = 0;

#define loraSerial Serial1
#define debugSerial Serial
#define TTN_VBAT_MEAS_EN A2
#define TTN_VBAT_MEAS 1
#define SETUP 1
#define BUTTON 2
#define INTERVAL 3
#define AWAY 4
#define HOME 5
#define CLK 13       // SPI Clock, shared with SD card
#define MISO 12      // Input data, from VS1053/SD card
#define MOSI 11      // Output data, to VS1053/SD card
#define BREAKOUT_RESET  9      // VS1053 reset pin (output)
#define BREAKOUT_CS     10     // VS1053 chip select pin (output)
#define BREAKOUT_DCS    8      // VS1053 Data/command select pin (output)
#define SHIELD_RESET  -1      // VS1053 reset pin (unused!)
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)
#define CARDCS 4     // Card chip select pin
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin
#define freqPlan TTN_FP_EU868

TheThingsNetwork ttn(loraSerial, debugSerial, freqPlan);

Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

void setup() {
  loraSerial.begin(57600);
  debugSerial.begin(9600);

  // Wait a maximum of 10s for Serial Monitor
  while (!debugSerial && millis() < 10000);

  // Set callback for incoming messages
  ttn.onMessage(message);

  debugSerial.println("-- TTN: STATUS");
  ttn.showStatus();

  debugSerial.println("-- TTN: JOIN");
  ttn.join(appEui, appKey);

  debugSerial.println("Adafruit VS1053 Simple Test");

  if (! musicPlayer.begin()) { // initialise the music player
     debugSerial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }
  debugSerial.println(F("VS1053 found"));

  musicPlayer.GPIO_pinMode(BUTTON, OUTPUT);

  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }

  musicPlayer.setVolume(0,0);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);
  musicPlayer.startPlayingFile("/READY.MP3");
}

void loop() {
  count++;
  delay(100);
  
  if ((musicPlayer.GPIO_digitalRead(BUTTON)) == HIGH && (TTN_BUTTON_PRESSED == false)) {    
        TTN_BUTTON_PRESSED = true;         
  }
  
  if ((musicPlayer.GPIO_digitalRead(BUTTON)) == LOW && (TTN_BUTTON_PRESSED == true)) {    
        TTN_BUTTON_PRESSED = false;         
  }
  
  if (TTN_BUTTON_PRESSED == true) {
    debugSerial.println("-- BUTTON: PRESSED ");
    debugSerial.print("-- SEND: BUTTON");
  
    //musicPlayer.startPlayingFile("/HOME.MP3");
    musicPlayer.startPlayingFile(filename);
  
    sendData(BUTTON);
    delay(6000);   
  }

  if (count > (10 * 14400)) {
    count = 0;
    debugSerial.println("-- INTERVAL");
    ttn.poll();
    sendData(INTERVAL);
  }
}

void wake() {
 debugSerial.println(F("-- WAKE"));
}

void sleep() {
}

// Send LORA data
void sendData(uint8_t port) {
  // Wake RN2483
  //ttn.wake();

  ttn.showStatus();

  byte *bytes;
  byte payload[2];

  uint16_t battery = getBattery();
  bytes = (byte *)&battery;
  payload[0] = bytes[1];
  payload[1] = bytes[0];
 
  ttn.sendBytes(payload, sizeof(payload), port);

  // Set RN2483 to sleep mode
  //ttn.sleep(36600000);

  // This one is not optionnal, remove it
  // and say bye bye to RN2983 sleep mode
  //delay(50);
}

// Receive Lora Hook
void message(const uint8_t *payload, size_t size, port_t port) {
  debugSerial.println("-- MESSAGE");
  
  if (filename == "HOME.MP3") {
    sendData(AWAY);
    filename = "AWAY.MP3";
  } else {
    sendData(HOME);
    filename = "HOME.MP3";
  }
}

// File listing helper
void printDirectory(File dir, int numTabs) {
   while(true) {
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}

// get Battery Information
uint16_t getBattery() {
  digitalWrite(TTN_VBAT_MEAS_EN, LOW);
  uint16_t val = analogRead(TTN_VBAT_MEAS);
  digitalWrite(TTN_VBAT_MEAS_EN, HIGH);
  uint16_t batteryVoltage = map(val, 0, 1024, 0, 3300) * 2; // *2 for voltage divider
  return batteryVoltage;
}
