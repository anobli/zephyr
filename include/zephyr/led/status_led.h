/* 
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_LED_STATUS_LED_H_
#define ZEPHYR_INCLUDE_LED_STATUS_LED_H_

#include <zephyr/led/led_player.h>

struct status_led_anim {
	int id;
	int prio;
	struct led_anim anim;
};

struct status_led {
	const struct device *dev;
	uint32_t led;

	struct led_anim_data anim_data;
	struct status_led_anim *anims;
	bool *actives;
	int count;
};

#define LED_ANIM(_frame) \


#define LED_ANIM_BLINK_FRAMES(_on_ms, _off_ms)	\
((struct led_anim_frame []) {			\
	{					\
		.delay_ms = _on_ms,		\
		.state = 1,			\
	},					\
	{					\
		.delay_ms = _off_ms,		\
		.state = 0,			\
	}					\
})

#define STATUS_LED_BLINK_ANIM(_id, _prio, _on_ms, _off_ms)			\
{										\
	.id = _id,								\
	.prio = _prio,								\
	.anim = {								\
		.frames = LED_ANIM_BLINK_FRAMES(_on_ms, _off_ms),		\
		.count = ARRAY_SIZE(LED_ANIM_BLINK_FRAMES(_on_ms, _off_ms)),	\
		.loop = 0,							\
	},									\
}

#define STATUS_LED_DEFINE(_name, _dev, _led, _anims)		\
static bool _name ## actives[ARRAY_SIZE(_anims)];		\
static struct status_led _name = {				\
	.dev = _dev, 						\
	.led = _led, 						\
	.anims = _anims, 					\
	.actives =  _name ## actives, 				\
	.count = ARRAY_SIZE(_anims), 				\
};

int status_led_set(struct status_led *status_led, int id);
int status_led_clear(struct status_led *status_led, int id);


#endif /* ZEPHYR_INCLUDE_LED_STATUS_LED_H_ */
