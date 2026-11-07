/*
 * Layout-aware replacement for &kp.
 *
 * JIS mode off: identical to ZMK's behavior_key_press.
 * JIS mode on : US-layout symbol keycodes are rewritten to the keycodes that
 *               produce the same symbol on a JIS-layout OS. Shift that the
 *               user holds is masked when the JIS key must be sent unshifted
 *               (e.g. Shift+2 "@" -> JIS "@" key without Shift).
 *
 * The conversion table follows kot149/zmk-layout-shift (MIT).
 */

#define DT_DRV_COMPAT zmk_behavior_jis_key_press

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>

#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keys.h>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/modifiers.h>
#include <dt-bindings/zmk/hid_usage.h>
#include <dt-bindings/zmk/hid_usage_pages.h>

#include "jis_layout_shift.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define SHIFT_MODS (MOD_LSFT | MOD_RSFT)

struct jis_map_entry {
    uint32_t from; // US keycode (with Shift when the symbol is a shifted one)
    uint32_t to;   // keycode that yields the same symbol on a JIS-layout OS
};

static const struct jis_map_entry us_to_jis[] = {
    {EQUAL, UNDERSCORE},                    // =
    {CARET, EQUAL},                         // ^
    {TILDE, PLUS},                          // ~
    {AT_SIGN, LEFT_BRACKET},                // @
    {GRAVE, LEFT_BRACE},                    // `
    {LEFT_BRACKET, RIGHT_BRACKET},          // [
    {RIGHT_BRACKET, BACKSLASH},             // ]
    {LEFT_BRACE, RIGHT_BRACE},              // {
    {RIGHT_BRACE, PIPE},                    // }
    {PLUS, COLON},                          // +
    {COLON, SINGLE_QUOTE},                  // :
    {ASTERISK, DOUBLE_QUOTES},              // *
    {DOUBLE_QUOTES, AT_SIGN},               // "
    {AMPERSAND, CARET},                     // &
    {SINGLE_QUOTE, AMPERSAND},              // '
    {LEFT_PARENTHESIS, ASTERISK},           // (
    {RIGHT_PARENTHESIS, LEFT_PARENTHESIS},  // )
    {UNDERSCORE, LS(INTERNATIONAL_1)},      // _
    {BACKSLASH, INTERNATIONAL_3},           // backslash
    {PIPE, LS(INTERNATIONAL_3)},            // |
};

static uint32_t normalize_base(uint32_t encoded) {
    uint32_t base = STRIP_MODS(encoded);
    if (ZMK_HID_USAGE_PAGE(base) == 0) {
        base = ZMK_HID_USAGE(HID_USAGE_KEY, ZMK_HID_USAGE_ID(base));
    }
    return base;
}

/* Returns true and fills mapped/mask when the keycode needs conversion. */
static bool jis_lookup(uint32_t encoded, uint32_t *mapped, zmk_mod_flags_t *mask) {
    zmk_mod_flags_t binding_mods = SELECT_MODS(encoded);
    zmk_mod_flags_t explicit_mods = zmk_hid_get_explicit_mods();
    bool shifted = ((explicit_mods | binding_mods) & SHIFT_MODS) != 0;
    uint32_t base = normalize_base(encoded);

    for (size_t i = 0; i < ARRAY_SIZE(us_to_jis); i++) {
        const struct jis_map_entry *entry = &us_to_jis[i];
        if (STRIP_MODS(entry->from) != base) {
            continue;
        }
        if (((SELECT_MODS(entry->from) & SHIFT_MODS) != 0) != shifted) {
            continue;
        }

        zmk_mod_flags_t to_mods = SELECT_MODS(entry->to);
        // Keep the binding's own Ctrl/Alt/GUI (e.g. LC(EQUAL)); Shift is decided by the table.
        zmk_mod_flags_t out_mods = to_mods | (binding_mods & ~SHIFT_MODS);

        *mapped = ((uint32_t)out_mods << 24) | STRIP_MODS(entry->to);
        *mask = (to_mods & SHIFT_MODS) ? 0 : (explicit_mods & SHIFT_MODS);
        return true;
    }

    return false;
}

struct held_key {
    uint32_t position;
    uint32_t original;
    uint32_t mapped;
    zmk_mod_flags_t mask;
};

static struct held_key held_keys[CONFIG_ZMK_JIS_LAYOUT_SHIFT_MAX_HELD];
static uint8_t held_count;
static zmk_mod_flags_t applied_mask;

static void update_mask(void) {
    zmk_mod_flags_t mask = 0;
    for (uint8_t i = 0; i < held_count; i++) {
        mask |= held_keys[i].mask;
    }
    if (mask == applied_mask) {
        return;
    }

    if (mask == 0) {
        zmk_hid_masked_modifiers_clear();
    } else {
        zmk_hid_masked_modifiers_set(mask);
    }
    applied_mask = mask;
}

static int on_jis_key_press_binding_pressed(struct zmk_behavior_binding *binding,
                                            struct zmk_behavior_binding_event event) {
    uint32_t keycode = binding->param1;
    uint32_t mapped;
    zmk_mod_flags_t mask;

    if (zmk_jis_layout_is_active() && jis_lookup(keycode, &mapped, &mask)) {
        if (held_count < ARRAY_SIZE(held_keys)) {
            held_keys[held_count++] = (struct held_key){
                .position = event.position,
                .original = keycode,
                .mapped = mapped,
                .mask = mask,
            };
            update_mask();
            LOG_DBG("JIS: position %d 0x%08X -> 0x%08X", event.position, keycode, mapped);
            keycode = mapped;
        } else {
            LOG_WRN("JIS: too many held keys, sending 0x%08X unconverted", keycode);
        }
    }

    return raise_zmk_keycode_state_changed_from_encoded(keycode, true, event.timestamp);
}

static int on_jis_key_press_binding_released(struct zmk_behavior_binding *binding,
                                             struct zmk_behavior_binding_event event) {
    uint32_t keycode = binding->param1;

    // Release exactly what was pressed, even if JIS mode was toggled meanwhile.
    for (uint8_t i = 0; i < held_count; i++) {
        if (held_keys[i].position == event.position && held_keys[i].original == keycode) {
            keycode = held_keys[i].mapped;
            held_keys[i] = held_keys[--held_count];
            update_mask();
            break;
        }
    }

    return raise_zmk_keycode_state_changed_from_encoded(keycode, false, event.timestamp);
}

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

// Same metadata as ZMK's &kp so Studio shows the usual key picker.
static const struct behavior_parameter_value_metadata param_values[] = {
    {
        .display_name = "Key",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_HID_USAGE,
    },
};

static const struct behavior_parameter_metadata_set param_metadata_set[] = {{
    .param1_values = param_values,
    .param1_values_len = ARRAY_SIZE(param_values),
}};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = ARRAY_SIZE(param_metadata_set),
    .sets = param_metadata_set,
};

#endif

static const struct behavior_driver_api behavior_jis_key_press_driver_api = {
    .binding_pressed = on_jis_key_press_binding_pressed,
    .binding_released = on_jis_key_press_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif
};

#define JIS_KP_INST(n)                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                   \
                            &behavior_jis_key_press_driver_api);

DT_INST_FOREACH_STATUS_OKAY(JIS_KP_INST)
