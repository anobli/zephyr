/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(coap);

#include <openthread/thread.h>
#include <coap_utils.h>

#ifdef CONFIG_OT_COAP_SAMPLE_LED
#include "led.h"
#endif /* CONFIG_OT_COAP_SAMPLE_LED */

#ifdef CONFIG_OT_COAP_SAMPLE_SW
#include <zephyr/drivers/gpio.h>
#include "button.h"

#ifdef CONFIG_OT_COAP_SAMPLE_SRP
#include "srp.h"
#endif

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_callback button_cb_data;

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	coap_led_set_state("ff03::1", 0, LED_MSG_STATE_TOGGLE);
}
#endif /* CONFIG_OT_COAP_SAMPLE_SW */

static void on_thread_state_changed(otChangedFlags flags,
									struct openthread_context *ctx,
									void *data)
{
	if (flags & OT_CHANGED_THREAD_ROLE) {
		switch(otThreadGetDeviceRole(ctx->instance)) {
			case OT_DEVICE_ROLE_CHILD:
			case OT_DEVICE_ROLE_ROUTER:
			case OT_DEVICE_ROLE_LEADER:
			LOG_INF("Connected to a thread network!");
#ifdef CONFIG_OT_COAP_SAMPLE_SRP
				ot_srp_init();
				break;
#endif
			default:
			LOG_INF("Not connected to a thread network!");
				break;
		}
	}
}

static struct openthread_state_changed_cb ot_state_changed_cb = {
	.state_changed_cb = on_thread_state_changed,
};

int main(void)
{
	int ret;

	openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_changed_cb);

#ifdef CONFIG_OT_COAP_SAMPLE_SERVER
#ifdef CONFIG_OT_COAP_SAMPLE_LED
	coap_led_reg_rsc();
#endif /* CONFIG_OT_COAP_SAMPLE_LED */
#ifdef CONFIG_OT_COAP_SAMPLE_SW
	coap_btn_reg_rsc();
#endif /* CONFIG_OT_COAP_SAMPLE_SW */
#endif /* CONFIG_OT_COAP_SAMPLE_SERVER */

	ret = coap_init();
	if (ret) {
		return ret;
	}

#ifdef CONFIG_OT_COAP_SAMPLE_SW
	button_init(&button);

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback_dt(&button, &button_cb_data);
#endif /*CONFIG_OT_COAP_SAMPLE_SW */

	return 0;
}
