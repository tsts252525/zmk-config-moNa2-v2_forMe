/*
 * Alt+Tab window switcher behaviors for ball gestures.
 * (torabo-tsuki の src/behavior_alt_tab.c を moNa2 へ移植。vendor prefix のみ
 *  "torabo," → "zmk," に変更。ロジックは同一。)
 *
 * &alt_tab OPEN_KEY NEXT_KEY
 *   First invocation presses and keeps holding Left Alt, then taps OPEN_KEY
 *   (e.g. TAB) to open the switcher. While Alt is still held, later
 *   invocations only tap NEXT_KEY (e.g. RIGHT) to move the selection.
 *
 * &alt_tab_end
 *   Releases Left Alt if it is held by &alt_tab (commits the selection).
 *
 * Alt is tracked here so it is pressed at most once, keeping ZMK's modifier
 * count balanced no matter how many gestures were performed.
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <dt-bindings/zmk/keys.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static bool alt_held;

static void tap_key(uint32_t keycode, int64_t timestamp) {
    raise_zmk_keycode_state_changed_from_encoded(keycode, true, timestamp);
    raise_zmk_keycode_state_changed_from_encoded(keycode, false, timestamp);
}

static int alt_tab_pressed(struct zmk_behavior_binding *binding,
                           struct zmk_behavior_binding_event event) {
    if (!alt_held) {
        raise_zmk_keycode_state_changed_from_encoded(LALT, true, event.timestamp);
        alt_held = true;
        tap_key(binding->param1, event.timestamp);
    } else {
        tap_key(binding->param2, event.timestamp);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int alt_tab_end_pressed(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {
    if (alt_held) {
        alt_held = false;
        raise_zmk_keycode_state_changed_from_encoded(LALT, false, event.timestamp);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int noop_released(struct zmk_behavior_binding *binding,
                         struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api alt_tab_driver_api = {
    .binding_pressed = alt_tab_pressed,
    .binding_released = noop_released,
};

static const struct behavior_driver_api alt_tab_end_driver_api = {
    .binding_pressed = alt_tab_end_pressed,
    .binding_released = noop_released,
};

static int alt_tab_init(const struct device *dev) { return 0; }

#define DT_DRV_COMPAT zmk_behavior_alt_tab
#define ALT_TAB_INST(n)                                                                            \
    BEHAVIOR_DT_INST_DEFINE(n, alt_tab_init, NULL, NULL, NULL, POST_KERNEL,                        \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &alt_tab_driver_api);
DT_INST_FOREACH_STATUS_OKAY(ALT_TAB_INST)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT zmk_behavior_alt_tab_end
#define ALT_TAB_END_INST(n)                                                                        \
    BEHAVIOR_DT_INST_DEFINE(n, alt_tab_init, NULL, NULL, NULL, POST_KERNEL,                        \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &alt_tab_end_driver_api);
DT_INST_FOREACH_STATUS_OKAY(ALT_TAB_END_INST)
#undef DT_DRV_COMPAT
