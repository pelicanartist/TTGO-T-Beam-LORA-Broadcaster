/*************************************
   ESP32 TT-GO T-Beam LoRa

   require the axp20x library that can be found
  https://github.com/lewisxhe/AXP202X_Library
  You must import it as gzip in sketch submenu
  in Arduino IDE
  This way, it is required to power up the GPS
  module, before trying to read it.

  Also get TinyGPS++ library from:
  https://github.com/mikalhart/TinyGPSPlus

  GPS Sample code from: https://github.com/LilyGO/TTGO-T-Beam/blob/master/GPS-T22_v1.0-20190612/GPS-T22_v1.0-20190612.ino

  There is a tendency to want to report more precise data than is supported by your GPS
  and perhaps more precision than is needed by your application.  Refer to the following
  table and compare with the abilities of your device and the requirements of your 
  application.  4 decimal places is probably the realistic limit of most arduino based
  GPS implementations, but do whatever you like.
  
   _GPS Precision Reference_
   
   decimal
   places  degrees      N/S or E/W     E/W at         E/W at       E/W at
                        at equator     lat=23N/S      lat=45N/S    lat=67N/S
   ------- -------      ----------     ----------     ---------    ---------
   0       1            111.32 km      102.47 km      78.71 km     43.496 km
   1       0.1          11.132 km      10.247 km      7.871 km     4.3496 km
   2       0.01         1.1132 km      1.0247 km      787.1 m      434.96 m
   3       0.001        111.32 m       102.47 m       78.71 m      43.496 m
   4       0.0001       11.132 m       10.247 m       7.871 m      4.3496 m
   5       0.00001      1.1132 m       1.0247 m       787.1 mm     434.96 mm
   6       0.000001     11.132 cm      102.47 mm      78.71 mm     43.496 mm
   7       0.0000001    1.1132 cm      10.247 mm      7.871 mm     4.3496 mm
   8       0.00000001   1.1132 mm      1.0247 mm      0.7871mm     0.43496mm
*/

// This gets used to parse the wifi mac to determine a device id
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

// Configure WiFi
#include <WiFi.h>

const char* ssid     = "";
const char* password = "";
bool wifi_connected = false;

// Use the WiFi MAC address to define a unique(?) system id
String wifimac = WiFi.macAddress();
String idpart1 = getValue(wifimac, ':', 4);
String idpart2 = getValue(wifimac, ':', 5);
String systemid = idpart1 + idpart2;


// Configure LoRa
#include <SPI.h>
#include <LoRa.h>

const int csPin = 18;          // LoRa radio chip select
const int resetPin = 23;       // LoRa radio reset
const int irqPin = 27;         // must be a hardware interrupt pin

int interval = 2000;          // interval between sends
long lastSendTime = 0;        // time of last packet send

// Configure Screen
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define OLED_SDA 21
#define OLED_SCL 22

Adafruit_SH1106 display(21, 22);



// Configure GPS
#include <TinyGPS++.h>
#include <axp20x.h>

TinyGPSPlus gps;
HardwareSerial GPS(1);
AXP20X_Class axp;

// Initialize array for tracking other nodes - node id, lat, long, time
const int maxarray = 5; // set the number of nodes to track - easier to define max than deal with parsing and formatting for variable length
String lastseen[maxarray][4] = {
  {"", "", ""}
};


// Configure Button
const int buttonPin = 38;    // the number of the pushbutton pin
int buttonState;             // the current reading from the input pin
int lastButtonState = HIGH;   // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

int menuScreen = 0;
int menuScreenMax = 3;


