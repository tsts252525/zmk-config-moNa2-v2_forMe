#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include "jis_layout_shift.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static bool jis_active = IS_ENABLED(CONFIG_ZMK_JIS_LAYOUT_SHIFT_DEFAULT_ON);

bool zmk_jis_layout_is_active(void) { return jis_active; }

#if IS_ENABLED(CONFIG_ZMK_JIS_LAYOUT_SHIFT_PERSIST)

#define JIS_SETTINGS_KEY "jis_layout/active"

static void jis_save_work_handler(struct k_work *work) {
    uint8_t value = jis_active ? 1 : 0;
    int ret = settings_save_one(JIS_SETTINGS_KEY, &value, sizeof(value));
    if (ret < 0) {
        LOG_ERR("JIS layout: failed to save state: %d", ret);
    }
}

static K_WORK_DELAYABLE_DEFINE(jis_save_work, jis_save_work_handler);

static int jis_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    const char *next;

    if (!settings_name_steq(name, "active", &next) || next) {
        return -ENOENT;
    }
    if (len != sizeof(uint8_t)) {
        return -EINVAL;
    }

    uint8_t value;
    int ret = read_cb(cb_arg, &value, sizeof(value));
    if (ret < 0) {
        return ret;
    }

    jis_active = value != 0;
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(jis_layout, "jis_layout", NULL, jis_settings_set, NULL, NULL);

#endif

void zmk_jis_layout_set_active(bool active) {
    if (jis_active == active) {
        return;
    }

    jis_active = active;
    LOG_INF("JIS layout conversion %s", active ? "ON" : "OFF");

#if IS_ENABLED(CONFIG_ZMK_JIS_LAYOUT_SHIFT_PERSIST)
    k_work_reschedule(&jis_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
#endif
}
