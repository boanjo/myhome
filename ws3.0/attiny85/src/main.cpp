#include <Arduino.h>
#include <TinyWireS.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>

#define I2C_SLAVE_ADDR 0x10

#define RAIN_PIN     4  // PB4 (pin 3)
#define OPT_CLEAR    3  // PB3 (pin 2)

volatile uint32_t rainCount = 0;
volatile bool tipFlag = false;

uint32_t EEMEM storedCount;

void onRequest();
void onReceive(uint8_t);

void setup() {
  pinMode(RAIN_PIN, INPUT);
  pinMode(OPT_CLEAR, INPUT);

  // Check if reset should happen (jumper pulls OPT_CLEAR low)
  if (digitalRead(OPT_CLEAR) == LOW) {
    rainCount = 0;
    eeprom_update_block((const void*)&rainCount, &storedCount, sizeof(rainCount));
  } else {
    // Load previous count from EEPROM
    eeprom_read_block((void*)&rainCount, &storedCount, sizeof(rainCount));
  }

  TinyWireS.begin(I2C_SLAVE_ADDR);
  TinyWireS.onRequest(onRequest);
  TinyWireS.onReceive(onReceive);

  // Enable Pin Change Interrupt for PB4 (RAIN_PIN)
  GIMSK |= (1 << PCIE);       // Enable pin change interrupts
  PCMSK |= (1 << PCINT4);     // Enable PCINT on PB4
}

void loop() {
  //TinyWireS_stop_check - it relies on manually calling this function often to stay in sync with the bus.
  TinyWireS_stop_check();

  if (tipFlag) {
    tipFlag = false;

    rainCount++;
    eeprom_update_block((const void*)&rainCount, &storedCount, sizeof(rainCount));
  }
}

volatile uint32_t lastTipMicros = 0;

ISR(PCINT0_vect) {
  uint8_t currentState = digitalRead(RAIN_PIN);
  if (currentState == LOW) {
    uint32_t now = micros();
    if (now - lastTipMicros > 5000) {  // 5 ms debounce
      tipFlag = true;
      lastTipMicros = now;
    }
  }
}

void onRequest() {
  TinyWireS.send((rainCount >> 24) & 0xFF);
  TinyWireS.send((rainCount >> 16) & 0xFF);
  TinyWireS.send((rainCount >> 8) & 0xFF);
  TinyWireS.send(rainCount & 0xFF);
}

void onReceive(uint8_t bytes) {
  if (bytes >= 1) {
    uint8_t command = TinyWireS.receive();
    if (command == 0x01) {
      rainCount = 0;
      eeprom_update_block((const void*)&rainCount, &storedCount, sizeof(rainCount));
    }
  }
}
