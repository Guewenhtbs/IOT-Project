/* Heltec Automation LoRaWAN communication example
 *
 * Function:
 * 1. Upload node data to the server using the standard LoRaWAN protocol.
 *  
 * Description:
 * 1. Communicate using LoRaWAN protocol.
 * 
 * HelTec AutoMation, Chengdu, China
 * 成都惠利特自动化科技有限公司
 * www.heltec.org
 *
 * */

#include "LoRaWan_APP.h"
#include <Arduino.h>

#include "WiFi.h"
#include "LoRa_conf.h"
#include "Images.h"

#include <Wire.h>  
#include "HT_SSD1306Wire.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEEddystoneURL.h>
#include <BLEEddystoneTLM.h>
#include <BLEBeacon.h>
#include <set>
#include <string>

/* LCD Setup */

#ifdef WIRELESS_STICK_V3
static SSD1306Wire  display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_64_32, RST_OLED); // addr , freq , i2c group , resolution , rst
#else
static SSD1306Wire  display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst
#endif

void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) //Vext default OFF
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, HIGH);
}

/* BLE & WiFi Setup */

int scanTime = 1;  //In seconds
BLEScan *pBLEScan;

std::set<String> macListWifi;
std::set<String> ssidListWifi;
int sameMacCounter = 0;

std::set<String> macListBle;
int beaconRssiBle = -100;

bool moving = false;

int BLE_SEUIL_MIN = 50;
int BLE_SEUIL_MAX = 100;
int WIFI_SEUIL_MIN = 5;
int WIFI_SEUIL_MAX = 15;

float BLE_RATIO = 1;
float WIFI_RATIO = 1-BLE_RATIO;
float MOVING_RATIO = 0.5;

/* For BLE Scan */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    String macBle = advertisedDevice.getAddress().toString();
    macListBle.insert(macBle);
    if (advertisedDevice.haveName() && advertisedDevice.getName() == "GUEWENN") {
      beaconRssiBle = advertisedDevice.getRSSI();
      Serial.print("Beacon found, RSSI : ");
      Serial.println(beaconRssiBle);
    }
  }
};

/* For WiFi Scan */
static void wifiscanning(bool firstScan) {
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
      String macWifiString = WiFi.BSSIDstr(i);
      Serial.printf("%-32.32s", macWifiString.c_str());
      Serial.print(" | ");
      String ssidWifiString = WiFi.SSID(i);
      Serial.printf("%-32.32s", ssidWifiString.c_str());
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

      if (firstScan) {
        macListWifi.insert(macWifiString);
        ssidListWifi.insert(ssidWifiString);
      } else {
        if (macListWifi.contains(macWifiString)) {
          sameMacCounter++;
        }
      }
      delay(10);
    }
  }  
}

/* LoRa Setup */
uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x06, 0x53, 0xC8 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


/* ABP para*/
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t devAddr =  ( uint32_t )0x007e6ae1;

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 50000;

/*OTAA or ABP*/
bool overTheAirActivation = true;

/*ADR enable*/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;
/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 4;

/* Prepares the payload of the frame */
static void prepareTxFrame( int macBLE, int macWiFi, int ssidWiFi, int crowded = 0, bool moving = false, int rssiDevice = 100 )
{
    appDataSize = 12;
    appData[0] = highByte(macBLE);
    appData[1] = lowByte(macBLE);

    appData[2] = highByte(macWiFi);
    appData[3] = lowByte(macWiFi);

    appData[4] = highByte(ssidWiFi);
    appData[5] = lowByte(ssidWiFi);

    appData[6] = highByte(crowded);
    appData[7] = lowByte(crowded);

    appData[8] = highByte(moving);
    appData[9] = lowByte(moving);

    appData[10] = highByte(rssiDevice);
    appData[11] = lowByte(rssiDevice);
}


