#define ESP32
//test
//test1
#if defined(ESP32)
  #include <WiFiMulti.h>
  WiFiMulti wifiMulti;
  #define DEVICE "ESP32"
#elif defined(ESP8266)
  #include <ESP8266WiFiMulti.h>
  ESP8266WiFiMulti wifiMulti;
  #define DEVICE "ESP8266"
#endif

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// WiFi AP SSID
#define WIFI_SSID "AAA"
// WiFi password
#define WIFI_PASSWORD "123456789"

#define INFLUXDB_URL "http://192.168.102.34:8086"
#define INFLUXDB_TOKEN "c0hxqdAmghFyH8PPMCw0J4mbMJ-lel9tc0SFxoIbm_8S1krAfLpH-01XSP6dH6V1C-cmPFPwYvAhSvC-kvMWpw=="
#define INFLUXDB_ORG "ddde3dc84eee1ca4"
#define INFLUXDB_BUCKET "data1"

// Time zone info
#define TZ_INFO "UTC7"

// Declare InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Declare Data point
// We'll create the Point dynamically each measurement to avoid stale fields

// Measurement interval (ms)
#define SAMPLE_INTERVAL_MS 15000


void setup() {
  Serial.begin(115200);

  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  // Accurate time is necessary for certificate validation and writing in batches
  // We use the NTP servers in your area as provided by: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");


  // Check server connection
  Serial.println("Validating InfluxDB connection...");
  Serial.print("Server URL: ");
  Serial.println(client.getServerUrl());
  
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.println("InfluxDB connection failed!");
    Serial.print("Error: ");
    Serial.println(client.getLastErrorMessage());
    Serial.println("Please check:");
    Serial.println("1. InfluxDB server is running");
    Serial.println("2. IP address and port are correct");
    Serial.println("3. Your network allows the connection");
    Serial.print("4. Device IP address: ");
    Serial.println(WiFi.localIP());
  }

  // Seed random generator for numeric id generation
#if defined(ESP32)
  randomSeed((unsigned long)(micros() ^ (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF)));
#else
  randomSeed((unsigned long)micros());
#endif
}

// Simple helper to mock sensor readings. Replace with real sensor code.
float readTemperature() { return 24.5 + (random(-50,50) / 100.0); }
float readHumidity() { return 50.0 + (random(-200,200) / 100.0); }
float readPressure() { return 1013.25 + (random(-300,300) / 100.0); }

void sendMeasurement() {
  // Device name and unique id (MAC)
  String deviceName = String(DEVICE);
  String mac = WiFi.macAddress();

  // Generate random numeric id per measurement (1..100)
  int numericId = random(1, 101);

  // Read values
  float temperature = readTemperature();
  float humidity = readHumidity();
  float pressure = readPressure();

  // Create point with measurement name "environment"
  Point sensor("environment");
  sensor.addTag("device", deviceName.c_str());
  // Add numeric id as a field (1..100)
  sensor.addField("id", numericId);
  sensor.addField("temperature", temperature);
  sensor.addField("humidity", humidity);
  sensor.addField("pressure", pressure);

  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  } else {
  Serial.print("Wrote: id="); Serial.print(numericId);
    Serial.print(",t="); Serial.print(temperature);
    Serial.print(",h="); Serial.print(humidity);
    Serial.print(",p="); Serial.println(pressure);
  }
}

unsigned long lastSample = 0;

void loop() {
  unsigned long now = millis();
  if (now - lastSample >= SAMPLE_INTERVAL_MS) {
    lastSample = now;
    sendMeasurement();
  }
  delay(10);
}