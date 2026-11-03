/*
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_fake.h>

#include <zephyr/fff.h>
DEFINE_FFF_GLOBALS;

#include <zephyr/led/led_player.h>

#define LED0_DEV DEVICE_DT_GET(DT_NODELABEL(led_0))
#define LED1_DEV DEVICE_DT_GET(DT_NODELABEL(led_1))
#define LED2_DEV DEVICE_DT_GET(DT_NODELABEL(led_2))

static struct gpio_dt_spec led0_gpio = GPIO_DT_SPEC_GET(DT_PATH(led_0, led), gpios);
static struct gpio_dt_spec led1_gpio = GPIO_DT_SPEC_GET(DT_PATH(led_1, led), gpios);

static struct led_anim_frame test_frames1[] = 
{
	{
		.delay_ms = 1000,
		.state = 1,
	},
	{
		.delay_ms = 2000,
		.state = 0,
	}
};

struct led_anim test_anim1 = {
	.frames = test_frames1,
	.count = ARRAY_SIZE(test_frames1),
	.loop = 1,
};

static struct led_anim_frame test_frames2[] = 
{
	{
		.delay_ms = 500,
		.state = 1,
	},
	{
		.delay_ms = 500,
		.state = 0,
	}
};

struct led_anim test_anim2 = {
	.frames = test_frames2,
	.count = ARRAY_SIZE(test_frames2),
	.loop = 1,
};

struct led_anim test_anim3 = {
	.frames = test_frames2,
	.count = ARRAY_SIZE(test_frames2),
	.loop = 0,
};

ZTEST(led_player, test_led_anim_get_next_anim_deadline_invalid_vals)
{
	struct led_anim_player player;
	struct led_anim_data *data;
	uint32_t deadline;

	/* In case of error, dealy should be 0 */
	deadline = led_anim_get_next_anim_deadline(NULL, &data);
	zassert_equal(deadline, 0);

	/* In case of error, dealy should be 0 */
	deadline = led_anim_get_next_anim_deadline(&player, NULL);
	zassert_equal(deadline, 0);
}

ZTEST(led_player, test_led_anim_get_next_anim_deadline)
{
	struct led_anim_data *data;
	struct led_anim_data data1 = { .deadline = 34};
	struct led_anim_data data2 = { .deadline = 2 };
	struct led_anim_data data3 = { .deadline = 21 };
	struct led_anim_player player;
	uint32_t deadline;

	sys_slist_init(&player.list);
	sys_slist_prepend(&player.list, &data1.node);
	sys_slist_prepend(&player.list, &data2.node);
	sys_slist_prepend(&player.list, &data3.node);

	deadline = led_anim_get_next_anim_deadline(&player, &data);
	zassert_equal(deadline, 2);

	/* A deadline of 0 mean animation is not running and should skipped */
	data2.deadline = 0;
	deadline = led_anim_get_next_anim_deadline(&player, &data);
	zassert_equal(deadline, 21);

	/* If there is not animation loaded, should return 0*/
	sys_slist_init(&player.list);
	deadline = led_anim_get_next_anim_deadline(&player, &data);
	zassert_equal(deadline, 0);
}

ZTEST(led_player, test_led_anim_init_invalid_vals)
{
	static struct led_anim_data data1;
	int ret;

	ret = led_anim_init(NULL, LED0_DEV, 0, &test_anim1);
	zassert_equal(ret, -EINVAL);

	ret = led_anim_init(&data1, NULL, 0, &test_anim1);
	zassert_equal(ret, -EINVAL);

	ret = led_anim_init(&data1, LED0_DEV, 0, NULL);
	zassert_equal(ret, -EINVAL);
}

ZTEST(led_player, test_led_anim_init)
{
	static struct led_anim_data data1;
	int ret;

	ret = led_anim_init(&data1, LED0_DEV, 0, &test_anim1);
	zassert_equal(ret, 0);
	zassert_equal(data1.dev, LED0_DEV);
	zassert_equal(data1.led, 0);
	zassert_equal_ptr(data1.anim, &test_anim1);
	zassert_equal(data1.current_frame, 0);
	zassert_equal(data1.deadline, 0);
}

ZTEST(led_player, test_led_anim_play_frame_invalid_vals)
{
	static struct led_anim_data data1;
	int ret;

	ret = led_anim_init(&data1, LED0_DEV, 0, &test_anim1);
	zassert_equal(ret, 0);

	ret = led_anim_play_frame(NULL);
	zassert_equal(ret, -EINVAL);
}

