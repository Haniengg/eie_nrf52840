/**
 * @file my_state_machine.c
 *
 * Lesson 5 Exercise 3: Lesson 4 password lock rewritten as a state machine
 *
 * Buttons/LEDs use code names: BTN0-BTN3 = board Button 1-4, LED0-LED3 = board LED1-4
 *
 * STARTUP      LED3 on for 3 s. Press BTN3 to set a new password, otherwise
 *              go to LOCKED with the default password.
 * SET_PASSWORD Enter a sequence with BTN0/BTN1/BTN2, press BTN3 to save it.
 * LOCKED       LED0 on. Enter a sequence with BTN0/BTN1/BTN2, press BTN3 to check.
 *              Prints "Correct!" or "Incorrect!".
 * WAITING      LED0 off. Press any button to go back to LOCKED.
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/smf.h>
#include <zephyr/sys/printk.h>

#include "BTN.h"
#include "LED.h"
#include "my_state_machine.h"

/*--------------------------------------------------------------------
 * Constants
 *--------------------------------------------------------------------*/
#define MAX_PASSWORD_LEN  8
#define STARTUP_TIME_MS   3000

/*--------------------------------------------------------------------
 * Function Prototypes
 *--------------------------------------------------------------------*/
static void startup_entry(void* o);
static enum smf_state_result startup_run(void* o);
static void startup_exit(void* o);

static void set_password_entry(void* o);
static enum smf_state_result set_password_run(void* o);

static void locked_entry(void* o);
static enum smf_state_result locked_run(void* o);

static void waiting_entry(void* o);
static enum smf_state_result waiting_run(void* o);

static void clear_all_buttons(void);
static void read_digit_buttons(void);

/*--------------------------------------------------------------------
 * Typedefs
 *--------------------------------------------------------------------*/
enum lock_states {
    STARTUP,
    SET_PASSWORD,
    LOCKED,
    WAITING
};

typedef struct {
    // Context variable used by zephyr to track state machine state. Must be first
    struct smf_ctx ctx;

    uint16_t count; // 1 ms ticks, used for the startup timer

    btn_id password[MAX_PASSWORD_LEN];
    uint8_t password_len;

    btn_id entered[MAX_PASSWORD_LEN];
    uint8_t entered_len;
    bool too_long; // more presses than MAX_PASSWORD_LEN
} lock_state_object_t;

/*--------------------------------------------------------------------
 * Local Variables
 *--------------------------------------------------------------------*/
static const struct smf_state lock_states[] = {
    [STARTUP]      = SMF_CREATE_STATE(startup_entry, startup_run, startup_exit, NULL, NULL),
    [SET_PASSWORD] = SMF_CREATE_STATE(set_password_entry, set_password_run, NULL, NULL, NULL),
    [LOCKED]       = SMF_CREATE_STATE(locked_entry, locked_run, NULL, NULL, NULL),
    [WAITING]      = SMF_CREATE_STATE(waiting_entry, waiting_run, NULL, NULL, NULL)
};

static lock_state_object_t lock_object;

/*--------------------------------------------------------------------
 * Public Functions
 *--------------------------------------------------------------------*/
void state_machine_init() {
    // Default password (used if the user doesn't set one at startup): BTN0, BTN1, BTN2
    lock_object.password[0] = BTN0;
    lock_object.password[1] = BTN1;
    lock_object.password[2] = BTN2;
    lock_object.password_len = 3;

    smf_set_initial(SMF_CTX(&lock_object), &lock_states[STARTUP]);
}

int state_machine_run() {
    return smf_run_state(SMF_CTX(&lock_object));
}

/*--------------------------------------------------------------------
 * Helpers
 *--------------------------------------------------------------------*/
static void clear_all_buttons(void) {
    for (int i = 0; i < NUM_BTNS; i++) {
        BTN_clear_pressed(i);
    }
}

// Clear the sequence the user is typing
static void reset_entry(void) {
    lock_object.entered_len = 0;
    lock_object.too_long = false;
}