void setup() {
  Serial.begin(115200);
  Serial.println("~~~~~~~~~~~~~~~~~~~~~~~");
  Serial.println("Configuration begins");

  //Setup WIFI - move to run in background later?
  /*
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  */
  
  Serial.print("--> WiFi MAC: ");
  Serial.println(wifimac);
  Serial.println("--> This is System ID: " + systemid);

  //Setup the display
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  /*
  display.clearDisplay();  // clears the display
  // 'test', 128x64px
  const unsigned char splashscreenBitmap [] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x37, 0x80, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x08, 0x0d, 0x80, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x80, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x0d, 0xc1, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x06, 0x18, 0xa2, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x03, 0xaa, 0x2f, 0x7a, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x06, 0x15, 0x15, 0xd7, 0x10, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x05, 0x0b, 0xf6, 0xb0, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x05, 0x2b, 0x64, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x02, 0xdd, 0xbe, 0x04, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x02, 0xaa, 0xd6, 0x04, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x31, 0xb7, 0x7b, 0xb6, 0x00, 
    0x00, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x1a, 0xda, 0xad, 0x1e, 0x00, 
    0x01, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x8d, 0x55, 0xd6, 0x17, 0x00, 
    0x01, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0xf5, 0xae, 0xbb, 0x1b, 0x00, 
    0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x15, 0x6a, 0xd7, 0xa0, 0x40, 
    0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x05, 0x6a, 0xb5, 0x6d, 0x60, 0x00, 
    0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x0d, 0x55, 0xaf, 0xb7, 0xc0, 0x00, 
    0x00, 0x78, 0x07, 0xc0, 0x3c, 0x07, 0xc0, 0x3e, 0x06, 0x00, 0x15, 0x56, 0xa9, 0x5a, 0x80, 0x20, 
    0x00, 0x70, 0x1f, 0xe0, 0xff, 0x0f, 0xe0, 0x7f, 0x05, 0x00, 0x35, 0x55, 0x5d, 0xee, 0xc0, 0x00, 
    0x00, 0x70, 0x3f, 0xe1, 0xff, 0x1f, 0xf0, 0xff, 0x0e, 0x1d, 0x55, 0x55, 0x66, 0xb5, 0xc0, 0x20, 
    0x00, 0x70, 0x39, 0xe3, 0xcf, 0x3c, 0xe1, 0xcf, 0x0a, 0x15, 0x2a, 0xaa, 0xba, 0xaf, 0x00, 0x00, 
    0x00, 0xe0, 0x70, 0xe3, 0x8f, 0x38, 0xf3, 0xc7, 0x0b, 0x36, 0x55, 0x56, 0xcb, 0xda, 0x80, 0x10, 
    0x00, 0xe0, 0x01, 0xc7, 0x8e, 0x78, 0xe3, 0x80, 0x0d, 0x2b, 0x55, 0x5a, 0xb5, 0x6d, 0x80, 0x00, 
    0x00, 0xe0, 0x01, 0xc7, 0x00, 0x71, 0xe3, 0xc0, 0x0b, 0x35, 0x6b, 0x6a, 0xde, 0xb7, 0x80, 0x10, 
    0x01, 0xe0, 0x3f, 0xc7, 0x00, 0x71, 0xc3, 0xf0, 0x0d, 0x14, 0x2d, 0x55, 0x53, 0x5a, 0x80, 0x00, 
    0x01, 0xc0, 0x7f, 0x8f, 0x00, 0xe3, 0xc1, 0xfc, 0x0e, 0x87, 0x95, 0x56, 0xdd, 0xef, 0x80, 0x10, 
    0x01, 0xc1, 0xff, 0x8e, 0x00, 0xe3, 0x80, 0xfc, 0x0b, 0xc5, 0x6a, 0xb5, 0x6a, 0xb5, 0x40, 0x10, 
    0x03, 0xc1, 0xc3, 0x9e, 0x00, 0xe3, 0x80, 0x3c, 0x0d, 0x62, 0xa2, 0xd6, 0xad, 0xbb, 0x80, 0x80, 
    0x03, 0x83, 0xc7, 0x9e, 0x3d, 0xe3, 0x80, 0x1c, 0x0e, 0xb2, 0xdd, 0x5b, 0x76, 0xd6, 0x80, 0x10, 
    0x03, 0x83, 0x87, 0x1c, 0x39, 0xc7, 0x9e, 0x38, 0x0b, 0x59, 0x6b, 0x6a, 0xab, 0x7b, 0xe1, 0xc0, 
    0x07, 0x03, 0x8f, 0x1e, 0x79, 0xcf, 0x1e, 0x78, 0x0d, 0xeb, 0xad, 0xad, 0xdd, 0xae, 0xa3, 0x50, 
    0x07, 0x03, 0xfe, 0x1f, 0xf1, 0xfe, 0x1f, 0xf0, 0x06, 0xad, 0x56, 0xb6, 0xb6, 0xf5, 0xff, 0xc0, 
    0x07, 0x03, 0xfe, 0x1f, 0xe1, 0xfc, 0x0f, 0xe0, 0x0b, 0x75, 0x89, 0x5a, 0xdb, 0x5f, 0x5a, 0xc0, 
    0x0f, 0x01, 0xce, 0x0f, 0x80, 0xf8, 0x07, 0x80, 0x06, 0xda, 0x80, 0x2b, 0x6d, 0xeb, 0xef, 0x40, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0xaf, 0x00, 0x1d, 0xb6, 0xbd, 0x75, 0xc0, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf5, 0x80, 0x02, 0xdb, 0xd7, 0xbe, 0x80, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xad, 0x00, 0x01, 0x6d, 0x7a, 0xd7, 0x80, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xb6, 0x00, 0x00, 0xb7, 0xaf, 0x7a, 0x80, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x7a, 0x00, 0x00, 0xfa, 0xf5, 0xdf, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd7, 0x00, 0x00, 0x0f, 0xbf, 0x6a, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfb, 0x00, 0x00, 0x00, 0xd5, 0xdd, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x56, 0x80, 0x00, 0x00, 0x7e, 0xf4, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7d, 0xc0, 0x00, 0x00, 0xd7, 0x5a, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x40, 0x00, 0x01, 0xfa, 0xf0, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1d, 0xe0, 0x08, 0x93, 0x57, 0xa8, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x78, 0x00, 0x03, 0xfd, 0xd0, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xbc, 0x20, 0x02, 0xab, 0x40, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xd4, 0x00, 0x8f, 0xff, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x02, 0x3a, 0xaa, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x08, 0x6f, 0xf8, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x20, 0xf5, 0x40, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x0f, 0xbf, 0xa0, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xbb, 0x68, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  display.drawBitmap(0,0, splashscreenBitmap, 128, 64, 0);
  display.display();  // shows the default adafruit library logo if nothing else has been defined above
  delay(2000);
  */
  
  display.clearDisplay();  // clears the display
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("This is device #");
  display.println("");
  display.setTextSize(5);
  display.print(systemid);
  display.display();  // shows the default adafruit library logo if nothing else has been defined above
  delay(2000);
  display.clearDisplay();  // clears the display
  Serial.println("--> Local display active.");

  //Setup GPS
  Wire.begin(21, 22);
  if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS)) {
    Serial.println("AXP192 Begin PASS");
  } else {
    Serial.println("AXP192 Begin FAIL");
  }
  axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
  
  GPS.begin(9600, SERIAL_8N1, 34, 12);   //34-TX 12-RX
  Serial.println("--> GPS listening.");
  // more precision? https://sites.google.com/site/wayneholder/self-driving-rc-car/getting-the-most-from-gps
  //GPS.print("$PMTK313,1*2E\r\n");  // Enable to search a SBAS satellite
  //GPS.print("$PMTK301,2*2E\r\n");  // Enable WAAS as DGPS Source
  
  //Setup LoRa
  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin); // set CS, reset, IRQ pin

  if (!LoRa.begin(915E6)) {             // initialize ratio at 915 MHz (US)
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing - this code is from the LoRaSetSpread example and the conventions are silly
  }
  LoRa.setSpreadingFactor(7);           // ranges from 6-12,default 7 see API docs
  Serial.println("--> LoRa init succeeded.");

  //Setup Button
  pinMode(buttonPin, INPUT);

  //Serial debug info
  Serial.println("Configuration OK, starting... ");
  Serial.println("~~~~~~~~~~~~~~~~~~~~~~~");

}


