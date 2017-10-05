// simplestesp8266clock.ino
//
// Libraries needed:
//  Time.h & TimeLib.h:  https://github.com/PaulStoffregen/Time
//  Timezone.h: https://github.com/JChristensen/Timezone
//  SSD1306.h & SSD1306Wire.h:  https://github.com/squix78/esp8266-oled-ssd1306
//  NTPClient.h: https://github.com/arduino-libraries/NTPClient
//  ESP8266WiFi.h & WifiUDP.h: https://github.com/ekstrand/ESP8266wifi
//
// 128x64 OLED pinout:
// GND goes to ground
// Vin goes to 3.3V
// Data to I2C SDA (GPIO 0)
// Clk to I2C SCL (GPIO 2)
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <string.h>
#include <Wire.h>
#include <SSD1306.h>
#include <SSD1306Wire.h>
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include <Timezone.h>

#include "OLEDDisplayUi.h"

// Define NTP properties
#define NTP_OFFSET   60 * 60      // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "0.id.pool.ntp.org"  // change this to whatever pool is closest (see ntp.org)

// Set up the NTP UDP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

// Create a display object
SSD1306  display(0x3c,D1, D2); //0x3d for the Adafruit 1.3" OLED, 0x3C being the usual address of the OLED
 
const char* ssid = "mnmkn";   // insert your own ssid 
const char* password = "user.100";              // and password
String date;
String t;
const char * days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"} ;
const char * months[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sep", "Oct", "Nov", "Dec"} ;
const char * ampm[] = {"AM", "PM"} ;

String suhu; 
String lembap ;

//tem and thingspeak init
String apiKey = "GVYYZ8JOK5X4ZY73";
const char* server = "api.thingspeak.com";
#define DHTPIN D3 // what pin we're connected to
DHT dht(DHTPIN, DHT11,15);
WiFiClient client;

void setup () 
{
  Serial.begin(115200); // most ESP-01's use 115200 but this could vary
  timeClient.begin();   // Start the NTP UDP client

  Wire.pins(D1,D2);// Start the OLED with GPIO 0 and 2 on ESP-01
  Wire.begin(D1,D2);
  display.init();
//  display.flipScreenVertically();   

  // Connect to wifi
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.print(ssid);
  display.drawString(0, 10, "Connecting to Wifi...");
  display.display();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi at ");
  Serial.print(WiFi.localIP());
  Serial.println("");
  display.drawString(0, 24, "Connected.");
  display.display();
  delay(1000);
}

void loop() 
{
  if (WiFi.status() == WL_CONNECTED) //Check WiFi connection status
  { 

  // dht init and thingspeak

  float hum = dht.readHumidity() ;
  float temp = dht.readTemperature();
  if (isnan(hum) || isnan(temp)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  if (client.connect(server,80)) {  //   "184.106.153.149" or api.thingspeak.com
    String postStr = apiKey;
           postStr +="&field1=";
           postStr += String(temp);
           postStr +="&field2=";
           postStr += String(hum);
           postStr += "\r\n\r\n";

     client.print("POST /update HTTP/1.1\n");
     client.print("Host: api.thingspeak.com\n");
     client.print("Connection: close\n");
     client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
     client.print("Content-Type: application/x-www-form-urlencoded\n");
     client.print("Content-Length: ");
     client.print(postStr.length());
     client.print("\n\n");
     client.print(postStr);


     Serial.print("Temperature: ");
     Serial.print(temp);
     Serial.print(" degrees Celcius Humidity: ");
     Serial.print(hum);
     Serial.println("% send to Thingspeak");
  }
  client.stop();

  Serial.println("Waiting...");
  // thingspeak needs minimum 15 sec delay between updates
//  delay(60000);

  //end thingspeak 
      


      
    date = "";  // clear the variables
    t = "";
    
    // update the NTP client and get the UNIX UTC timestamp 
    timeClient.update();
    unsigned long epochTime =  timeClient.getEpochTime();

    // convert received time stamp to time_t object
    time_t local, utc;
    utc = epochTime;

    // Then convert the UTC UNIX timestamp to local time
    TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, +360}; 
    TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, +420};
    Timezone usEastern(usEDT, usEST);
    local = usEastern.toLocal(utc);

    // now format the Time variables into strings with proper names for month, day etc
    date += days[weekday(local)-1];
    date += ", ";
    date += months[month(local)-1];
    date += " ";
    date += day(local);
    date += ", ";
    date += year(local);

    // format the time to 12-hour format with AM/PM and no seconds
    t += hourFormat12(local);
    t += ":";
    if(minute(local) < 10)  // add a zero if minute is under 10
      t += "0";
    t += minute(local);
    t += " ";
    t += ampm[isPM(local)];

    // Display the date and time
    Serial.println("");
    Serial.print("Local date: ");
    Serial.print(date);
    Serial.println("");
    Serial.print("Local time: ");
    Serial.print(t);
//    delay(3000);
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawStringMaxWidth(64, 8, 128, "Susiloharjo");
    display.setFont(ArialMT_Plain_16);
    display.drawStringMaxWidth(64, 33, 128, "Clock");
    display.display();
    // print the date and time on the OLED
    delay(2000);

     //begin show temp
    
    suhu = String(temp);
    lembap = String(hum);
    
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_10);
    display.drawStringMaxWidth(64, 0, 128, "Temperature");
    display.setFont(ArialMT_Plain_16);
    display.drawStringMaxWidth(64, 15, 128, suhu + " C");
  
    
    display.setFont(ArialMT_Plain_10);
    display.drawStringMaxWidth(64, 33, 128, "Humidity");
    display.setFont(ArialMT_Plain_16);
    display.drawStringMaxWidth(64, 45, 128, lembap + "%");
    
    display.display();
    // print the date and time on the OLED
    delay(5000);
    
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawStringMaxWidth(64, 10, 128, t);
    display.setFont(ArialMT_Plain_10);
    display.drawStringMaxWidth(64, 38, 128, date);
    display.display();
    delay(5000);
    
   

    
  }
  else // attempt to connect to wifi again if disconnected
  {
    display.clear();
    display.drawString(0, 10, "Connecting to Wifi...");
    display.display();
    WiFi.begin(ssid, password);
    display.drawString(0, 24, "Connected.");
    display.display();
    delay(1000);
  }
    
  delay(30000);    //Send a request to update every 10 sec (= 10,000 ms)
}