// Record any BTN0/BTN1/BTN2 presses into the entered sequence
static void read_digit_buttons(void) {
    for (btn_id b = BTN0; b <= BTN2; b++) {
        if (BTN_check_clear_pressed(b)) {
            if (lock_object.entered_len < MAX_PASSWORD_LEN) {
                lock_object.entered[lock_object.entered_len++] = b;
            } else {
                lock_object.too_long = true;
            }
        }
    }
}

static bool entered_matches_password(void) {
    if (lock_object.too_long || lock_object.entered_len != lock_object.password_len) {
        return false;
    }
    for (int i = 0; i < lock_object.password_len; i++) {
        if (lock_object.entered[i] != lock_object.password[i]) {
            return false;
        }
    }
    return true;
}

/*--------------------------------------------------------------------
 * STARTUP: LED3 on for 3 s, BTN3 -> SET_PASSWORD, timeout -> LOCKED
 *--------------------------------------------------------------------*/
static void startup_entry(void* o) {
    printk("Startup: press BTN3 within 3 s to set a new password\n");
    LED_set(LED3, LED_ON);
    lock_object.count = 0;
    clear_all_buttons();
}

static enum smf_state_result startup_run(void* o) {
    if (BTN_check_clear_pressed(BTN3)) {
        smf_set_state(SMF_CTX(&lock_object), &lock_states[SET_PASSWORD]);
    } else if (lock_object.count >= STARTUP_TIME_MS) {
        smf_set_state(SMF_CTX(&lock_object), &lock_states[LOCKED]);
    } else {
        lock_object.count++;
    }
    return SMF_EVENT_HANDLED;
}

static void startup_exit(void* o) {
    LED_set(LED3, LED_OFF);
}

/*--------------------------------------------------------------------
 * SET_PASSWORD: enter a sequence with BTN0-2, BTN3 saves it
 *--------------------------------------------------------------------*/
static void set_password_entry(void* o) {
    printk("Enter a new password with BTN0/BTN1/BTN2, then press BTN3 to save\n");
    LED_blink(LED3, LED_2HZ); // blinking LED3 = entry mode
    reset_entry();
    clear_all_buttons();
}

static enum smf_state_result set_password_run(void* o) {
    read_digit_buttons();

    if (BTN_check_clear_pressed(BTN3)) {
        if (lock_object.entered_len == 0 || lock_object.too_long) {
            printk("Password must be 1-%d presses, try again\n", MAX_PASSWORD_LEN);
            reset_entry();
        } else {
            for (int i = 0; i < lock_object.entered_len; i++) {
                lock_object.password[i] = lock_object.entered[i];
            }
            lock_object.password_len = lock_object.entered_len;
            printk("Password saved (%d presses)\n", lock_object.password_len);
            LED_set(LED3, LED_OFF); // stops the blinking
            smf_set_state(SMF_CTX(&lock_object), &lock_states[LOCKED]);
        }
    }
    return SMF_EVENT_HANDLED;
}

/*--------------------------------------------------------------------
 * LOCKED: LED0 on, enter a sequence, BTN3 checks it
 *--------------------------------------------------------------------*/
static void locked_entry(void* o) {
    printk("Locked: enter password, then press BTN3\n");
    LED_set(LED0, LED_ON);
    reset_entry();
    clear_all_buttons();
}

static enum smf_state_result locked_run(void* o) {
    read_digit_buttons();

    if (BTN_check_clear_pressed(BTN3)) {
        if (entered_matches_password()) {
            printk("Correct!\n");
        } else {
            printk("Incorrect!\n");
        }
        smf_set_state(SMF_CTX(&lock_object), &lock_states[WAITING]);
    }
    return SMF_EVENT_HANDLED;
}

/*--------------------------------------------------------------------
 * WAITING: LED0 off, any button -> LOCKED
 *--------------------------------------------------------------------*/
static void waiting_entry(void* o) {
    printk("Press any button to reset\n");
    LED_set(LED0, LED_OFF);
    clear_all_buttons();
}

static enum smf_state_result waiting_run(void* o) {
    for (btn_id b = BTN0; b < NUM_BTNS; b++) {
        if (BTN_check_clear_pressed(b)) {
            smf_set_state(SMF_CTX(&lock_object), &lock_states[LOCKED]);
            break;
        }
    }
    return SMF_EVENT_HANDLED;
}