ZTEST(led_player, test_led_anim_play_frame)
{
	static struct led_anim_data data1;
	int ret;

	ret = led_anim_init(&data1, LED0_DEV, 0, &test_anim1);
	zassert_equal(ret, 0);

	ret = led_anim_play_frame(&data1);
	zassert_equal(ret, 0);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg0_history[0], led0_gpio.port);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg1_history[0], BIT(led0_gpio.pin));

	ret = led_anim_play_frame(&data1);
	zassert_equal(ret, 0);
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg0_history[0], led0_gpio.port);
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg1_history[0], BIT(led0_gpio.pin));

	ret = led_anim_play_frame(&data1);
	zassert_equal(ret, -ENOENT);
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.call_count, 1);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.call_count, 1);
}

ZTEST(led_player, test_led_anim_schedule_next_invalid_vals)
{
	int ret;

	ret = led_anim_schedule_next(NULL);
	zassert_equal(ret, -EINVAL);
}

ZTEST(led_player, test_led_anim_schedule_next)
{
	struct led_anim_player player;
	static struct led_anim_data data1;
	int ret;

	led_anim_player_do_init(&player);
	led_anim_init(&data1, LED0_DEV, 0, &test_anim1);

	ret = led_anim_schedule_next(&player);
	zassert_equal(ret, 0);

	/* Prepare manually the anim to play and run it */
	k_sleep(K_MSEC(1000)); /* Prevent an uptime of 0*/
	data1.deadline = k_uptime_get() - 1;
	sys_slist_prepend(&player.list, &data1.node);
	ret = led_anim_schedule_next(&player);
	zassert_equal(ret, 0);

	/* Give some time to run animation */
	k_sleep(K_MSEC(5000));

	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg0_history[0], led0_gpio.port);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg1_history[0], BIT(led0_gpio.pin));
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg0_history[0], led0_gpio.port);
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg1_history[0], BIT(led0_gpio.pin));
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.call_count, 1);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.call_count, 1);
}

ZTEST(led_player, test_led_anim_schedule_next_multiple_led)
{
	struct led_anim_player player;
	static struct led_anim_data data1;
	static struct led_anim_data data2;
	int ret;

	led_anim_player_do_init(&player);
	led_anim_init(&data1, LED0_DEV, 0, &test_anim1);
	led_anim_init(&data2, LED1_DEV, 0, &test_anim1);

	/* Prepare manually the anim to play and run it */
	k_sleep(K_MSEC(1000)); /* Prevent an uptime of 0*/
	data1.deadline = k_uptime_get();
	data2.deadline = k_uptime_get() + 500;
	sys_slist_prepend(&player.list, &data1.node);
	sys_slist_prepend(&player.list, &data2.node);
	ret = led_anim_schedule_next(&player);
	zassert_equal(ret, 0);

	/* Give some time to run animation */
	k_sleep(K_MSEC(5000));

	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg0_history[0], led0_gpio.port);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg1_history[0], BIT(led0_gpio.pin));
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg0_history[1], led1_gpio.port);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg1_history[1], BIT(led1_gpio.pin));
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg0_history[0], led0_gpio.port);
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg1_history[0], BIT(led0_gpio.pin));
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg0_history[1], led1_gpio.port);
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg1_history[1], BIT(led1_gpio.pin));
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.call_count, 2);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.call_count, 2);
}

ZTEST(led_player, test_led_anim_schedule_next_multiple_led_and_anim)
{
	struct led_anim_player player;
	static struct led_anim_data data1;
	static struct led_anim_data data2;
	int ret;

	led_anim_player_do_init(&player);
	led_anim_init(&data1, LED0_DEV, 0, &test_anim1);
	led_anim_init(&data2, LED1_DEV, 0, &test_anim2);

	/* Prepare manually the anim to play and run it */
	k_sleep(K_MSEC(1000)); /* Prevent an uptime of 0*/
	data1.deadline = k_uptime_get();
	data2.deadline = k_uptime_get() + 100;
	sys_slist_prepend(&player.list, &data1.node);
	sys_slist_prepend(&player.list, &data2.node);
	ret = led_anim_schedule_next(&player);
	zassert_equal(ret, 0);

	/* Give some time to run animation */
	k_sleep(K_MSEC(5000));

	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg0_history[0], led0_gpio.port);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg1_history[0], BIT(led0_gpio.pin));
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg0_history[1], led1_gpio.port);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg1_history[1], BIT(led1_gpio.pin));
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg0_history[0], led1_gpio.port);
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg1_history[0], BIT(led1_gpio.pin));
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg0_history[1], led0_gpio.port);
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg1_history[1], BIT(led0_gpio.pin));
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.call_count, 2);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.call_count, 2);
}