/* Display information about LoRa joining */
static void display_join() {
  display.clear();
  display.drawXbm(
    (display.width()-3*Crowded_width)/4,
    (display.height()-Crowded_height)/2+5,
    Crowded_width,
    Crowded_height,
    scan
  );
  display.drawString(50, 30, "LoRa Joining");
  display.display();
}

/* Display information about scanning */
static void display_scan() {
  display.clear();
  display.drawXbm(
    (display.width()-3*Scan_width)/4,
    (display.height()-Scan_height)/2+5,
    Scan_width,
    Scan_height,
    scan
  );
  display.drawString(50, 30, "scanning");
  display.display();
}

/* Display BLE informations */
static void display_ble(int macBLE) {
  display.clear();
  display.drawXbm(
    (display.width()-3*BLE_width)/4,
    (display.height()-BLE_height)/2+5,
    BLE_width,
    BLE_height,
    ble
  );
  display.drawString(50, 30, "BLE devices: " + String(macBLE));
  display.display();
}

/* Display WiFi informations */
static void display_wifi(int macWiFi, int ssidWiFi) {
  display.clear();
  display.drawXbm(
    (display.width()-3*WiFi_width)/4,
    (display.height()-WiFi_height)/2+5,
    WiFi_width,
    WiFi_height,
    wifi
  );
  display.drawString(50, 20, "WiFi APs: " + String(macWiFi));
  display.drawString(50, 40, "WiFi SSIDs: " + String(ssidWiFi));
  display.display();
}

/* Display Crowd Level informations */
static void display_crowded(int crowded) {
  display.clear();
  switch ( crowded )  {
    case 3 : {
      display.drawXbm(
        (display.width()-3*Crowded_width)/4,
        (display.height()-Crowded_height)/2+5,
        Crowded_width,
        Crowded_height,
        crowded_3
      );
      display.drawString(50, 30, "Crowded");
      break;
    }
    case 2 : {
      display.drawXbm(
        (display.width()-3*Crowded_width)/4,
        (display.height()-Crowded_height)/2+5,
        Crowded_width,
        Crowded_height,
        crowded_2
      );
      display.drawString(50, 30, "Moderate");
      break;
    }
    default : {
      display.drawXbm(
        (display.width()-3*Crowded_width)/4,
        (display.height()-Crowded_height)/2+5,
        Crowded_width,
        Crowded_height,
        crowded_1
      );
      display.drawString(50, 30, "Calm");
      break;
    }
  }
  display.display();
}

/* Display beacon tracking informations */
static void display_beacon(int rssiDevice) {
  display.clear();
  if (rssiDevice <= -100) {
    display.drawXbm(
      (display.width()-3*Visible_width)/4,
      (display.height()-Visible_height)/2+5,
      Visible_width,
      Visible_height,
      hidden
    );
    display.drawString(50, 30, "Beacon hidden");
  } else {
    display.drawXbm(
      (display.width()-3*Visible_width)/4,
      (display.height()-Visible_height)/2+5,
      Visible_width,
      Visible_height,
      visible
    );
    display.drawString(50, 30, "RSSI: " + String(rssiDevice) + "dBm");
  }
  display.display();
}

/* Display mobility informations */
static void display_position(bool moving) {
  display.clear();
  if (moving) {
    display.drawXbm(
      (display.width()-3*Position_width)/4,
      (display.height()-Position_height)/2+5,
      Position_width,
      Position_height,
      movinq
    );
    display.drawString(50, 30, "Mobile");
  } else {
    display.drawXbm(
      (display.width()-3*Position_width)/4,
      (display.height()-Position_height)/2+5,
      Position_width,
      Position_height,
      statiq
    );
    display.drawString(50, 30, "Static");
  }
  display.display();
}

void setup() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();  //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(false);  //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  VextON();
  delay(100);

  display.init();
  display.clear();
  display.display();
  
  display.setContrast(255);

  display_join();

  Serial.println("Setup done");
  delay(1000);
}

