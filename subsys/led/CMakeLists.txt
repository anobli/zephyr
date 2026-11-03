# Copyright (c) 2026 BayLibre SAS
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_include_directories(.)
zephyr_library_sources(led_player.c)
zephyr_library_sources_ifdef(CONFIG_STATUS_LED status_led.c)
