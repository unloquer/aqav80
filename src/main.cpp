// #define _TASK_TIMECRITICAL      // Enable monitoring scheduling overruns
#define _TASK_SLEEP_ON_IDLE_RUN // Enable 1 ms SLEEP_IDLE powerdowns between tasks if no callback methods were invoked during the pass
#define _TASK_STATUS_REQUEST    // Compile with support for StatusRequest functionality - triggering tasks on status change events in addition to time only
#define _TASK_WDT_IDS           // Compile with support for wdt control points and task ids
#define _TASK_LTS_POINTER       // Compile with support for local task storage pointer
#define _TASK_PRIORITY          // Support for layered scheduling priority
// #define _TASK_MICRO_RES         // Support for microsecond resolution
// #define _TASK_STD_FUNCTION      // Support for std::function (ESP8266 and ESP32 ONLY)
#define _TASK_DEBUG             // Make all methods and variables public for debug purposes
#define _TASK_INLINE       // Make all methods "inline" - needed to support some multi-tab, multi-file implementations
#define _TASK_TIMEOUT           // Support for overall task timeout

#define SENSOR_ID "v80_aprendiedo"

#define INTERNET
#define DHT_SENSOR
#define MIC
#define APP
#define GPS
#define MAP
#define LED
#define LED_CODE
#define DEBUGGING

#if(defined INTERNET || defined MAP)
#define FIXED_LAT "6.256984"
#define FIXED_LON "-75.578214"
#else
#define FIXED_LAT "0"
#define FIXED_LON "0"
#endif


#ifdef  DEBUGGING
#define DMSG(args...)       Serial.print(args)
#define DMSGf(args...)      Serial.printf(args)
#define DMSGln(args...)   Serial.println(args)
#else
#define DMSG(args...)
#define DMSGf(args...)
#define DMSGln(str)
#endif

#ifdef APP
#define DEBUGGING
#endif


#ifdef INTERNET 
#define INFLUXDB_HOST "aqa.unloquer.org"
// #define INFLUXDB_PORT "8086"
#define INFLUXDB_DATABASE "v80"
//if used with authentication
// #define INFLUXDB_USER "user"
// #define INFLUXDB_PASS "password"
#endif


using namespace std;
#include <Arduino.h>

#ifdef INTERNET
#include <ESP8266WiFi.h>
#include <InfluxDb.h>
#endif

#ifdef DHT_SENSOR
#include "DHT.h"
#endif

#include <SoftwareSerial.h>
#include <PMS.h>
#include <FastLED.h>
#include <vector>
#include <TaskScheduler.h>

#ifdef INTERNET
#define WIFI_SSID "PCES"
#define WIFI_PASS "42891185_ellen"
Influxdb influx(INFLUXDB_HOST);
#endif

vector<unsigned int> v1;      // for average
vector<unsigned int> v25;      // for average
vector<unsigned int> v10;      // for average
unsigned short int apm1 = 0;        // last PM10 average
unsigned short int apm25 = 0;        // last PM2.5 average
unsigned short int apm10 = 0;        // last PM10 average
vector<unsigned int> vmic;      // for average
unsigned short int amic = 0;        // last PM10 average
unsigned short int h = 0;
unsigned short int t = 0;

bool ledToggle = false;

// MIC
#ifdef MIC
const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;
#endif 

// PLANTOWER
#define P_TOWER_RX D2
#define P_TOWER_TX 6
SoftwareSerial plantower_serial(P_TOWER_RX, P_TOWER_TX);
PMS pms(plantower_serial);
PMS::DATA data;

// LED
#ifdef LED
#define LED_PIN D1
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS 64
CRGB leds[NUM_LEDS];
int BRIGHTNESS = 10; // this is half brightness
#endif
// DHT
#ifdef DHT_SENSOR
#define DHTPIN D3
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
#endif
/***
 * Average methods
 **/

