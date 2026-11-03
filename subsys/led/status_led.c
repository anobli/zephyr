/* 
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/led/status_led.h>

int status_led_anim_current_prio(struct status_led *status_led, int *id)
{
	int prio = -ENOENT;

	for (int i = 0; i < status_led->count; i++) {
		if (status_led->actives[i]) {
			prio = MAX(prio, status_led->anims[i].prio);
			if (id) {
				*id = i;
			}
		}
	}

	return prio;
}

static void status_led_anim_completed(void *data)
{
	struct status_led *status_led = data;
	int i;

	status_led_anim_current_prio(status_led, &i);
	status_led_clear(status_led, i);
}

static void status_led_play(struct status_led *status_led, int i)
{
	led_anim_init(&status_led->anim_data, status_led->dev,
			status_led->led, &status_led->anims[i].anim);
	led_anim_play_anim_with_cb(&status_led->anim_data,
					status_led_anim_completed, status_led);
}

int status_led_set(struct status_led *status_led, int id)
{
	int prio;
	
	if (!status_led) {
		return -EINVAL;
	}


	prio = status_led_anim_current_prio(status_led, NULL);
	for (int i = 0; i < status_led->count; i++) {
		if (status_led->anims[i].id == id) {
			if (status_led->anims[i].prio >= prio) {
				status_led_play(status_led, i);
			}
			status_led->actives[i] = true;
		}
	}

	return 0;
}

int status_led_clear(struct status_led *status_led, int id)
{
	int prio;
	int i;

	for (i = 0; i < status_led->count; i++) {
		if (status_led->anims[i].id == id) {
			led_anim_stop_anim(&status_led->anim_data);
			status_led->actives[i] = false;
		}
	}

	prio = status_led_anim_current_prio(status_led, &i);
	if (prio >= 0) {
		status_led_play(status_led, i);
	}

	return 0;
}
