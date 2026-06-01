#include "bios.h"
#include "tusb.h"    // tinyusb device API - make sure your build includes tinyusb
#include <string.h>
#include <ctype.h>
#include <Arduino.h>


// HID boot keyboard report is 8 bytes:
// [0] - modifiers
// [1] - reserved
// [2..7] - up to 6 keycodes
struct hid_boot_kbd_report_t {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
};

// Simple ASCII -> HID usage mapping for common characters.
// This table is intentionally small — extend as needed.
static bool ascii_to_hid(char c, uint8_t *out_mod, uint8_t *out_key)
{
  *out_mod = 0;
  *out_key = 0;

  // letters a-z
  if (c >= 'a' && c <= 'z') {
    *out_key = 0x04 + (c - 'a'); // HID usage for 'a' is 0x04
    return true;
  }
  if (c >= 'A' && c <= 'Z') {
    *out_mod = (1 << 1); // Left Shift
    *out_key = 0x04 + (c - 'A');
    return true;
  }

  // digits 1-9, 0
  if (c >= '1' && c <= '9') {
    *out_key = 0x1E + (c - '1'); // HID 1..9 start at 0x1E
    return true;
  }
  if (c == '0') {
    *out_key = 0x27; // HID 0
    return true;
  }

  // common punctuation and whitespace
  switch (c) {
    case ' ': *out_key = 0x2C; return true; // Space
    case '\n': *out_key = 0x28; return true; // Enter
    case '\r': *out_key = 0x28; return true; // Enter
    case '\b': *out_key = 0x2A; return true; // Backspace
    case '-': *out_key = 0x2D; return true; 
    case '_': *out_mod = (1<<1); *out_key = 0x2D; return true; 
    case '=': *out_key = 0x2E; return true; 
    case '+': *out_mod = (1<<1); *out_key = 0x2E; return true;
    case '[': *out_key = 0x2F; return true;
    case '{': *out_mod = (1<<1); *out_key = 0x2F; return true;
    case ']': *out_key = 0x30; return true;
    case '}': *out_mod = (1<<1); *out_key = 0x30; return true;
    case '\\': *out_key = 0x31; return true;
    case '|': *out_mod = (1<<1); *out_key = 0x31; return true;
    case ';': *out_key = 0x33; return true;
    case ':': *out_mod = (1<<1); *out_key = 0x33; return true;
    case '\'': *out_key = 0x34; return true;
    case '"': *out_mod = (1<<1); *out_key = 0x34; return true;
    case '`': *out_key = 0x35; return true;
    case '~': *out_mod = (1<<1); *out_key = 0x35; return true;
    case ',': *out_key = 0x36; return true;
    case '<': *out_mod = (1<<1); *out_key = 0x36; return true;
    case '.': *out_key = 0x37; return true;
    case '>': *out_mod = (1<<1); *out_key = 0x37; return true;
    case '/': *out_key = 0x38; return true;
    case '?': *out_mod = (1<<1); *out_key = 0x38; return true;
    case '!': *out_mod = (1<<1); *out_key = 0x1E; return true; // Shift + 1
    case '@': *out_mod = (1<<1); *out_key = 0x1F; return true; // Shift + 2
    case '#': *out_mod = (1<<1); *out_key = 0x20; return true; // Shift + 3
    case '$': *out_mod = (1<<1); *out_key = 0x21; return true; // Shift + 4 
    case '%': *out_mod = (1<<1); *out_key = 0x22; return true; // Shift + 5
    case '^': *out_mod = (1<<1); *out_key = 0x23; return true; // Shift + 6
    case '&': *out_mod = (1<<1); *out_key = 0x24; return true; // Shift + 7
    case '*': *out_mod = (1<<1); *out_key = 0x25; return true; // Shift + 8
    case '(': *out_mod = (1<<1); *out_key = 0x26; return true; // Shift + 9
    case ')': *out_mod = (1<<1); *out_key = 0x27; return true; // Shift + 0
    
    default:
        return false;
}
}

// Wait until TinyUSB HID is ready for a report (non-blocking busy-wait).
// You may tweak timeout behavior or convert to non-blocking as needed.
static void _wait_tud_ready(void) {
  // tinyusb provides tud_hid_ready() to check if interface can accept report
  // small spin until ready -- BIOS is slow; a tiny delay helps
  uint32_t t0 = millis();
  while (!tud_hid_ready()) {
    // yield so USB stack can process
    delay(1);
    // avoid infinite loop in case device not enumerated
    if ((millis() - t0) > 5000) break;
  }
}

void bios_init()
{
  // TinyUSB stack must be initialized by the environment (Arduino core for S2/S3 often does that).
  // If not initialized already, call tusb_init() here.
  // Note: many Arduino + TinyUSB setups do this for you — check your board core.
  tusb_init();
}

void bios_send_raw_report(uint8_t modifiers, const uint8_t keys[6])
{
  hid_boot_kbd_report_t rpt;
  rpt.modifiers = modifiers;
  rpt.reserved = 0;
  memcpy(rpt.keys, keys, 6);

  _wait_tud_ready();
  // report id = 0 for boot keyboard; length = sizeof(report)
  tud_hid_report(0, &rpt, sizeof(rpt));
}

void bios_send_keycode(uint8_t modifiers, uint8_t keycode)
{
  uint8_t keys[6] = {0,0,0,0,0,0};
  keys[0] = keycode;
  bios_send_raw_report(modifiers, keys);

  // release
  uint8_t zero[6] = {0,0,0,0,0,0};
  bios_send_raw_report(0, zero);
}

bool bios_send_char(char c)
{
  uint8_t mod = 0;
  uint8_t key = 0;
  if (!ascii_to_hid(c, &mod, &key)) return false;

  bios_send_keycode(mod, key);
  // short delay to ensure BIOS registers key (adjust if needed)
  delay(20);
  return true;
}

void bios_print(const char* s)
{
  if (!s) return;
  while (*s) {
    // attempt to send; if unsupported char, skip
    if (!bios_send_char(*s)) {
      // you can add fallback handling (e.g., send via ALT+Numpad) if needed
    }
    s++;
    // short gap — BIOS interfaces often need a bit more time between characters
    delay(30);
  }
}
