#include QMK_KEYBOARD_H
#if __has_include("keymap.h")
#    include "keymap.h"
#endif
#include <raw_hid.h>
#include "print.h"

#if defined(OS_DETECTION_ENABLE)
    #include "os_detection.h"
#endif

enum custom_keycodes {
    CKC_COPY = SAFE_RANGE,
    CKC_PASTE,
    JIGGLE,
    SMTD_KEYCODES_BEGIN,
    CKC_Z, // reads as C(ustom) + KC_A
    CKC_X,
    CKC_C,
    CKC_D,
    CKC_H,
    CKC_COMM,
    CKC_DOT,
    CKC_SLSH,
    CKC_CLGV,
    CKC_GUTA,
    SMTD_KEYCODES_END,
};

#include "sm_td/sm_td.h"

#define MEH_SPACE MT(MOD_MEH, KC_SPACE)
// alternative
// MEH_T(kc)		Left Control, Shift and Alt when held, kc when tapped

// Layer 1: SMTD_LT(CKC_CLGV, KC_GRV, 1)
// Layer 2: SMTD_LT(CKC_GUTA, KC_TAB, 2)

// Some helper C macros
    #define GENERAL_MODIFIER_KEY_DELAY_MS 20
    #define GENERAL_KEY_ACTION_DELAY_MS   50

    #define KEY_MODIFIER_ACTION(keycode, modifier) \
        SS_DOWN(modifier) \
        SS_DELAY(GENERAL_MODIFIER_KEY_DELAY_MS) \
        SS_TAP(keycode) \
        SS_DELAY(GENERAL_KEY_ACTION_DELAY_MS) \
        SS_UP(modifier) \
        SS_DELAY(GENERAL_MODIFIER_KEY_DELAY_MS)

    #define KEY_CTRL_ACTION(keycode) \
        KEY_MODIFIER_ACTION(keycode,X_LCTL)

    #define KEY_APPLE_KEY_ACTION(keycode) \
        KEY_MODIFIER_ACTION(keycode,X_LCMD)


