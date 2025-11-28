/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
   Changed to a beacon scanner to report iBeacon, EddystoneURL and EddystoneTLM beacons by beegee-tokyo
   Upgraded Eddystone part by Tomas Pilny on Feb 20, 2023
*/

#include <Arduino.h>

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

std::set<String> macList;

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

    Serial.println("\n");
    Serial.println(advertisedDevice.getAddress().toString().c_str());
    Serial.println("\n");

    String mac = advertisedDevice.getAddress().toString();

    macList.insert(mac);

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
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("Scan Starting... ");
  macList.clear();
  BLEScanResults *foundDevices = pBLEScan->start(scanTime, false);
  Serial.print("Devices found: ");
  Serial.println(foundDevices->getCount());
  Serial.println("Scan done!");

  Serial.print("Nombre total de MAC unique détectées: ");
  Serial.println(macList.size()); 

  String mac = "c8:a1:dc:92:3c:1c";

    // Vérifier si la MAC est déjà dans le set
  if (macList.find(mac) != macList.end()) {
        Serial.println("MAC ICI !");
  } 

  int a = 1;

  Serial.println("Liste des MAC uniques :");
  for (const auto &mac : macList) {
      Serial.println(a);
      Serial.println(mac);
      a++;
  }


  pBLEScan->clearResults();  // delete results fromBLEScan buffer to release memory
  delay(15000);
}
