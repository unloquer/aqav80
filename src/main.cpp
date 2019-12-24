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

#define FIXED_LAT "6.200022"
#define FIXED_LON "-75.505357"
#define SENSOR_ID "aqa_id"

// #define DEBUGGING
#ifdef  DEBUGGING
#define DMSG(args...)       Serial.print(args)
#define DMSGf(args...)      Serial.printf(args)
#define DMSGln(args...)   Serial.println(args)
#else
#define DMSG(args...)
#define DMSGf(args...)
#define DMSGln(str)
#endif

#define INFLUXDB_HOST "aqa.unloquer.org"
// #define INFLUXDB_PORT "8086"
#define INFLUXDB_DATABASE "v80"
//if used with authentication
// #define INFLUXDB_USER "user"
// #define INFLUXDB_PASS "password"

using namespace std;
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <InfluxDb.h>
#include <SoftwareSerial.h>
#include <PMS.h>
#include <FastLED.h>
#include <vector>
#include "DHT.h"
#include <TaskScheduler.h>

#define WIFI_SSID "HOME-4E05"
#define WIFI_PASS "BDFBC14F7BC4E656"

Influxdb influx(INFLUXDB_HOST);

vector<unsigned int> v1;      // for average
vector<unsigned int> v25;      // for average
vector<unsigned int> v10;      // for average
unsigned int apm1 = 0;        // last PM10 average
unsigned int apm25 = 0;        // last PM2.5 average
unsigned int apm10 = 0;        // last PM10 average
vector<unsigned int> vmic;      // for average
unsigned int amic = 0;        // last PM10 average
unsigned int h = 0;
unsigned int t = 0;

// MIC
const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;

// PLANTOWER
#define P_TOWER_RX D2
#define P_TOWER_TX 6
SoftwareSerial plantower_serial(P_TOWER_RX, P_TOWER_TX);
PMS pms(plantower_serial);
PMS::DATA data;

// LED
#define LED_PIN D6
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS 1
CRGB leds[NUM_LEDS];
int BRIGHTNESS = 20; // this is half brightness

// DHT
#define DHTPIN D3
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
/***
 * Average methods
 **/

void saveDataForAverage(unsigned int pm25, unsigned int pm10, unsigned int pm1){
  v1.push_back(pm1);
  v25.push_back(pm25);
  v10.push_back(pm10);
}
void saveMicDataForAverage(unsigned int mic){
  vmic.push_back(mic);
}
unsigned int getPM1Average(){
  unsigned int pm1_average = accumulate( v1.begin(), v1.end(), 0.0)/v1.size();
  v1.clear();
  return pm1_average;
}
unsigned int getPM25Average(){
  unsigned int pm25_average = accumulate( v25.begin(), v25.end(), 0.0)/v25.size();
  v25.clear();
  return pm25_average;
}
unsigned int getPM10Average(){
  unsigned int pm10_average = accumulate( v10.begin(), v10.end(), 0.0)/v10.size();
  v10.clear();
  return pm10_average;
}
unsigned int getMicAverage(){
  unsigned int mic_average = accumulate( vmic.begin(), vmic.end(), 0.0)/vmic.size();
  vmic.clear();
  return mic_average;
}
unsigned int getHumidity(){
  return dht.readHumidity();
}
unsigned int getTemperature(){
  return dht.readTemperature();
}
void pmAverageLoop(){
  apm25 = getPM25Average();  // global var for display
  apm10 = getPM10Average();
  apm1 = getPM1Average();
}
void micAverageLoop(){
  amic = getMicAverage();  // global var for display
}
void readHTData(){
  h = getHumidity();
  // Read temperature as Celsius (the default)
  t = getTemperature();
  DMSG("Temperature ");DMSGln(t);
  DMSG("Humidity ");DMSGln(h);
}

Scheduler runner;

// CALLBACKS
void printMicAverage() {
  micAverageLoop();
  DMSG("Mic average ");
  DMSG(","); DMSG(amic);
}
void printPMAverage() {
  pmAverageLoop();
  DMSG("PM 1.0 (ug/m3): ");    DMSGln(apm1);
  DMSG("PM 2.5 (ug/m3): ");    DMSGln(apm25);
  DMSG("PM 10.0 (ug/m3): ");   DMSGln(apm10);
}
void readPlantowerData() {
  DMSGln("Leyendo PM ... ");
  if (pms.readUntil(data)) {
    saveDataForAverage(data.PM_AE_UG_2_5, data.PM_AE_UG_10_0, data.PM_AE_UG_1_0);
  }
  else DMSGln("No data.");
}
void readMicData() {
  DMSG("Leyendo Mic ... ");
  unsigned long startMillis= millis();  // Start of sample window
  unsigned int peakToPeak = 0;   // peak-to-peak level

  unsigned int signalMax = 0;
  unsigned int signalMin = 1024;

  // collect data for 50 mS
  while (millis() - startMillis < sampleWindow)
    {
      sample = analogRead(0);
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
  saveMicDataForAverage(peakToPeak);
}
void sendToInflux() {
  micAverageLoop();
  pmAverageLoop();
  char row[256];

  sprintf(
          row,
          "%s,id=%s lat=%s,lng=%s,t=%u,h=%u,pm1=%u,pm25=%u,pm10=%u,s=%u",
          SENSOR_ID,SENSOR_ID,FIXED_LAT,FIXED_LON,t,h,apm1,apm25,apm10,amic
          );
  Serial.println(row);

  influx.write(row);
}
// TASKS
Task readPlantowerTask(400, TASK_FOREVER, &readPlantowerData);
Task readMicTask(120, TASK_FOREVER, &readMicData);
Task getHTTask(15000, TASK_FOREVER, &readHTData);
Task writeToInflux(15000, TASK_FOREVER, &sendToInflux);

void setup(){
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  DMSGln("Connecting to WIFI");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  influx.setDb(INFLUXDB_DATABASE);
  LEDS.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  plantower_serial.begin(9600);
  pms.wakeUp();
  FastLED.setBrightness(BRIGHTNESS);
  dht.begin();

  // setup time
  runner.init();
  DMSGln("Initialized scheduler");
  runner.addTask(readPlantowerTask);
  runner.addTask(readMicTask);
  runner.addTask(getHTTask);
  runner.addTask(writeToInflux);
  DMSGln("added tasks");
  readPlantowerTask.enable();
  readMicTask.enable();
  getHTTask.enable();
  writeToInflux.enable();
}

void loop(){
  runner.execute();
}
