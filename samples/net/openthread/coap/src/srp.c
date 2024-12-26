/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(coap);

#include <openthread/link.h>
#include "srp.h"

static const char *SRP_INSTANCE_NAME = "ot-service";
static const char *SRP_SERVICE_NAME = "_coap_example._udp";
#define OT_DEFAULT_COAP_PORT 1234

static bool ot_srp_init_done = false;

void ot_srp_callback(otError aError, const otSrpClientHostInfo *aHostInfo,
		     const otSrpClientService *aServices,
		     const otSrpClientService *aRemovedServices, void *aContext)
{
	if (aError != OT_ERROR_NONE) {
		LOG_ERR("SRP update error: %s", otThreadErrorToString(aError));
	}
	LOG_INF("SRP update registered");
}

void ot_srp_set_host_name(otInstance *ot, char *host_name, uint16_t size)
{
	otExtAddress extAddress;

	otLinkGetFactoryAssignedIeeeEui64(ot, &extAddress);
    size = MIN(size, strlen(CONFIG_BOARD) + 1);
	strncpy(host_name, CONFIG_BOARD, size);
    strncat(host_name, "-", size);

	for (int i = 0; i < OT_EXT_ADDRESS_SIZE; i++) {
		char tmp[3];

		sprintf(tmp, "%02x", extAddress.m8[i]);
		strncat(host_name, tmp, size);
	}
}

int ot_srp_init(void)
{
	otError error;
	otInstance *ot;
	otSrpClientBuffersServiceEntry *entry;
	char *host_name;
	char *instance_name;
	char *service_name;
	uint16_t size;

	if (ot_srp_init_done) {
		return 0;
	}

	LOG_INF("Initializing SRP client");

	ot = openthread_get_default_instance();
	if (!ot) {
		LOG_ERR("Failed to get an OpenThread instance");
		return -ENODEV;
	}

	otSrpClientSetCallback(ot, ot_srp_callback, NULL);
	host_name = otSrpClientBuffersGetHostNameString(ot, &size);
	ot_srp_set_host_name(ot, host_name, size);
	error = otSrpClientSetHostName(ot, host_name);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to set SRP host name: %s", otThreadErrorToString(error));
		return -1;
	}

	error = otSrpClientEnableAutoHostAddress(ot);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to set SRP host address: %s", otThreadErrorToString(error));
		return -1;
	}

	entry = otSrpClientBuffersAllocateService(ot);
	if (entry == NULL) {
		LOG_ERR("Failed to allocate SRP service: %s", otThreadErrorToString(error));
		return -1;
	}
	entry->mService.mPort = OT_DEFAULT_COAP_PORT;

	instance_name = otSrpClientBuffersGetServiceEntryInstanceNameString(entry, &size);
	size = MIN(size, strlen(SRP_INSTANCE_NAME) + 1);
	memcpy(instance_name, SRP_INSTANCE_NAME, size);

	service_name = otSrpClientBuffersGetServiceEntryServiceNameString(entry, &size);
	size = MIN(size, strlen(SRP_SERVICE_NAME) + 1);
	memcpy(service_name, SRP_SERVICE_NAME, size);

	error = otSrpClientAddService(ot, &entry->mService);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to register SRP service: %s", otThreadErrorToString(error));
		return -1;
	}

	otSrpClientEnableAutoStartMode(ot, NULL, NULL);

	ot_srp_init_done = true;

	return 0;
}