void saveDataForAverage(unsigned short int pm25, unsigned short int pm10, unsigned short int pm1){
  v1.push_back(pm1);
  v25.push_back(pm25);
  v10.push_back(pm10);
  //try use delete for avoid memory leak
}
#ifdef MIC
void saveMicDataForAverage(unsigned int mic){
  vmic.push_back(mic);
  // Serial.print(","); Serial.print(mic);
  //try use only needed space in memory , int or short int  ?
}
#endif
unsigned short int getPM1Average(){
  unsigned short int pm1_average = accumulate( v1.begin(), v1.end(), 0.0)/v1.size();
  v1.clear();
  return pm1_average;
}
unsigned short int getPM25Average(){
  unsigned short int pm25_average = accumulate( v25.begin(), v25.end(), 0.0)/v25.size();
  v25.clear();
  return pm25_average;
}
unsigned short int getPM10Average(){
  unsigned short int pm10_average = accumulate( v10.begin(), v10.end(), 0.0)/v10.size();
  v10.clear();
  return pm10_average;
}
#ifdef MIC
unsigned short int getMicAverage(){
  unsigned short int mic_average = accumulate( vmic.begin(), vmic.end(), 0.0)/vmic.size();
  vmic.clear();
  return mic_average;
}
#endif
#ifdef DHT_SENSOR
unsigned short int getHumidity(){
  return dht.readHumidity();
}
unsigned short int getTemperature(){
  return dht.readTemperature();
}
#endif
void pmAverageLoop(){
  apm25 = getPM25Average();  // global var for display
  apm10 = getPM10Average();
  apm1 = getPM1Average();
}
#ifdef MIC
void micAverageLoop(){
  amic = getMicAverage();  // global var for display
}
#endif
#ifdef DHT_SENSOR
void readHTData(){
  h = getHumidity();
  // Read temperature as Celsius (the default)
  t = getTemperature();
  DMSG("Temperature ");DMSGln(t);
  DMSG("Humidity ");DMSGln(h);
}
#endif
Scheduler runner;