// Broadcast the message 
void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
}

// Fix leading zeros in time elements
String format_time(int someval){
  // Add back leading 0 in time like 10:01:05 so it doesn't look like 10:1:5
  String retnum;
  if (someval<10){
    retnum = "0" +String(someval);
  }
  else retnum = someval;
  return retnum;
}

// Read current time from GPS - returns string in GMT
String gettime(){
  String currenttime;
  String hr;
  String mn;
  String sec;
  hr = format_time(gps.time.hour());
  mn = format_time(gps.time.minute());
  sec = format_time(gps.time.second());
  currenttime = hr +":" +mn +":" +sec;
  return currenttime;
}


// Write the LCD and serial output
void updatedisplay(int menuScreen) {
  String currenttime;
  String mylat;
  String mylng;
  String sats;
  String alt;
  int hdop_val;
  String position_rating = "";

  mylat = String(gps.location.lat(),4);
  mylng = String(gps.location.lng(),4);
  sats = gps.satellites.value();
  alt = gps.altitude.feet();
  hdop_val = gps.hdop.value();
  currenttime = gettime();
  
  if ((hdop_val > 2000) && (hdop_val < 9000)){
    position_rating = "Poor";
  }
  else if ((hdop_val < 2000) && (hdop_val > 500)){
    position_rating = "OK";
  }
  else if ((hdop_val < 500) && (hdop_val > 200)){
    position_rating = "Good";
  } 
  else if ((hdop_val < 200) && (hdop_val > 100)){
    position_rating = "Ex";
  }
  else if ((hdop_val < 100) && (hdop_val > 0)){
    position_rating = "Ideal";
  }
  else{
    position_rating = "No Fix";
  }
  position_rating = position_rating +" (" + (hdop_val / 100.0) +")";

  display.clearDisplay();  // Clear the buffer to prepare for the next screen draw
 
  // Default Display - GPS information
  if (menuScreen == 0){
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
  
    Serial.println("Latitude  : " +mylat);
    //display.println("Lat : " +mylat);
    display.println(mylat +" , " +mylng);
  
    Serial.println("Longitude : " +mylng);
    //display.println("Lon : " +mylng);
  
    Serial.println("Satellites: " +sats);
    //display.println("Sats: " +sats);
    display.println(position_rating +" " +sats +" sats");
  
    Serial.println("Altitude  : " +alt +" ft");
    display.println("Alt : " +alt +" ft");

    Serial.println("Time      : " +currenttime);
    display.println("Time: " +currenttime);
  
    Serial.println("Fix       : " +position_rating);
    //display.println("Fix : " +position_rating);
    
    //Serial.print("IP        : ");
    //Serial.println(WiFi.localIP());
    //display.print("IP  : ");
    //display.println(WiFi.localIP());
  }

  if (menuScreen == 1){
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    Serial.println("Recently seen peers");
    display.println("Recently seen peers");
    for (int i = 0; i < maxarray; i++){
     if (lastseen[i][0].length() > 1){
        String current_vector = getVector(lastseen[i][1].toDouble(), lastseen[i][2].toDouble());
        Serial.println(lastseen[i][0] +"      : " +current_vector);
        display.println(lastseen[i][0] +": " +current_vector);
     }
    }
  }

  if (menuScreen == 2){
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Altitude (ft) : ");
    display.println("");
    display.setTextSize(5);
    display.print(alt);
    
    Serial.println("Altitude  : " +alt +" ft");
  }

  // Show the Device ID on the main screen
  if (menuScreen == 3){
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("This is device #");
    display.println("");
    display.setTextSize(5);
    display.print(systemid);
    Serial.print("--> This is Device # ");
    Serial.println(systemid);
  }
  
  Serial.println("**********************");
  display.display();
}

