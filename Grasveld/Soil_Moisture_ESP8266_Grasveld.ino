#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

//Connect to BSSID
const char* ssid     = "Bossel_IoT";
const char* password = "@TelgenKamp17Rolde*";
WiFiClient client;

// Domoticz server info
const char* host = "192.168.1.251";
int port = 8080; //Domoticz port

// Domoticz device info - CHANGE THIS FOR DIFFERENT DEVICES
int idx = 6540; //IDX for this virtual sensor, found in Setup -> Devices
const char* dzvariable = "Bodemsensor%202";     // Uservariable for sensor
String dzvariableidx = "25";                    // IDX of Uservariable

// Initialization
bool calibration = false;
bool deepsleepmode = true;
const int powersoil = D8; // Digital Pin 8 will power the sensor, acting as switch
const long interval = 2000;
unsigned int humidity = 0;
int lowval = 90;          // Default setting, this is changed after connecting to domoticz and reading the values of the user variables
int highval = 410;        // Default setting, this is changed after connecting to domoticz and reading the values of the user variables
int maxval = 200;         // Maximum value of the sensor within Domoticz
int diffval;
int val;
const unsigned reading_count = 10; // Numbber of readings each time in order to stabilise
unsigned int analogVals[reading_count];
unsigned int counter = 0;
unsigned int values_avg = 0;
const unsigned sleepTimeS = 60; // Seconds to sleep between readings
unsigned sleepTimeMin = 30; // Minutes to sleep between readings, default value, will be overwritten by Domoticz
int HumStat = 0; // Status Variable for humidity- It can be Dry, Wet, Normal.
String url;
String localip;

void setup() {
  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  localip = WiFi.localIP().toString();
  long rssi = WiFi.RSSI();
  Serial.print("RSSI:");
  Serial.println(rssi);
  DzCalibration();
  pinMode(powersoil,OUTPUT);
  digitalWrite(powersoil, HIGH);
  delay(1000); 
  // Sending IP-adress to Domoticz variable
  // Example String: "/json.htm?type=command&param=updateuservariable&vname=USERVARIABLENAME&vtype=USERVARIABLETYPE&vvalue=USERVARIABLEVALUE";
      val = analogRead(A0);                                                     // Read value from sensor
      Serial.print("Value sent to domoticz: ");
      Serial.println(val);
      String ipurl = "";                                                       
      ipurl = "/json.htm?type=command&param=updateuservariable&vname=";
      ipurl += String(dzvariable);
      ipurl += "&vtype=2&vvalue={IDX:%20"; 
      ipurl += String(idx);
      ipurl += ";%20IP:%20";
      ipurl += String(localip);
      ipurl += ";%20RSSI:%20";
      ipurl += String(rssi);
      ipurl += ";%20MINval:%20";
      ipurl += String(lowval);
      ipurl += ";%20HIGHval:%20";
      ipurl += String(highval);
      ipurl += ";%20MEASval:%20";
      ipurl += String(val);
      ipurl += ";%20Sleeptime:%20";
      ipurl += String(sleepTimeMin);
      ipurl += "}";
  sendIPToDomoticz(ipurl);
  // Start-up delay
  delay(2000);
}

void setmoisture()
{
  if (calibration)
  {
    Serial.println("Entering calibration mode. Values are sent to uservariable in Domoticz.");
    Serial.println("Powering module ON.");
    digitalWrite(powersoil, HIGH);                                              //Powering the module on pin D8
    delay(1000);
    long rssi = WiFi.RSSI();
    //while (WiFi.status() == WL_CONNECTED) {
    while (calibration) {
      val = analogRead(A0);                                                     // Read value from sensor
      Serial.print("Value sent to domoticz: ");
      Serial.println(val);
      String ipurl = "";                                                       
      ipurl = "/json.htm?type=command&param=updateuservariable&vname=";
      ipurl += String(dzvariable);
      ipurl += "&vtype=2&vvalue={IDX:%20"; 
      ipurl += String(idx);
      ipurl += ";%20IP:%20";
      ipurl += String(localip);
      ipurl += ";%20RSSI:%20";
      ipurl += String(rssi);
      ipurl += ";%20MINval:%20";
      ipurl += String(lowval);
      ipurl += ";%20HIGHval:%20";
      ipurl += String(highval);
      ipurl += ";%20MEASval:%20";
      ipurl += String(val);
      ipurl += "}";
      HTTPClient http; 
      http.begin(client,host,port,ipurl);
      http.setAuthorization("api","api");
      int httpCode = http.GET();
      if (httpCode) 
      {
        if (httpCode != 200) 
        {
          Serial.print("HTTP response: ");
          Serial.println(httpCode);
        }
      }
      http.end();
      delay(2000);
      DzCalibration();
    }
  }
  else
  {
    Serial.print("Starting variables.");
    delay(500);
    Serial.print("..");
    delay(250);
    Serial.println("...");
    delay(250);
    values_avg = 0;                             //Set to default
    humidity = 0;                               //Set to default
    counter = 0;                                //Set to default
    diffval = highval - lowval;
    Serial.print("Low value: ");                
    Serial.println(lowval);                     //Serial print the default values as set at the beginning
    Serial.print("High value: ");
    Serial.println(highval);                    //Serial print the default values as set at the beginning
    Serial.print("Difference: ");
    Serial.println(diffval);                    //Serial print the default values as set at the beginning
    Serial.print("Max value for domoticz: ");
    Serial.println(maxval);                     //Serial print the default values as set at the beginning
    Serial.println("Powering module ON");
    digitalWrite(powersoil, HIGH);              //Powering the module on pin D8
    delay(1000);
    // read the input on analog pin 0:
    for( counter = 0; counter < reading_count; counter++)   //Reading values and create an average
    {
      analogVals[reading_count] = analogRead(A0) - lowval;
      delay(2000); // 2 seconds between each reading to get a better average value
      values_avg = (values_avg + analogVals[reading_count]);
    }
    values_avg = values_avg/reading_count;
    Serial.print("Average Readings value...:");
    Serial.println(values_avg);
    // make average value on a logarithmic scale
    humidity = (values_avg * values_avg) / (((highval - lowval) * (highval - lowval)) / maxval);
    // print out the value the sensor reads:
    Serial.print("Average humidity value...:");
    Serial.println(humidity);
    // Setting values between the minumum and maximum of Domoticz, else there is a false reading
    if humidity < 0
    {
      humidity = 0;
    }
    if humidity > maxval
    {
      humidity = maxval;
    }

    // Domoticz JSON string for updating moisture value
    String url = "/json.htm?type=command&param=udevice&idx=";
    url += String(idx);
    url += "&nvalue="; 
    url += String(humidity);
    if ((humidity >= 0) && (humidity <= maxval)) // only update Domoticz when the reading has a valid value
    {
      sendToDomoticz(url);
    }
  }
}

