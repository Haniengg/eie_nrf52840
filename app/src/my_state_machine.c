/**
 * @file my_state_machine.c
 *
 * Lesson 5 Exercise 2
 * Diagram numbering matches the board labels (LED1-4, Button 1-4),
 * which are LED0-LED3 and BTN0-BTN3 in code.
 */

#include <zephyr/smf.h>
#include <zephyr/sys/printk.h>

#include "BTN.h"
#include "LED.h"
#include "my_state_machine.h"

/*--------------------------------------------------------------------
 * Function Prototypes
 *--------------------------------------------------------------------*/
static void s0_entry(void* o);
static enum smf_state_result s0_run(void* o);
static void s0_exit(void* o);

static void s1_entry(void* o);
static enum smf_state_result s1_run(void* o);
static void s1_exit(void* o);

static void s2_entry(void* o);
static enum smf_state_result s2_run(void* o);
static void s2_exit(void* o);

static void s3_entry(void* o);
static enum smf_state_result s3_run(void* o);
static void s3_exit(void* o);

static void s4_entry(void* o);
static enum smf_state_result s4_run(void* o);
static void s4_exit(void* o);

static void all_leds_off(void);
static void clear_all_buttons(void);

/*--------------------------------------------------------------------
 * Typedefs
 *--------------------------------------------------------------------*/
enum led_state_machine_states {
    S0,
    S1,
    S2,
    S3,
    S4
};

typedef struct {
    // Context variable used by zephyr to track state machine state. Must be first
    struct smf_ctx ctx;

    uint16_t count; // counts 1 ms ticks from main's loop
} led_state_object_t;

/*--------------------------------------------------------------------
 * Local Variables
 *--------------------------------------------------------------------*/
static const struct smf_state led_states[] = {
    [S0] = SMF_CREATE_STATE(s0_entry, s0_run, s0_exit, NULL, NULL),
    [S1] = SMF_CREATE_STATE(s1_entry, s1_run, s1_exit, NULL, NULL),
    [S2] = SMF_CREATE_STATE(s2_entry, s2_run, s2_exit, NULL, NULL),
    [S3] = SMF_CREATE_STATE(s3_entry, s3_run, s3_exit, NULL, NULL),
    [S4] = SMF_CREATE_STATE(s4_entry, s4_run, s4_exit, NULL, NULL)
};

static led_state_object_t led_state_object;

/*--------------------------------------------------------------------
 * Public Functions
 *--------------------------------------------------------------------*/
void state_machine_init() {
    led_state_object.count = 0;
    smf_set_initial(SMF_CTX(&led_state_object), &led_states[S0]);
}

int state_machine_run() {
    return smf_run_state(SMF_CTX(&led_state_object));
}

/*--------------------------------------------------------------------
 * Helpers
 *--------------------------------------------------------------------*/
static void all_leds_off(void) {
    for (int i = 0; i < NUM_LEDS; i++) {
        LED_set(i, LED_OFF); // LED_set also stops any blinking
    }
}

// Throw away old presses so a press from a previous state doesn't
// trigger a transition the moment we enter a new state
static void clear_all_buttons(void) {
    for (int i = 0; i < NUM_BTNS; i++) {
        BTN_clear_pressed(i);
    }
}

/*--------------------------------------------------------------------
 * S0: All LEDs off
 *--------------------------------------------------------------------*/
static void s0_entry(void* o) {
    printk("Entering S0\n");
    all_leds_off();
    clear_all_buttons();
}

static enum smf_state_result s0_run(void* o) {
    if (BTN_check_clear_pressed(BTN0)) {            // Button 1
        smf_set_state(SMF_CTX(&led_state_object), &led_states[S1]);
    }
    return SMF_EVENT_HANDLED;
}

static void s0_exit(void* o) {
    printk("Exiting S0\n");
}

/*--------------------------------------------------------------------
 * S1: LED 1 blinks at 4 Hz
 *--------------------------------------------------------------------*/
static void s1_entry(void* o) {
    printk("Entering S1\n");
    all_leds_off();
    LED_blink(LED0, LED_4HZ);
    clear_all_buttons();
}

static enum smf_state_result s1_run(void* o) {
    if (BTN_check_clear_pressed(BTN3)) {            // Button 4
        smf_set_state(SMF_CTX(&led_state_object), &led_states[S0]);
    } else if (BTN_check_clear_pressed(BTN1)) {     // Button 2
        smf_set_state(SMF_CTX(&led_state_object), &led_states[S2]);
    } else if (BTN_check_clear_pressed(BTN2)) {     // Button 3
        smf_set_state(SMF_CTX(&led_state_object), &led_states[S4]);
    }
    return SMF_EVENT_HANDLED;
}

static void s1_exit(void* o) {
    printk("Exiting S1\n");
}

/*--------------------------------------------------------------------
 * S2: LED 1 and 3 on, LED 2 and 4 off -> S3 after 1 s
 *--------------------------------------------------------------------*/
static void s2_entry(void* o) {
    printk("Entering S2\n");
    LED_set(LED0, LED_ON);
    LED_set(LED1, LED_OFF);
    LED_set(LED2, LED_ON);
    LED_set(LED3, LED_OFF);
    led_state_object.count = 0;
    clear_all_buttons();
}

static enum smf_state_result s2_run(void* o) {
    if (BTN_check_clear_pressed(BTN3)) {            // Button 4
        smf_set_state(SMF_CTX(&led_state_object), &led_states[S0]);
    } else if (led_state_object.count >= 1000) {    // 1 s
        smf_set_state(SMF_CTX(&led_state_object), &led_states[S3]);
    } else {
        led_state_object.count++;
    }
    return SMF_EVENT_HANDLED;
}

static void s2_exit(void* o) {
    printk("Exiting S2\n");
}

/*--------------------------------------------------------------------
 * S3: LED 1 and 3 off, LED 2 and 4 on -> S2 after 2 s
 *--------------------------------------------------------------------*/
static void s3_entry(void* o) {
    printk("Entering S3\n");
    LED_set(LED0, LED_OFF);
    LED_set(LED1, LED_ON);
    LED_set(LED2, LED_OFF);
    LED_set(LED3, LED_ON);
    led_state_object.count = 0;
    clear_all_buttons();
}

static enum smf_state_result s3_run(void* o) {
    if (BTN_check_clear_pressed(BTN3)) {            // Button 4
        smf_set_state(SMF_CTX(&led_state_object), &led_states[S0]);
    } else if (led_state_object.count >= 2000) {    // 2 s
        smf_set_state(SMF_CTX(&led_state_object), &led_states[S2]);
    } else {
        led_state_object.count++;
    }
    return SMF_EVENT_HANDLED;
}

static void s3_exit(void* o) {
    printk("Exiting S3\n");
}

/*--------------------------------------------------------------------
 * S4: All LEDs blink at 16 Hz
 *--------------------------------------------------------------------*/
static void s4_entry(void* o) {
    printk("Entering S4\n");
    all_leds_off(); // start them all off so they blink in sync
    for (int i = 0; i < NUM_LEDS; i++) {
        LED_blink(i, LED_16HZ);
    }
    clear_all_buttons();
}

static enum smf_state_result s4_run(void* o) {
    if (BTN_check_clear_pressed(BTN3)) {            // Button 4
        smf_set_state(SMF_CTX(&led_state_object), &led_states[S0]);
    }
    return SMF_EVENT_HANDLED;
}

static void s4_exit(void* o) {
    printk("Exiting S4\n");
}