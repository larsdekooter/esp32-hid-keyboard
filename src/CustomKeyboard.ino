#include <Arduino.h>
#include "tusb.h"
#include "bios.h"   // the header we built earlier

void setup() {
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, HIGH);
  tusb_init();         // initialize TinyUSB
  delay(2000);         // wait for host to enumerate
  digitalWrite(LED_GREEN, LOW);
  bios_print("HELLO BIOS!\n");
}

void loop() {
  tud_task();          // keep USB stack alive
}
