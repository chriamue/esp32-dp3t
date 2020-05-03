/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by pcbreflux
*/


/*
   Create a BLE server that will send periodic iBeacon frames.
   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create advertising data
   3. Start advertising.
   4. wait
   5. Stop advertising.
   6. deep sleep

*/
#include "sys/time.h"

#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEBeacon.h"
#include "esp_sleep.h"

#include "keystore.h"

#define GPIO_DEEP_SLEEP_DURATION     10  // sleep x seconds and then wake up
RTC_DATA_ATTR static time_t last;        // remember last boot in RTC Memory
RTC_DATA_ATTR static uint32_t bootcount; // remember number of boots in RTC Memory

#ifdef __cplusplus
extern "C" {
#endif

uint8_t temprature_sens_read();
//uint8_t g_phyFuns;

#ifdef __cplusplus
}
#endif

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
BLEAdvertising *pAdvertising;
struct timeval now;

#define LONG_UUID           "0000FD6F-0000-1000-8000-00805F9B34FB" // UUID 1 128-Bit (may use linux tool uuidgen or random numbers via https://www.uuidgenerator.net/)
#define SHORT_UUID            "FD6F"
void print_hex(const uint8_t *x, int len)
{
  int i;
  for (i = 0; i < len; i++) {
    Serial.printf("%02x", x[i]);
  }
  Serial.printf("\n");
}

void setAdvertisement() {
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  oAdvertisementData.setCompleteServices(BLEUUID(SHORT_UUID));
  //oAdvertisementData.setPartialServices(BLEUUID(SHORT_UUID));
  oAdvertisementData.setFlags(0x1A); // 0x01 Genaral Discoverable // BR_EDR_NOT_SUPPORTED 0x04

  // Example: 0x02011A020AEB03036FFD13166FFD7007DBBD684CB2DEF80F51ED19F3B9DC
  std::string strServiceData = "";

  strServiceData += (char)0x02;     // Len
  strServiceData += (char)0x01;   // Type
  strServiceData += (char)0x1A;   // Flag

  strServiceData += (char)0x02;     // Len
  strServiceData += (char)0x0A;   // Type
  strServiceData += (char)0xEB;   //

  strServiceData += (char)0x03;     // Len
  strServiceData += (char)0x03;   // Type
  strServiceData += (char)0x6FFD;   //

  std::string buff = "6FFD";
  const uint8_t * ephid = dp3t_get_ephid(0);
  for (int i = 0; i < 16; i++) {
    char str[3];
    sprintf(str, "%02X", ephid[i]);
    buff += str;
  }
  strServiceData += (char)0x13;   // Len
  strServiceData += (char)0x16;   // Type
  strServiceData += buff;

  std::string strServiceData2 = "";
  strServiceData2 += (char)0x13;    // Len
  strServiceData2 += (char)0x16;   // Type
  strServiceData2 += buff;
  //oAdvertisementData.setServiceData(BLEUUID(BEACON_UUID), "02011A020AEB03036FFD13166FFD7007DBBD684CB2DEF80F51ED19F3B9DC");
  oAdvertisementData.addData(strServiceData);
  //oAdvertisementData.setServiceData(BLEUUID(SHORT_UUID), strServiceData);
  pAdvertising->setAdvertisementData(oAdvertisementData);
  Serial.print(oAdvertisementData.getPayload().c_str());
  Serial.print(strServiceData.c_str());
}

void initDP3T() {
  uint8_t *ephid, *sk_t0;
  sk_t0 = dp3t_get_skt_0();
  Serial.print("sk_t0: ");
  print_hex(sk_t0, 32);

  dp3t_generate_beacons(sk_t0, 0);
  Serial.print("ephid 0: ");
  print_hex(dp3t_get_ephid(0), 16);
}

void setup() {


  Serial.begin(115200);

  initDP3T();

  gettimeofday(&now, NULL);

  Serial.printf("start ESP32 %d\n", bootcount++);

  Serial.printf("deep sleep (%lds since last reset, %lds since last boot)\n", now.tv_sec, now.tv_sec - last);

  last = now.tv_sec;

  // Create the BLE Device
  BLEDevice::init("");

  // Create the BLE Server
  // BLEServer *pServer = BLEDevice::createServer(); // <-- no longer required to instantiate BLEServer, less flash and ram usage

  pAdvertising = BLEDevice::getAdvertising();

  setAdvertisement();
  // Start advertising
  pAdvertising->start();
  Serial.println("Advertizing started...");
  delay(100);
  pAdvertising->stop();
  Serial.printf("enter deep sleep\n");
  esp_deep_sleep(1000000LL * GPIO_DEEP_SLEEP_DURATION);
  Serial.printf("in deep sleep\n");
}

void loop() {
}