// CALLBACKS
void sendToInflux();
#ifdef DEBUGGING
void printMicAverage(){
  micAverageLoop();
  DMSG("Mic average ");
  DMSG(","); DMSG(amic);
}
void printPMAverage(){
  pmAverageLoop();
  DMSG("PM 1.0 (ug/m3): ");    DMSGln(apm1);
  DMSG("PM 2.5 (ug/m3): ");    DMSGln(apm25);
  DMSG("PM 10.0 (ug/m3): ");   DMSGln(apm10);
}
#endif
#ifdef APP
void printLEDColor(){
  if(apm25 < 12)DMSGln("verde");
  if(apm25 >= 12 && apm25 < 35)DMSGln("amarillo");
  if(apm25 >= 35 && apm25 < 55)DMSGln("naranja");
  if(apm25 >= 55 && apm25 < 150)DMSGln("rojo");
  if(apm25 >= 150 && apm25 < 255)DMSGln("morado");
  if(apm25 >= 255)DMSGln("cafe");
}
#endif
void readPlantowerData(){
  DMSGln("Leyendo PM ... ");
  if (pms.readUntil(data)) {
    saveDataForAverage(data.PM_AE_UG_2_5, data.PM_AE_UG_10_0, data.PM_AE_UG_1_0);
  }
  else DMSGln("No data.");
}
#ifdef MIC
void readMicData(){
  DMSG("Leyendo Mic ... ");
  unsigned long startMillis= millis();  // Start of sample window
  unsigned int peakToPeak = 0;   // peak-to-peak level

  unsigned int signalMax = 0;
  unsigned int signalMin = 1024;

  // collect data for 50 mS
  while (millis() - startMillis < sampleWindow)
    {
      sample = analogRead(0);
      DMSG(sample);
      if (sample > signalMax)
        {
          signalMax = sample;  // save just the max levels
        }
      else if (sample < signalMin)
        {
          signalMin = sample;  // save just the min levels
        }
    }
  peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
  saveMicDataForAverage(peakToPeak*0.07447 + 39.82947);
}
#endif
#ifdef LED
CRGB setColor(){
  CRGB alert = CRGB::Black;
  #ifdef LED_CODE
  if(apm25 < 12){
      int color=255*apm25/12;
      alert = CRGB(0,color,0);
   }
  if(apm25 >= 12 && apm25 < 35) {
      int color=255*apm25/35;
      alert = CRGB(255,color,0);
    }
  if(apm25 >= 35 && apm25 < 55) {
      int color=150*apm25/55;
      alert = CRGB(255,color,0);
    }
  if(apm25 >= 55 && apm25 < 75) {
      int color=255*apm25/75;
      alert = CRGB(color,0,0);
  }
  if(apm25 >= 75 && apm25 < 255)  {
      int color=180*apm25/255;
      alert = CRGB(175,0,color);
  }
  #else
  if(apm25 < 12) alert = CRGB::Green; // CRGB::Green; // Alert.ok
  if(apm25 >= 12 && apm25 < 35) alert = CRGB::Gold; // Alert.notGood;
  if(apm25 >= 35 && apm25 < 55) alert = CRGB::Tomato; // Alert.bad;
  if(apm25 >= 55 && apm25 < 150) alert = CRGB::DarkRed; // CRGB::Red; // Alert.dangerous;
  if(apm25 >= 150 && apm25 < 255) alert = CRGB::Purple; // CRGB::Purple; // Alert.VeryDangerous;
  #endif
  if(apm25 >= 255) alert = CRGB::Brown; // Alert.harmful;
  return alert;

}
void setLed(){
  ledToggle = !ledToggle;
    for(int i=0; i < 4; i++) {
    for(int j=0; j < NUM_LEDS; j++) leds[j] = ledToggle ? setColor() : CRGB::Black;
      FastLED.show();
  }
}
#endif
// TASKS
Task readPlantowerTask(400, TASK_FOREVER, &readPlantowerData);
#ifdef MIC
Task readMicTask(120, TASK_FOREVER, &readMicData);
#endif
#ifdef DHT_SENSOR
Task getDHTTask(15000, TASK_FOREVER, &readHTData);
#endif
#ifdef INTERNET
Task writeToInflux(15000, TASK_FOREVER, &sendToInflux);
#endif
#ifdef LED
Task ledBlink(1000, TASK_FOREVER, &setLed);
#endif
// CALLBACKS
#ifdef INTERNET
void sendToInflux(){
  pmAverageLoop();
  #ifdef MIC
  micAverageLoop();
  #endif
  #ifdef LED
  ledBlink.setInterval(map(amic, 0, 150, 1000, 400));
  #endif
  char row[256];

  sprintf(
          row,
          "%s,id=%s lat=%s,lng=%s,t=%u,h=%u,pm1=%u,pm25=%u,pm10=%u,s=%u",
          SENSOR_ID,SENSOR_ID,FIXED_LAT,FIXED_LON,t,h,apm1,apm25,apm10,amic
          );
  DMSGln(row);
  influx.write(row);
}


void connectToWifi(){
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  DMSGln("Connecting to WIFI"); 
 while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
#endif
void setup(){
  Serial.begin(115200);
  #ifdef INTERNET
  connectToWifi();
  influx.setDb(INFLUXDB_DATABASE);
  #endif
  pms.wakeUp();
  plantower_serial.begin(9600);
  #ifdef LED
  FastLED.setBrightness(BRIGHTNESS);
  LEDS.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  #endif
  #ifdef DHT_SENSOR
  dht.begin();
  #endif
  //printPMAverage();
  //printMicAverage();
  
  // setup time
  runner.init();
  //DMSGln("Initialized scheduler");
  runner.addTask(readPlantowerTask);
  //DMSGln("added tasks");
  readPlantowerTask.enable();
  #ifdef MIC
  runner.addTask(readMicTask);
  readMicTask.enable();
  #endif
  #ifdef INTERNET
  runner.addTask(writeToInflux);
  writeToInflux.enable();
  #endif
  #ifdef DHT_SENSOR
  runner.addTask(getDHTTask);
  getDHTTask.enable();
  #endif
  #ifdef LED
  runner.addTask(ledBlink);
  ledBlink.enable();
  #endif
}

void loop(){
  runner.execute();
  
}
