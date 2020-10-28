#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#define PIN_TIME 2
void watchdogSleep1Second();
void setup_watchdog(int timerPrescaler);

void setup() {
  pinMode(PIN_TIME, OUTPUT);

  //Power down various bits of hardware to lower power usage
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); //Power down everything, wake up from WDT
  sleep_enable();

}

void loop() {
  watchdogSleep1Second();

  if(digitalRead(PIN_TIME) == HIGH) {
    digitalWrite(PIN_TIME, LOW);
  } else {
    digitalWrite(PIN_TIME, HIGH);
  }
}

void watchdogSleep1Second() {
  ADCSRA &= ~(1 << ADEN); //Disable ADC, saves ~230uA
  setup_watchdog(6); //Setup watchdog to go off after 1sec
  sleep_mode(); //Go to sleep! Wake up 1sec later and check water
  ADCSRA |= (1 << ADEN); //Enable ADC
}

//This runs each time the watch dog wakes us up from sleep
ISR(WDT_vect) {
}

//Sets the watchdog timer to wake us up, but not reset
//0=16ms, 1=32ms, 2=64ms, 3=128ms, 4=250ms, 5=500ms
//6=1sec, 7=2sec, 8=4sec, 9=8sec
//From: http://interface.khm.de/index.php/lab/experiments/sleep_watchdog_battery/
void setup_watchdog(int timerPrescaler) {

  if (timerPrescaler > 9 ) timerPrescaler = 9; //Limit incoming amount to legal settings

  byte bb = timerPrescaler & 7;
  if (timerPrescaler > 7) bb |= (1 << 5); //Set the special 5th bit if necessary

  //This order of commands is important and cannot be combined
  MCUSR &= ~(1 << WDRF); //Clear the watch dog reset
  WDTCR |= (1 << WDCE) | (1 << WDE); //Set WD_change enable, set WD enable
  WDTCR = bb; //Set new watchdog timeout value
  WDTCR |= _BV(WDIE); //Set the interrupt enable, this will keep unit from resetting after each int
}