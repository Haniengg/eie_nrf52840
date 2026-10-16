/**
 * @file main.c
 */

#include <inttypes.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "BTN.h"
#include "LED.h"

#define BLINK_MS 500

int main(void) {
  if (0 > BTN_init()) {
    return 0;
  }
  if (0 > LED_init()) {
    return 0;
  }

  printk("Hello from H\n");

  while (1) {
    if (BTN_is_pressed(BTN0)) {
      LED_set(LED1, LED_ON);
    } else {
      LED_set(LED1, LED_OFF);
    }
    k_msleep(10); // check the button 100 times a second
  }
  return 0;
}