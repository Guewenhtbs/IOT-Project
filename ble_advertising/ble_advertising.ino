#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertising.h>
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_ble_api.h"

#define BEACON_NAME "GUEWENN"

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE beacon...");

  BLEDevice::init(BEACON_NAME);

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

  BLEAdvertisementData advData;
  advData.setFlags(ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
  advData.setName(BEACON_NAME);

  pAdvertising->setAdvertisementData(advData);
  

  pAdvertising->setMinInterval(0x20); // 20ms
  pAdvertising->setMaxInterval(0x40); // 40ms

  pAdvertising->start();

}

void loop() {
  delay(5000);
}