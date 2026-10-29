/* 
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/led/led_player.h>
#include <zephyr/drivers/led.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_player, LOG_LEVEL_INF);

struct led_anim_player g_player;

uint32_t led_anim_get_next_anim_deadline(struct led_anim_player *player, struct led_anim_data **data)
{
	struct led_anim_data *p_data, *s_data = NULL;
	uint32_t deadline = 0xffffffff;

	if (!player || sys_slist_is_empty(&player->list) || !data) {
		return 0;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&player->list, p_data, node) {
		/* Skip animation that are not running */
		if (!p_data->deadline) {
			continue;
		}

		if (p_data->deadline < deadline) {
			deadline = p_data->deadline;
			s_data = p_data;
		}
	}

	*data = s_data;
	return deadline;
}

int led_anim_play_frame(struct led_anim_data *data)
{
	struct led_anim_frame *frame;

	if (!data) {
		return -EINVAL;
	}

	if (data->current_frame >= data->anim->count) {
		if (data->remaining_loop || !data->anim->loop) {
			data->remaining_loop--;
			data->current_frame = 0;
		} else {
			if (data->cb) {
				data->cb(data->user_data);
			}
			data->deadline = 0;
			return -ENOENT;
		}
	}

	frame = &data->anim->frames[data->current_frame++];
	if (frame->state) {
		led_on(data->dev, data->led);
	} else {
		led_off(data->dev, data->led);
	}

	data->deadline = k_uptime_get() + frame->delay_ms;

	return 0;
}

int led_anim_schedule_next(struct led_anim_player *player)
{
	struct led_anim_data *data;
	uint32_t deadline;
	uint32_t now;
	int ret;

	if (!player) {
		return -EINVAL;
	}

retry:
	deadline = led_anim_get_next_anim_deadline(player, &data);
	if (!deadline) {
		LOG_INF("No animation active, exiting");
		return 0;
	}

	now = k_uptime_get();
	if (deadline < now) {
		LOG_INF("Playing frame now");

		/* If we are already to late, play the frame and try to found another one to schedule*/
		led_anim_play_frame(data);
		goto retry;
	} else {
		player->scheduled_data = data;
		if (player->scheduled_deadline > deadline) {
			LOG_INF("Replacing the scheduled frame by a new one");
			player->scheduled_deadline = deadline;
			ret = k_work_reschedule(&player->dwork, K_MSEC(deadline - now));
		} else {
			LOG_INF("Scheduling next frame");
			player->scheduled_deadline = deadline;
			ret = k_work_schedule(&player->dwork, K_MSEC(deadline - now));
		}
	}

	return 0;
}

int led_anim_init(struct led_anim_data *data, const struct device *dev, uint32_t led, struct led_anim *anim)
{
	if (!data || !dev || !anim) {
		return -EINVAL;
	}

	data->dev = dev;
	data->led = led;
	data->anim = anim;
	data->deadline = 0;
	data->current_frame = 0;
	data->remaining_loop = anim->loop - 1;

	return 0;
}

static void led_anim_player_work_cb(struct k_work *work)
{
        struct k_work_delayable *dwork = k_work_delayable_from_work(work);
        struct led_anim_player *player = CONTAINER_OF(dwork, struct led_anim_player,
						      dwork);
	int ret;

	k_mutex_lock(&player->mutex, K_FOREVER);
	LOG_INF("Loading the frame");
	ret = led_anim_play_frame(player->scheduled_data);
	if (!ret) {
		LOG_INF("Scheduling the next frame");
		led_anim_schedule_next(player);
	}
	k_mutex_unlock(&player->mutex);
}

int led_anim_play_anim(struct led_anim_data *data)
{
	return led_anim_play_anim_with_cb(data, NULL, NULL);
}

int led_anim_play_anim_with_cb(struct led_anim_data *data, led_anim_complete_cb cb, void *user_data)
{
	int ret;

	if (!data) {
		return -EINVAL;
	}

	data->cb = cb;
	data->user_data = user_data;

	k_mutex_lock(&g_player.mutex, K_FOREVER);

	/* Make sure anim start right now */
	data->deadline = k_uptime_get();
	sys_slist_append(&g_player.list, &data->node);
	ret = led_anim_schedule_next(&g_player);
	k_mutex_unlock(&g_player.mutex);

	return ret;
}

int led_anim_stop_anim(struct led_anim_data *data)
{
	struct k_work_sync sync;

	if (!data) {
		return -EINVAL;
	}

	k_mutex_lock(&g_player.mutex, K_FOREVER);

	LOG_INF("Stopping the animation");
	/* Remove current anim for workqueue and schedule next anim if any */
	if (g_player.scheduled_data == data) {
		LOG_INF("Cancelling animation work");
		k_work_cancel_delayable_sync(&g_player.dwork, &sync);
		g_player.scheduled_data = NULL;
	}

	sys_slist_find_and_remove(&g_player.list, &data->node);
	led_anim_schedule_next(&g_player);

	LOG_INF("Done");
	k_mutex_unlock(&g_player.mutex);

	return 0;
}

int led_anim_player_do_init(struct led_anim_player *player)
{
	k_work_init_delayable(&player->dwork,
			      led_anim_player_work_cb);
	sys_slist_init(&player->list);
	k_mutex_init(&player->mutex);

	return 0;

}

static int led_anim_player_init(void)
{
	return led_anim_player_do_init(&g_player);
}

#define LED_SERVICE_INIT_PRIO 32
SYS_INIT(led_anim_player_init, POST_KERNEL, LED_SERVICE_INIT_PRIO);
