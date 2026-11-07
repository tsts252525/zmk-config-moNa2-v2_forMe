#define DT_DRV_COMPAT zmk_behavior_jis_layout_toggle

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <drivers/behavior.h>

#include <zmk/behavior.h>

#include "jis_layout_shift.h"

// Order matches the `mode` enum in zmk,behavior-jis-layout-toggle.yaml
enum jis_toggle_mode {
    JIS_TOGGLE_MODE_TOGGLE,
    JIS_TOGGLE_MODE_ON,
    JIS_TOGGLE_MODE_OFF,
};

struct behavior_jis_layout_toggle_config {
    enum jis_toggle_mode mode;
};

static int on_jis_toggle_binding_pressed(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_jis_layout_toggle_config *config = dev->config;

    switch (config->mode) {
    case JIS_TOGGLE_MODE_ON:
        zmk_jis_layout_set_active(true);
        break;
    case JIS_TOGGLE_MODE_OFF:
        zmk_jis_layout_set_active(false);
        break;
    case JIS_TOGGLE_MODE_TOGGLE:
    default:
        zmk_jis_layout_set_active(!zmk_jis_layout_is_active());
        break;
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_jis_toggle_binding_released(struct zmk_behavior_binding *binding,
                                          struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_jis_layout_toggle_driver_api = {
    .binding_pressed = on_jis_toggle_binding_pressed,
    .binding_released = on_jis_toggle_binding_released,
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define JIS_TOGGLE_INST(n)                                                                         \
    static const struct behavior_jis_layout_toggle_config jis_toggle_config_##n = {                \
        .mode = DT_INST_ENUM_IDX(n, mode),                                                         \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &jis_toggle_config_##n, POST_KERNEL,              \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                   \
                            &behavior_jis_layout_toggle_driver_api);

DT_INST_FOREACH_STATUS_OKAY(JIS_TOGGLE_INST)