ZTEST(led_player, test_led_anim_schedule_next_loop)
{
	struct k_work_sync sync;
	struct led_anim_player player;
	static struct led_anim_data data1;
	int ret;

	led_anim_player_do_init(&player);
	led_anim_init(&data1, LED0_DEV, 0, &test_anim3);

	/* Prepare manually the anim to play and run it */
	k_sleep(K_MSEC(1000)); /* Prevent an uptime of 0*/
	data1.deadline = k_uptime_get();
	sys_slist_prepend(&player.list, &data1.node);
	ret = led_anim_schedule_next(&player);
	zassert_equal(ret, 0);

	/* Give some time to run animation */
	k_sleep(K_MSEC(5000));

	/* The anim is set to infinite, so we need to stop it to let the test finish */
	k_work_cancel_delayable_sync(&player.dwork, &sync);

	for (int i = 0; i < 4; i++) {
		zassert_equal(gpio_fake_port_set_bits_raw_fake.arg0_history[i], led0_gpio.port);
		zassert_equal(gpio_fake_port_set_bits_raw_fake.arg1_history[i], BIT(led0_gpio.pin));
		zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg0_history[i], led0_gpio.port);
		zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg1_history[i], BIT(led0_gpio.pin));
	}
}

ZTEST(led_player, test_led_anim_play_anim_invalid_vals)
{
	static struct led_anim_data data1;
	int ret;

	ret = led_anim_init(&data1, LED0_DEV, 0, &test_anim1);
	zassert_equal(ret, 0);

	ret = led_anim_play_anim(NULL);
	zassert_equal(ret, -EINVAL);
}

ZTEST(led_player, test_led_anim_play_anim)
{
	static struct led_anim_data data1;
	int ret;

	led_anim_init(&data1, LED0_DEV, 0, &test_anim1);

	k_sleep(K_MSEC(1000)); /* Prevent an uptime of 0 */

	ret = led_anim_play_anim(&data1);
	zassert_equal(ret, 0);

	/* Give some time to run animation */
	k_sleep(K_MSEC(5000));

	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg0_history[0], led0_gpio.port);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg1_history[0], BIT(led0_gpio.pin));
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg0_history[0], led0_gpio.port);
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg1_history[0], BIT(led0_gpio.pin));
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.call_count, 1);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.call_count, 1);
}

ZTEST(led_player, test_led_anim_stop_anim_invalid_vals)
{
	static struct led_anim_data data1;
	int ret;

	ret = led_anim_init(&data1, LED0_DEV, 0, &test_anim1);
	zassert_equal(ret, 0);

	ret = led_anim_stop_anim(NULL);
	zassert_equal(ret, -EINVAL);
}

ZTEST(led_player, test_led_anim_stop_anim)
{
	static struct led_anim_data data1;
	int count_clear, count_set;
	int ret;

	led_anim_init(&data1, LED0_DEV, 0, &test_anim3);

	k_sleep(K_MSEC(1000)); /* Prevent an uptime of 0 */

	led_anim_play_anim(&data1);

	/* Give some time to run animation */
	k_sleep(K_MSEC(5000));

	ret = led_anim_stop_anim(&data1);
	zassert_equal(ret, 0);

	/* Make sure anim has been stopped */
	count_clear = gpio_fake_port_clear_bits_raw_fake.call_count;
	count_set = gpio_fake_port_set_bits_raw_fake.call_count;
	k_sleep(K_MSEC(5000));
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.call_count, count_clear);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.call_count, count_set);
}

ZTEST(led_player, test_led_anim_stop_anim_one_of_two)
{
	static struct led_anim_data data1;
	static struct led_anim_data data2;
	int count_clear, count_set;
	int ret;

	led_anim_init(&data1, LED0_DEV, 0, &test_anim3);
	led_anim_init(&data2, LED1_DEV, 0, &test_anim3);

	k_sleep(K_MSEC(1000)); /* Prevent an uptime of 0 */

	led_anim_play_anim(&data1);
	led_anim_play_anim(&data2);

	/* Give some time to run animation */
	k_sleep(K_MSEC(5000));

	ret = led_anim_stop_anim(&data1);
	zassert_equal(ret, 0);

	/* Make sure that we still have an anim running */
	count_clear = gpio_fake_port_clear_bits_raw_fake.call_count;
	count_set = gpio_fake_port_set_bits_raw_fake.call_count;
	k_sleep(K_MSEC(5000));
	zassert_not_equal(gpio_fake_port_clear_bits_raw_fake.call_count, count_clear);
	zassert_not_equal(gpio_fake_port_set_bits_raw_fake.call_count, count_set);

	led_anim_stop_anim(&data2);
}

ZTEST(led_player, test_led_anim_stop_anim_already_stopped)
{
	static struct led_anim_data data1;
	int ret;

	led_anim_init(&data1, LED0_DEV, 0, &test_anim1);

	k_sleep(K_MSEC(1000)); /* Prevent an uptime of 0 */

	led_anim_play_anim(&data1);

	/* Give some time to run animation. Anim should have stop */
	k_sleep(K_MSEC(5000));

	ret = led_anim_stop_anim(&data1);
	zassert_equal(ret, 0);
}


ZTEST_SUITE(led_player, NULL, NULL, NULL, NULL, NULL);