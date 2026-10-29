/* 
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_LED_LED_PLAYER_H_
#define ZEPHYR_INCLUDE_LED_LED_PLAYER_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

struct led_anim_frame {
	uint16_t delay_ms;
	bool state;
};

struct led_anim {
	struct led_anim_frame *frames;
	int count;
	int loop;
};

typedef void (*led_anim_complete_cb)(void *user_data);

struct led_anim_data {
	const struct device *dev;
	uint32_t led;

	struct led_anim *anim;
	int current_frame;
	int remaining_loop;
	uint32_t deadline;

	led_anim_complete_cb cb;
	void *user_data;	

	sys_snode_t node;
};

struct led_anim_player {
	struct k_work_delayable dwork;

	sys_slist_t list;
	uint32_t scheduled_deadline;
	struct led_anim_data *scheduled_data;
	struct k_mutex mutex;
};

uint32_t led_anim_get_next_anim_deadline(struct led_anim_player *player, struct led_anim_data **data);
int led_anim_schedule_next(struct led_anim_player *player);

int led_anim_play_frame(struct led_anim_data *data);
int led_anim_init(struct led_anim_data *data, const struct device *dev, uint32_t led, struct led_anim *anim);
int led_anim_player_do_init(struct led_anim_player *player);
 
int led_anim_play_anim(struct led_anim_data *data);
int led_anim_play_anim_with_cb(struct led_anim_data *data, led_anim_complete_cb cb, void *user_data);
int led_anim_stop_anim(struct led_anim_data *data);


#endif /* ZEPHYR_INCLUDE_LED_LED_PLAYER_H_ */
