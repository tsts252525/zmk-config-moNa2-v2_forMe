/*
 * xy-block input processor: drops pointer REL_X / REL_Y events so the mouse
 * cursor stays put. Used on the ball-gesture layers so that rolling the ball
 * to draw a gesture does not move the cursor (scroll/wheel and buttons pass
 * through untouched). Placed AFTER the mouse-gesture processor in the
 * listener override so the gesture is still recognised first.
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_xy_block

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <drivers/input_processor.h>

static int xy_block_handle_event(const struct device *dev, struct input_event *event,
                                 uint32_t param1, uint32_t param2,
                                 struct zmk_input_processor_state *state) {
    ARG_UNUSED(dev);
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);
    ARG_UNUSED(state);

    if (event->type == INPUT_EV_REL &&
        (event->code == INPUT_REL_X || event->code == INPUT_REL_Y)) {
        return ZMK_INPUT_PROC_STOP;
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static const struct zmk_input_processor_driver_api xy_block_driver_api = {
    .handle_event = xy_block_handle_event,
};

static int xy_block_init(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

#define XY_BLOCK_INST(n)                                                                            \
    DEVICE_DT_INST_DEFINE(n, xy_block_init, NULL, NULL, NULL, POST_KERNEL,                          \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &xy_block_driver_api);
DT_INST_FOREACH_STATUS_OKAY(XY_BLOCK_INST)