// Populate the global lastseen array
void update_lastseen(String reporter, String rep_lat, String rep_lng) {
  String currenttime = gettime();
  for (int i = 0; i < maxarray; i++){
    // check if we've seen this system previously, update the record if found
    if (lastseen[i][0] == reporter){
      Serial.println(reporter +" last seen at " +lastseen[i][3] +" vector was " +getVector(lastseen[i][1].toDouble(), lastseen[i][2].toDouble()) +", new vector " +getVector(rep_lat.toDouble(), rep_lng.toDouble()));
      lastseen[i][1] = rep_lat;
      lastseen[i][2] = rep_lng;
      lastseen[i][3] = currenttime;
      i = maxarray + 1;  // prevent filling the array with the same data over and over
    }
    else{
      // if a blank line is found, add the system
      if (lastseen[i][0] == ""){
        Serial.println(reporter +" first contact at " +currenttime +" (" +rep_lat +"," +rep_lng +")");
        lastseen[i][0] = reporter;
        lastseen[i][1] = rep_lat;
        lastseen[i][2] = rep_lng;
        lastseen[i][3] = currenttime;
        i = maxarray + 1; // prevent filling the array with the same data over and over
      }
      else{
        Serial.println(reporter +" not in slot " +i);
      }     
    }
  }
}

// Listen for LoRa packets
void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  String incoming = "";
  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  Serial.println("#####");
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println("#####");

  // try to parse message
  String reporter;
  String rep_lat;
  String rep_lng;
  //String calc_vector;
  reporter = getValue(incoming, ',', 0);
  rep_lat  = getValue(incoming, ',', 1);
  rep_lng  = getValue(incoming, ',', 2);
  //calc_vector = getVector(rep_lat.toDouble(), rep_lng.toDouble());
  //Serial.println(reporter + calc_vector);
  if ((reporter.length() == 4) && (rep_lat.length() == 7)){
    update_lastseen(reporter, rep_lat, rep_lng);  
  }
  else{
    Serial.println("--> Rejecting malformed message: " +incoming);
  }

}


