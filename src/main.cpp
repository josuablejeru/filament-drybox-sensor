#include <Arduino.h>
#include <ESP8266WiFiMulti.h>
#include <secrets.h>
#include <influxdb.h>
#include <utility>
#include <WEMOS_SHT3X.h>

#define DEVICE "ESP8266"

#include <InfluxDbClient.h>

ESP8266WiFiMulti wifiMulti;

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
SHT3X sht30(0x44);
Point sensor("inside_temperature");

void setup()
{
  Serial.begin(9600);

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  timeSync(TZ_INFO, "de.pool.ntp.org");

  // Check server connection
  if (client.validateConnection())
  {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  }
  else
  {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  sensor.addTag("location", "office");
  sensor.addTag("sensor", "SHT30");
  sensor.addTag("device", DEVICE);
}

/**
 * Returns the temperature and humidity from the SHT30 sensor
 * if the floats are 0, an error occurred
 * @return std::pair<float, float>
 */
std::pair<float, float> getTemperatureAndHumidity()
{
  if (sht30.get() == 0)
  {
    return std::make_pair(sht30.cTemp, sht30.humidity);
  }
  else
  {
    Serial.println("Error reading sensor data");
    return std::make_pair(0, 0);
  }
}

void loop()
{
  Serial.print("Sending data to InfluxDB: ");

  sensor.clearFields();

  // Read the data and write it to the sensor
  std::pair measurement = getTemperatureAndHumidity();
  sensor.addField("temperature", measurement.first);
  sensor.addField("humidity", measurement.second);

  // Print what are we exactly writing
  Serial.print("Writing: ");
  Serial.println(sensor.toLineProtocol());

  // Check WiFi connection and reconnect if needed
  if (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.println("Wifi connection lost");
  }

  // Write point to InfluxDB
  if (!client.writePoint(sensor))
  {
    Serial.println(client.getLastErrorMessage());
  }

  delay(1000);
}