void sleepmode()
{
  if (deepsleepmode)
  {
    delay(interval);
    Serial.println("Powering module off");
    digitalWrite(powersoil, LOW);
    delay(interval);
    delay(5000);
    Serial.print("Entering deep-sleep mode for Xmin*60 sec..");
    delay(1000);
    Serial.print("...");
    delay(1000);
    Serial.println("...");
    delay(1000);
    Serial.print("ESP8266 in sleep mode for nr of minutes: ");
    Serial.println(sleepTimeMin);
    ESP.deepSleep(sleepTimeMin * sleepTimeS * 1000000);
  }
  else
  {
    Serial.println("Sleep mode disabled due to settings in Domoticz. Delaying for 60 seconds.");
    delay(60000);
    DzCalibration();
  }
}

void loop() {
  setmoisture();
}

void sendToDomoticz(String url) //Sending reading values to Domoticz
{
  Serial.print("Connecting to ");
  Serial.println(host);
  Serial.print("Requesting URL: ");
  Serial.println(url);
  HTTPClient http;
  http.begin(client,host,port,url);
  http.setAuthorization("api","api");
  int httpCode = http.GET();
    if (httpCode) 
    {
      if (httpCode == 200) 
      {
        String payload = http.getString();
        Serial.println("Domoticz response "); 
        Serial.println(payload);
        sleepmode();
      }
      else
      {
        Serial.print("HTTP response: ");
        Serial.println(httpCode);
      }
    }
  Serial.println("closing connection");
  http.end();
}

void sendIPToDomoticz(String ipurl) //Only when starting: sending IDX and IP-address to Domoticz
{
  Serial.print("Connecting to ");
  Serial.println(host);
  Serial.print("Requesting URL: ");
  Serial.println(host);
  Serial.println(port);
  Serial.println(ipurl);
  HTTPClient http;
  http.begin(client,host,port,ipurl);
  http.setAuthorization("api","api");
  int httpCode = http.GET();
    if (httpCode) 
    {
      if (httpCode == 200) 
      {
        String payload = http.getString();
        Serial.println("Domoticz response "); 
        Serial.println(payload);
      }
      else
      {
        Serial.print("HTTP response: ");
        Serial.println(httpCode);
      }
    }
  Serial.println("closing connection");
  http.end();
}

void DzCalibration() //Only when starting: request calibration status from Domoticz
{
  String calurl = "";                                                       
  calurl = "/json.htm?type=command&param=getuservariable&idx=";
  calurl += dzvariableidx;
  Serial.println("Checking for calibration status");
  HTTPClient http;
  http.begin(client,host,port,calurl);
  http.setAuthorization("api","api");
  int httpCode = http.GET();
  String r = http.getString();
  // Example uservariable domoticz value: 0;0;655;30; 
  int c = r.indexOf("Value");         // Start of value from HTTP request uservariables
  int l = r.indexOf(";",c + 1);       // Start of value for lowvalue setting
  int h = r.indexOf(";",l + 1);       // Start of value for highvalue setting
  int s = r.indexOf(";",h + 1);       // Start of value for sleeptime setting
  int e = r.indexOf(";",s + 1);       // End of uservariable values
  String cal = r.substring(c + 10, c + 11);
  String low = r.substring(l + 1, h);
  String high = r.substring(h + 1, s);
  String sleeptime = r.substring(s + 1, e);
  lowval = low.toInt();
  highval = high.toInt();
  sleepTimeMin = sleeptime.toInt();
  Serial.print("Values from Domoticz: ");
  Serial.print(lowval);
  Serial.print(" - ");
  Serial.print(highval);
  Serial.print(" - ");
  Serial.println(sleeptime);
  if (cal == "0")
  {
    calibration = false;
  }
  else
  {
    calibration = true;
    Serial.println("Domoticz status calibration true.");
  }
  if (sleepTimeMin == 0)
  {
    deepsleepmode = false;
  }
  else
  {
    deepsleepmode = true;
  }
  http.end();
  delay(5000);
}