void loop()
{
  switch( deviceState )
  {
    case DEVICE_STATE_INIT:
    {
#if(LORAWAN_DEVEUI_AUTO)
      LoRaWAN.generateDeveuiByChipID();
#endif
      LoRaWAN.init(loraWanClass,loraWanRegion);
      //both set join DR and DR when ADR off 
      LoRaWAN.setDefaultDR(3);
      break;
    }
    case DEVICE_STATE_JOIN:
    {
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND:
    {
      display_scan();

      // Partie BLE
      Serial.println("Scan BLE Starting... ");
      macListBle.clear();
      beaconRssiBle = -100;
      BLEScanResults *foundDevices = pBLEScan->start(scanTime, false);
      int macBLE = (int) macListBle.size();

      Serial.print("Nombre total de MAC unique détectées: ");
      Serial.println(macBLE);
      Serial.println("Scan Ble done!");
      pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory 
      delay(100);

      // Partie WiFi
      macListWifi.clear();
      ssidListWifi.clear();
      
      int macWiFi = 0;
      int ssidWiFi = 0;

      wifiscanning(true);

      macWiFi = (int) macListWifi.size();
      ssidWiFi = (int) ssidListWifi.size();
      Serial.print("Nombre total de MAC unique détectées: ");
      Serial.println(macWiFi);
      Serial.print("Nombre total de SSID unique détectées: ");
      Serial.println(ssidWiFi);

      Serial.println("Scan Wifi done");

      // Delete the scan result to free memory for code below.
      WiFi.scanDelete();

      // Wait a bit before scanning again.
      delay(100);
      int crowdedBLE = 0;

      if (macBLE >= BLE_SEUIL_MAX) {
        crowdedBLE = 3;
      } 
      else if (macBLE >= BLE_SEUIL_MIN) {
        crowdedBLE = 2;
      } 
      else {
        crowdedBLE = 1;
      }

      int crowdedWiFi = 0;

      if (ssidWiFi >= WIFI_SEUIL_MAX) {
        crowdedWiFi = 3;
      } 
      else if (ssidWiFi >= WIFI_SEUIL_MIN) {
        crowdedWiFi = 2;
      } 
      else {
        crowdedWiFi = 1;
      }

      int meanCrowded = round(BLE_RATIO*crowdedBLE + WIFI_RATIO*crowdedWiFi);
      Serial.print("Crowded BLE :");
      Serial.print(crowdedBLE);
      Serial.print(", Crowded WiFi :");
      Serial.print(crowdedWiFi);
      Serial.print(", Valeur moyenne :");
      Serial.print((float) BLE_RATIO*crowdedBLE + WIFI_RATIO*crowdedWiFi);
      Serial.print(", Valeur envoyée :");
      Serial.println(meanCrowded);
      display_ble(macBLE);
      delay(5000);
      display_wifi(macWiFi, ssidWiFi);
      delay(5000);
      display_crowded(meanCrowded);
      delay(5000);
      display_beacon(beaconRssiBle);
      delay(5000);
      display_scan();
      sameMacCounter = 0;
      wifiscanning(false);

      Serial.println(sameMacCounter);
      Serial.println((int) macListWifi.size());
      moving = (sameMacCounter < (int) macListWifi.size() * MOVING_RATIO);
      if (moving) {
        Serial.println("L'environnement est en mouvement");
      } else {
        Serial.println("L'environnement est statique");
      }

      // Delete the scan result to free memory for code below.
      WiFi.scanDelete();

      // Wait a bit before scanning again.
      delay(100);

      display_position(moving);
      delay(5000);

      prepareTxFrame(macBLE, macWiFi, ssidWiFi, meanCrowded, moving, abs(beaconRssiBle));

      LoRaWAN.send();
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP:
    {
      LoRaWAN.sleep(loraWanClass);
      break;
    }
    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
}