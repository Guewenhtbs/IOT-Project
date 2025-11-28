/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
   Changed to a beacon scanner to report iBeacon, EddystoneURL and EddystoneTLM beacons by beegee-tokyo
   Upgraded Eddystone part by Tomas Pilny on Feb 20, 2023
*/

#include <Arduino.h>

#include "WiFi.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEEddystoneURL.h>
#include <BLEEddystoneTLM.h>
#include <BLEBeacon.h>
#include <set>
#include <string>

int scanTime = 1;  //In seconds
BLEScan *pBLEScan;

std::set<String> macListWifi;
std::set<String> ssidListWifi;

std::set<String> macListBle;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveName()) {
      Serial.print("Device name: ");
      Serial.println(advertisedDevice.getName().c_str());
      Serial.println("");
    }

    if (advertisedDevice.haveServiceUUID()) {
      BLEUUID devUUID = advertisedDevice.getServiceUUID();
      Serial.print("Found ServiceUUID: ");
      Serial.println(devUUID.toString().c_str());
      Serial.println("");
    }

    //Serial.println("\n");
    //Serial.println(advertisedDevice.getAddress().toString().c_str());
    //Serial.println("\n");

    String macBle = advertisedDevice.getAddress().toString();

    macListBle.insert(macBle);

  }


};

void setup() {
  Serial.begin(115200);
  Serial.println("Scanning...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();  //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(false);  //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("Setup done");


}

void loop() {
  // PARTIE BLE

  // put your main code here, to run repeatedly:
  Serial.println("");
  Serial.println("Scan Ble Starting... ");
  Serial.println("");
  macListBle.clear();
  BLEScanResults *foundDevices = pBLEScan->start(scanTime, false);
  Serial.print("Devices found: ");
  Serial.println(foundDevices->getCount());


  Serial.print("Nombre total de MAC unique détectées: ");
  Serial.println( (int) macListBle.size());


  int a = 1;

  Serial.println("Liste des MAC uniques :");
  for (const auto &macBle : macListBle) {
      Serial.println(a);
      Serial.println(macBle);
      a++;
  }

  Serial.println("");
  Serial.println("Scan Ble done!");
  Serial.println("");



  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory 
  delay(15000);

  // PARTIE WIFI

  macListWifi.clear();
  ssidListWifi.clear();
  Serial.println("Scan Wifi start");
  Serial.println("");

  // WiFi.scanNetworks will return the number of networks found.
  int n = WiFi.scanNetworks();

  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    Serial.println("Nr | MAC                              | SSID                             | RSSI | CH | Encryption");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.printf("%2d", i + 1);
      Serial.print(" | ");
      String macWifi = WiFi.BSSIDstr(i);
      Serial.printf("%-32.32s", macWifi.c_str());
      Serial.print(" | ");
      String ssidWifi = WiFi.SSID(i);
      Serial.printf("%-32.32s", ssidWifi.c_str());
      Serial.print(" | ");
      Serial.printf("%4ld", WiFi.RSSI(i));
      Serial.print(" | ");
      Serial.printf("%2ld", WiFi.channel(i));
    
      
      Serial.print(" | ");
      switch (WiFi.encryptionType(i)) {
        case WIFI_AUTH_OPEN:            Serial.print("open"); break;
        case WIFI_AUTH_WEP:             Serial.print("WEP"); break;
        case WIFI_AUTH_WPA_PSK:         Serial.print("WPA"); break;
        case WIFI_AUTH_WPA2_PSK:        Serial.print("WPA2"); break;
        case WIFI_AUTH_WPA_WPA2_PSK:    Serial.print("WPA+WPA2"); break;
        case WIFI_AUTH_WPA2_ENTERPRISE: Serial.print("WPA2-EAP"); break;
        case WIFI_AUTH_WPA3_PSK:        Serial.print("WPA3"); break;
        case WIFI_AUTH_WPA2_WPA3_PSK:   Serial.print("WPA2+WPA3"); break;
        case WIFI_AUTH_WAPI_PSK:        Serial.print("WAPI"); break;
        default:                        Serial.print("unknown");
      }
      Serial.println();
      

      macListWifi.insert(macWifi);
      ssidListWifi.insert(ssidWifi);


      delay(10);
    }

  

    Serial.print("Nombre total de MAC unique détectées: ");
    Serial.println((int) macListWifi.size());
    Serial.print("Nombre total de SSID unique détectées: ");
    Serial.println((int) ssidListWifi.size());
  }
  Serial.println("");
  Serial.println("Scan Wifi done");
  Serial.println("");

  // Delete the scan result to free memory for code below.
  WiFi.scanDelete();

  // Wait a bit before scanning again.
  delay(15000);

}
