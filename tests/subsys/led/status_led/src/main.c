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

#include <zephyr/led/status_led.h>

#define TEST_STATUS_1 1
#define TEST_STATUS_2 65
#define TEST_STATUS_3 -10

#define TEST_PRIO_1 1
#define TEST_PRIO_2 6
#define TEST_PRIO_3 2

#define LED0_DEV DEVICE_DT_GET(DT_NODELABEL(led_0))

static struct gpio_dt_spec led0_gpio = GPIO_DT_SPEC_GET(DT_PATH(led_0, led), gpios);


static struct status_led_anim anims[] = {
	STATUS_LED_BLINK_ANIM(TEST_STATUS_1, TEST_PRIO_1, 500, 500),
	STATUS_LED_BLINK_ANIM(TEST_STATUS_2, TEST_PRIO_2, 1000, 1000),
	STATUS_LED_BLINK_ANIM(TEST_STATUS_3, TEST_PRIO_3, 2000, 500),
};
STATUS_LED_DEFINE(test_status_led, LED0_DEV, 0, anims);

ZTEST(status_led, test_status_led_set_invalid_vals)
{
	int ret;
	
	ret = status_led_set(NULL, 0);
	zassert_equal(ret, -EINVAL);
}

ZTEST(status_led, test_status_led_set)
{
	int ret;

	/* Required to a non zero base time */
	k_sleep(K_MSEC(1));

	ret = status_led_set(&test_status_led, TEST_STATUS_1);
	zassert_equal(ret, 0);

	k_sleep(K_MSEC(1));
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg0_history[0], led0_gpio.port);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg1_history[0], BIT(led0_gpio.pin));

	k_sleep(K_MSEC(500));
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg0_history[0], led0_gpio.port);
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg1_history[0], BIT(led0_gpio.pin));

	k_sleep(K_MSEC(500));
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg0_history[1], led0_gpio.port);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg1_history[1], BIT(led0_gpio.pin));

	status_led_clear(&test_status_led, TEST_STATUS_1);
}

ZTEST(status_led, test_status_led_set_with_higher_prio)
{
	int ret;

	/* Required to a non zero base time */
	k_sleep(K_MSEC(1));

	ret = status_led_set(&test_status_led, TEST_STATUS_1);
	zassert_equal(ret, 0);

	k_sleep(K_MSEC(1));
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg0_history[0], led0_gpio.port);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg1_history[0], BIT(led0_gpio.pin));

	k_sleep(K_MSEC(500));
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg0_history[0], led0_gpio.port);
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg1_history[0], BIT(led0_gpio.pin));

	k_sleep(K_MSEC(500));
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg0_history[1], led0_gpio.port);
	zassert_equal(gpio_fake_port_set_bits_raw_fake.arg1_history[1], BIT(led0_gpio.pin));

	ret = status_led_set(&test_status_led, TEST_STATUS_2);
	k_sleep(K_MSEC(1000));
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg0_history[1], led0_gpio.port);
	zassert_equal(gpio_fake_port_clear_bits_raw_fake.arg1_history[1], BIT(led0_gpio.pin));

	status_led_clear(&test_status_led, TEST_STATUS_1);
	status_led_clear(&test_status_led, TEST_STATUS_2);
}

ZTEST_SUITE(status_led, NULL, NULL, NULL, NULL, NULL);