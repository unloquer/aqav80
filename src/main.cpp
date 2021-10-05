// #define _TASK_TIMECRITICAL      // Enable monitoring scheduling overruns
#define _TASK_SLEEP_ON_IDLE_RUN // Enable 1 ms SLEEP_IDLE powerdowns between
                                // tasks if no callback methods were invoked
                                // during the pass
#define _TASK_STATUS_REQUEST    // Compile with support for StatusRequest
                                // functionality - triggering tasks on status
                                // change events in addition to time only
#define _TASK_WDT_IDS // Compile with support for wdt control points and task
                      // ids
#define _TASK_LTS_POINTER // Compile with support for local task storage pointer
#define _TASK_PRIORITY    // Support for layered scheduling priority
// #define _TASK_MICRO_RES         // Support for microsecond resolution
// #define _TASK_STD_FUNCTION      // Support for std::function (ESP8266 and
// ESP32 ONLY)
#define _TASK_DEBUG  // Make all methods and variables public for debug purposes
#define _TASK_INLINE // Make all methods "inline" - needed to support some
                     // multi-tab, multi-file implementations
#define _TASK_TIMEOUT // Support for overall task timeout

#define FIXED_LAT "6.263578"
#define FIXED_LON "-75.597417"
#define SENSOR_ID "SNS-1"

#define DEBUGGING
#ifdef DEBUGGING
#define DMSG(args...) Serial.print(args)
#define DMSGf(args...) Serial.printf(args)
#define DMSGln(args...) Serial.println(args)
#else
#define DMSG(args...)
#define DMSGf(args...)
#define DMSGln(str)
#endif

#define INFLUXDB_HOST "aqa.unloquer.org"
// #define INFLUXDB_PORT "8086"
#define INFLUXDB_DATABASE "v80"
// if used with authentication
// #define INFLUXDB_USER "user"
// #define INFLUXDB_PASS "password"

using namespace std;
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FastLED.h>
#include <InfluxDb.h>
#include <TaskScheduler.h>
#include <vector>

#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASS "WIFI_PASSWORD"

Influxdb influx(INFLUXDB_HOST);

vector<unsigned int> vmic; // for average
unsigned int amic = 0;

bool ledToggle = false;

// MIC
const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;

// LED
#define LED_PIN D1
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS 1
CRGB leds[NUM_LEDS];
int BRIGHTNESS = 10; // this is half brightness

/***
 * Average methods
 **/

void saveMicDataForAverage(unsigned int mic) {
  vmic.push_back(mic);
  Serial.print(",");
  Serial.println(mic);
}
unsigned int getMicAverage() {
  unsigned int mic_average =
      accumulate(vmic.begin(), vmic.end(), 0.0) / vmic.size();
  vmic.clear();
  return mic_average;
}
void micAverageLoop() {
  amic = getMicAverage(); // global var for display
}

Scheduler runner;

// CALLBACKS
void sendToInflux();
void printMicAverage() {
  micAverageLoop();
  DMSG("Mic average ");
  DMSG(",");
  DMSGln(amic);
}
void readMicData() {
  DMSG("Leyendo Mic ... ");
  unsigned long startMillis = millis(); // Start of sample window
  unsigned int peakToPeak = 0;          // peak-to-peak level

  unsigned int signalMax = 0;
  unsigned int signalMin = 1024;

  // collect data for 50 mS
  while (millis() - startMillis < sampleWindow) {
    sample = analogRead(0);
    if (sample > signalMax) {
      signalMax = sample; // save just the max levels
    } else if (sample < signalMin) {
      signalMin = sample; // save just the min levels
    }
  }
  peakToPeak = signalMax - signalMin; // max - min = peak-peak amplitude
  // saveMicDataForAverage(peakToPeak * 0.07447 + 39.82947);
  // Esta es la función de ajuste lineal. Revisar proceso de calibración
  // ../utils/regresion.R
  saveMicDataForAverage(peakToPeak);
}
CRGB setColor() {
  CRGB alert = CRGB::Black;

  // Adaptar escala para sonido
  // if (apm25 < 12)
  //   alert = CRGB::Green;
  // if (apm25 >= 12 && apm25 < 35)
  //   alert = CRGB::Gold;
  // if (apm25 >= 35 && apm25 < 55)
  //   alert = CRGB::Tomato;
  // if (apm25 >= 55 && apm25 < 150)
  //   alert = CRGB::DarkRed;
  // if (apm25 >= 150 && apm25 < 250)
  //   alert = CRGB::Purple;
  // if (apm25 >= 250)
  //   alert = CRGB::Brown;

  // return alert;
}
void setLed() {
  ledToggle = !ledToggle;
  leds[0] = ledToggle ? setColor() : CRGB::Black;
  FastLED.show();
}

// TASKS
Task readMicTask(120, TASK_FOREVER, &readMicData);
Task writeToInflux(15000, TASK_FOREVER, &sendToInflux);
Task ledBlink(1000, TASK_FOREVER, &setLed);
// CALLBACKS
void sendToInflux() {
  micAverageLoop();
  ledBlink.setInterval(map(amic, 0, 150, 1000, 400));
  char row[256];

  sprintf(row, "%s,id=%s lat=%s,lng=%s,s=%u", SENSOR_ID, SENSOR_ID, FIXED_LAT,
          FIXED_LON, amic);
  DMSGln(row);

  influx.write(row);
}

void connectToWifi() {
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

void setup() {
  Serial.begin(115200);
  connectToWifi();
  // setup time
  runner.init();
  DMSGln("Initialized scheduler");
  runner.addTask(readMicTask);
  runner.addTask(writeToInflux);
  // runner.addTask(ledBlink);
  DMSGln("added tasks");
  readMicTask.enable();
  writeToInflux.enable();
  // ledBlink.enable();
}

void loop() { runner.execute(); }