#define KC_CTSC RCTL_T(KC_SCLN)
#define KC_CTLA LCTL_T(KC_A)
#define KC_LSHZ LSFT_T(KC_Z)
#define KC_RLSH RSFT_T(KC_SLSH)
#define KC_SPM2 LT(2, KC_SPC)
#define KC_BSM1 LT(1, KC_BSPC)
#define KC_GUTA GUI_T(KC_TAB)
#define KC_CLGV CTL_T(KC_GRV)

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT_split_3x5_2(
        KC_Q,    KC_W,    KC_F,    KC_P,    KC_B,            KC_J,    KC_L,  KC_U,    KC_Y,   KC_SCLN,
        KC_A,    KC_R,    KC_S,    KC_T,    KC_G,            KC_M,    KC_N,  KC_E,    KC_I,   KC_O,
        CKC_Z,   CKC_X,   CKC_C,   CKC_D,    KC_V,           KC_K,    CKC_H, CKC_COMM,CKC_DOT,CKC_SLSH,
                                    CKC_CLGV, KC_BSPC,  MEH_SPACE, CKC_GUTA
    ),
    [1] = LAYOUT_split_3x5_2(
        KC_1,    KC_2,    KC_3,    KC_4,    KC_5,            KC_6,    KC_7,    KC_8,    KC_9,    KC_0,
        QK_GESC, KC_HOME, KC_PGDN, KC_PGUP, KC_END,          KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT, KC_QUOT,
        JIGGLE, KC_TRNS, CKC_COPY, KC_BTN1, CKC_PASTE,         KC_MS_L, KC_MS_D, KC_MS_U, KC_MS_R, KC_ENT,
                                    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
    [2] = LAYOUT_split_3x5_2(
        KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,           KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,
        KC_TAB,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,         KC_MINS, KC_EQL,  KC_LBRC, KC_RBRC, KC_PIPE,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,         KC_UNDS, KC_PLUS, KC_TRNS, KC_TRNS, QK_BOOT,
                                    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
    [3] = LAYOUT_split_3x5_2(
        KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,            KC_Y,    KC_U,  KC_I,    KC_O,   KC_P,
        KC_CTLA, KC_S,    KC_D,    KC_F,    KC_G,            KC_H,    KC_J,  KC_K,    KC_L,   KC_SCLN,
        KC_LSHZ, KC_X,    KC_C,    KC_V,    KC_B,            KC_N,    KC_M,  KC_COMM, KC_DOT, KC_RLSH,
                                    KC_CLGV, KC_BSM1, KC_SPM2, KC_GUTA

    )
};

#ifdef OTHER_KEYMAP_C
#    include OTHER_KEYMAP_C
#endif // OTHER_KEYMAP_C

bool process_record_user(uint16_t keycode, keyrecord_t *record) {

    if (record->event.pressed) {
        static deferred_token token = INVALID_DEFERRED_TOKEN;
        static report_mouse_t report = {0};
        if (token) {
            // If jiggler is currently running, stop when any key is pressed.
            cancel_deferred_exec(token);
            token = INVALID_DEFERRED_TOKEN;
            report = (report_mouse_t){};  // Clear the mouse.
            host_mouse_send(&report);

        } else if (keycode == JIGGLE) {

            uint32_t jiggler_callback(uint32_t trigger_time, void* cb_arg) {
                // Deltas to move in a circle of radius 20 pixels over 32 frames.
                static const int8_t deltas[32] = {
                    0, -1, -2, -2, -3, -3, -4, -4, -4, -4, -3, -3, -2, -2, -1, 0,
                    0, 1, 2, 2, 3, 3, 4, 4, 4, 4, 3, 3, 2, 2, 1, 0};
                static uint8_t phase = 0;
                // Get x delta from table and y delta by rotating a quarter cycle.
                report.x = deltas[phase];
                report.y = deltas[(phase + 8) & 31];
                phase = (phase + 1) & 31;
                host_mouse_send(&report);
                return 16;  // Call the callback every 16 ms.
            }
            token = defer_exec(1, jiggler_callback, NULL);  // Schedule callback.
        }
    }

    if (!process_smtd(keycode, record)) {
        return false;
    }
    // your code here
    // QMK: shift + backspace = delete
    switch (keycode) {
        case CKC_COPY:
            if (record->event.pressed) {
                #if defined(OS_DETECTION_ENABLE)
                os_variant_t host = detected_host_os();
                if (host == OS_MACOS || host == OS_IOS) {
                    // Mac: Cmd + C
                    SEND_STRING(KEY_APPLE_KEY_ACTION(X_C));
                }
                else {
                    // Linux, Windows, etc.: Ctrl + C
                    SEND_STRING(KEY_CTRL_ACTION(X_C));
                }
                #endif
            }
            break;
        case CKC_PASTE:
            if (record->event.pressed) {
                #if defined(OS_DETECTION_ENABLE)
                os_variant_t host = detected_host_os();
                if (host == OS_MACOS || host == OS_IOS) {
                    // Mac: Cmd + V
                    SEND_STRING(KEY_APPLE_KEY_ACTION(X_V));
                }
                else {
                    // Linux, Windows, etc.: Ctrl + V
                    SEND_STRING(KEY_CTRL_ACTION(X_V));
                }
                #endif
            }
            break;
        case KC_BSPC: {
            static uint16_t registered_key = KC_NO;
            if (record->event.pressed) {  // On key press.
                const uint8_t mods = get_mods();
#ifndef NO_ACTION_ONESHOT
                uint8_t shift_mods = (mods | get_oneshot_mods()) & MOD_MASK_SHIFT;
#else
                uint8_t shift_mods = mods & MOD_MASK_SHIFT;
#endif          // NO_ACTION_ONESHOT
                if (shift_mods) {  // At least one shift key is held.
                    registered_key = KC_DEL;
                // If one shift is held, clear it from the mods. But if both
                // shifts are held, leave as is to send Shift + Del.
                if (shift_mods != MOD_MASK_SHIFT) {
#ifndef NO_ACTION_ONESHOT
                    del_oneshot_mods(MOD_MASK_SHIFT);
#endif  // NO_ACTION_ONESHOT
                    unregister_mods(MOD_MASK_SHIFT);
                }
            } else {
                registered_key = KC_BSPC;
            }

            register_code(registered_key);
            set_mods(mods);
        } else {  // On key release.
            unregister_code(registered_key);
        }
      } return false;

    // Other macros...
    }
    return true;  // Default return value if process_smtd returns true
}

void on_smtd_action(uint16_t keycode, smtd_action action, uint8_t tap_count) {
    switch (keycode) {
        SMTD_MT(CKC_Z, KC_Z, KC_LEFT_GUI)
        SMTD_MT(CKC_X, KC_X, KC_LEFT_ALT)
        SMTD_MT(CKC_C, KC_C, KC_LEFT_CTRL)
        SMTD_MT(CKC_D, KC_D, KC_LSFT)

        SMTD_MT(CKC_SLSH, KC_SLSH, KC_LEFT_GUI)
        SMTD_MT(CKC_DOT, KC_DOT, KC_LEFT_ALT)
        SMTD_MT(CKC_COMM, KC_COMM, KC_LEFT_CTRL)
        SMTD_MT(CKC_H, KC_H, KC_LSFT)

        SMTD_LT(CKC_CLGV, KC_GRV, 1)
        SMTD_LT(CKC_GUTA, KC_TAB, 2)


        // SMTD_MT(CKC_SPACE, KC_SPACE, MOD_MEH)
    }
}

void keyboard_post_init_user(void) {
  // Customise these values to desired behaviour
  debug_enable=true;
  debug_matrix=true;
  debug_keyboard=true;
  //debug_mouse=true;
}

void raw_hid_receive(uint8_t *data, uint8_t length) {
    dprintf("\nReceived raw HID packet (length=%d):\n", length);
    // Print every byte received
    for (uint8_t i = 0; i < length; i++) {
        dprintf("data[%d] = 0x%02X\n", i, data[i]);
    }

    dprintf("Current layer state: 0x%08X\n", (unsigned int)layer_state);
    dprintf("Highest active layer: %d\n", get_highest_layer(layer_state));

    uint8_t command = data[0];  // Command is in first byte
    uint8_t layer = data[1];    // Layer is in second byte

    switch(command) {
        case 0x03: {  // Layer switch command
            dprintf("Command: Layer switch (0x00) to layer %d\n", layer);

            if (layer <= 3) {
                layer_clear();  // Clear all layers first
                layer_move(0);  // Force the layer to 0 (colemak-dh)

                uint8_t current = get_highest_layer(layer_state);
                dprintf("New layer: %d\n", current);

                // Must clear data buffer before setting response
                memset(data, 0, length);
                data[0] = 0x00;        // Success
                data[1] = current;      // Current layer
                data[2] = 0xAA;        // Acknowledgment

                dprintf("Layer switch successful\n");
            } else {
                dprintf("Invalid layer %d requested\n", layer);
                memset(data, 0, length);
                data[0] = 0xFF;  // Error
            }

            dprintf("Sending response\n");
            raw_hid_send(data, length);
            break;
        }

        default: {
            dprintf("Switching to qwerty\n");

            layer_clear();  // Clear all layers first
            layer_move(3);  // Force the layer to 3 (qwerty)

            uint8_t current = get_highest_layer(layer_state);
            dprintf("New layer: %d\n", current);

            memset(data, 0, length);
            data[0] = 0xFF;
            raw_hid_send(data, length);
            break;
        }
    }
}

// compile
// qmk compile -c -kb ferris/sweep -km default -e CONVERT_TO=rp2040_ce
// git clone --recursive https://github.com/morphykuffour/qmk_firmware.git

// features
// https://www.monotux.tech/posts/2024/05/qmk-os-detection/
// https://getreuer.info/posts/keyboards/macros3/index.html#a-mouse-jiggler
// https://github.com/stasmarkin/sm_td.git
// https://www.reddit.com/r/qmk/comments/1i1uik2/getting_os_detection_into_a_macro/?share_id=jwGzhv4UoJQ-W5FqVNdVc&utm_content=1&utm_medium=ios_app&utm_name=ioscss&utm_source=share&utm_term=1
// https://docs.qmk.fm/features/rawhid
