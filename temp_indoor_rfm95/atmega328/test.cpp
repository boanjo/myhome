#include <SPI.h>
#include <RH_RF95.h>

// --- Standard MySensors RFM95 wiring on ATmega328P ---
// GND    → GND
// VCC    → 3.3V (IMPORTANT: not 5V!)
// D10    → NSS (CS)
// D2     → DIO0 (interrupt)
// D13    → SCK
// D12    → MISO (CIPO)
// D11    → MOSI (COPI)

#define RFM95_CS   10   // D10 → CS/NSS
#define RFM95_IRQ  2    // D2  → DIO0
#define RFM95_FREQ 868.0

RH_RF95 rf95(RFM95_CS, RFM95_IRQ);

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("ATmega328P RFM95 LoRa TX init");

  if (!rf95.init()) {
    Serial.println("LoRa init failed! Check wiring.");
    while (1);
  }
  Serial.println("LoRa init OK");

  if (!rf95.setFrequency(RFM95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }

  Serial.print("Set frequency to "); Serial.println(RFM95_FREQ);
  rf95.setTxPower(13, false);
}

void loop() {
  const char *msg = "Hello from ATmega TX!";
  rf95.send((uint8_t*)msg, strlen(msg));
  rf95.waitPacketSent();
  Serial.println("Sent packet");
  delay(2000);
}