// Return distance and direction from current location to target
String getVector(double target_lat, double target_lng) {
  String return_string = "";
  if (String(gps.location.lat()) == "0.00") {
    return_string = " - unable to calculate vector, my position is unknown.";
  }
  else {
    double distance = gps.distanceBetween(gps.location.lat(), gps.location.lng(), target_lat, target_lng);  // returns distance in meters
    double courseto = gps.courseTo(gps.location.lat(), gps.location.lng(), target_lat, target_lng);
    String card = gps.cardinal(courseto);
    if (distance < 10) {
      return_string = " < 10m";
    }
    else {
      if (distance > 1000) {
        return_string = String((distance / 1000), 1) + "km " + card +" (" +String(courseto, 0) + ")"  ;
        //return_string = String(courseto, 0) + " (" + card + ") at " + String((distance / 1000), 1) + " km.";
      }
      return_string = String(distance, 0) + "m " + card +" (" +String(courseto, 0) + ")";
      //return_string = String(courseto, 0) + " (" + card + ") at " + String(distance, 0) + " m";
    }

  }
  return return_string;
}

// Return string value for encryption type in WiFi survey
//String translateEncryptionType(wifi_auth_mode_t encryptionType) {
String translateEncryptionType(int encryptionType) {
  switch (encryptionType) {
    case (WIFI_AUTH_OPEN):
      return "Open";
    case (WIFI_AUTH_WEP):
      return "WEP";
    case (WIFI_AUTH_WPA_PSK):
      return "WPA_PSK";
    case (WIFI_AUTH_WPA2_PSK):
      return "WPA2_PSK";
    case (WIFI_AUTH_WPA_WPA2_PSK):
      return "WPA_WPA2_PSK";
    case (WIFI_AUTH_WPA2_ENTERPRISE):
      return "WPA2_ENTERPRISE";
  }
}

// Scan WiFi
void scanNetworks() {
 
  int numberOfNetworks = WiFi.scanNetworks();
 
  Serial.print("Number of networks found: ");
  Serial.println(numberOfNetworks);

  if (numberOfNetworks > 0){
    Serial.println("SSID,RSSI,MAC,Encryption" );
  }
 
  for (int i = 0; i < numberOfNetworks; i++) {
 
    //Serial.print("Network name: ");
    Serial.print(WiFi.SSID(i));
 
    //Serial.print(", Signal strength: ");
    Serial.print(",");
    Serial.print(WiFi.RSSI(i));
 
    //Serial.print(", MAC address: ");
    Serial.print(",");
    Serial.print(WiFi.BSSIDstr(i));
 
    //Serial.print(", Encryption type: ");
    Serial.print(",");
    String encryptionTypeDescription = translateEncryptionType(WiFi.encryptionType(i));
    Serial.println(encryptionTypeDescription);
    
 
  }
  Serial.println("-----------------------");
}

long startInterval = millis();

// Main loop
void loop() {
  
  // Watch the button for changes
  // read the state of the switch into a local variable:
  int reading = digitalRead(buttonPin);
  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:
    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;
      //Serial.println("Button state change detected");

      // only toggle the button action if the new button state is LOW
      if (buttonState == LOW) {
        //Serial.println("Button is pressed");
        menuScreen++;
        if (menuScreen > menuScreenMax){
          menuScreen = 0;
        }
        Serial.print("Selected screen number  ");
        Serial.println(menuScreen);
        updatedisplay(menuScreen);
      }
    }
  }
  lastButtonState = reading;

  
  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());

  if (millis() - lastSendTime > interval) {
    updatedisplay(menuScreen);

    String message = systemid;      //prefix message

    if (String(gps.location.lat()) == "0.00") {
      Serial.println("No Fix - " + message  );
    }
    else {
      message = message += ",";
      String mylat = String(gps.location.lat(), 4);
      String mylong = String(gps.location.lng(), 4);
      message += mylat;      // add latitude, 4 decimal places ~ 11m precision
      message += ",";
      message += mylong;      // add longitude
      Serial.println("LoRa send: " + message);
      sendMessage(message);
    }
    lastSendTime = millis();            // timestamp the message
    interval = random(5000) + 5000;     // 5 - 10 seconds

    smartDelay(1000);

    //if (millis() > 5000 && gps.charsProcessed() < 10)
    //  Serial.println(F("No GPS data received: check wiring"));

    // Set up for the next pass
    display.clearDisplay();
    //if ((millis() - lastSendTime > interval) && (!wifi_connected)){
    //  scanNetworks();
    //}
  } // End main update loop
  

}

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (GPS.available())
      gps.encode(GPS.read());
  } while (millis() - start < ms);
}
