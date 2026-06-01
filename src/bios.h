#ifndef BIOS_H
#define BIOS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the BIOS/boot-protocol keyboard USB interface.
// Must be called once during setup().
void bios_init();

// Send a single HID keycode with modifiers (modifiers bitmask per USB HID: bit0=Ctrl, bit1=Shift, bit2=Alt, bit3=GUI).
// keycode should be the USB HID usage (e.g. 0x04 for 'a').
// The function will press then release the key.
void bios_send_keycode(uint8_t modifiers, uint8_t keycode);

// Send a single ASCII character (handles basic letters, digits, common punctuation).
// Returns true if sent, false if character not supported.
bool bios_send_char(char c);

// Print a null-terminated string (sends each char sequentially).
// Note: does not add delays itself — BIOSes can be picky: you may want to add small delays between calls in your usage.
void bios_print(const char* s);

// Low level: send raw boot-keyboard report (modifiers, six-key-array).
// Provided for advanced usage.
void bios_send_raw_report(uint8_t modifiers, const uint8_t keys[6]);

#ifdef __cplusplus
}
#endif

#endif // BIOS_H
