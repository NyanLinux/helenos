/*
 * Copyright (c) 2011 Jan Vesely
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/** @addtogroup usb
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#include <errno.h>
#include <stdint.h>
#include <ddi.h>
#include <devman.h>
#include <usb/debug.h>

#include "root_hub.h"


int uhci_root_hub_init(
  uhci_root_hub_t *instance, void *addr, size_t size, ddf_dev_t *rh)
{
	assert(instance);
	assert(rh);
	int ret;
	ret = usb_hc_find(rh->handle, &instance->hc_handle);
	usb_log_info("rh found(%d) hc handle: %d.\n", ret, instance->hc_handle);
	if (ret != EOK) {
		return ret;
	}

	/* allow access to root hub registers */
	assert(sizeof(port_status_t) * UHCI_ROOT_HUB_PORT_COUNT == size);
	port_status_t *regs;
	ret = pio_enable(
	  addr, sizeof(port_status_t) * UHCI_ROOT_HUB_PORT_COUNT, (void**)&regs);

	if (ret < 0) {
		usb_log_error("Failed to gain access to port registers at %p\n", regs);
		return ret;
	}

	/* add fibrils for periodic port checks */
	unsigned i = 0;
	for (; i < UHCI_ROOT_HUB_PORT_COUNT; ++i) {
		/* connect to the parent device (HC) */
		int parent_phone = devman_device_connect(instance->hc_handle, 0);
		//usb_drv_hc_connect(rh, instance->hc_handle, 0);
		if (parent_phone < 0) {
			usb_log_error("Failed to connect to the HC device port %d.\n", i);
			return parent_phone;
		}
		/* mind pointer arithmetics */
		int ret = uhci_port_init(
		  &instance->ports[i], regs + i, i, ROOT_HUB_WAIT_USEC, rh, parent_phone);
		if (ret != EOK) {
			unsigned j = 0;
			for (;j < i; ++j)
				uhci_port_fini(&instance->ports[j]);
			return ret;
		}
	}

	return EOK;
}
/*----------------------------------------------------------------------------*/
int uhci_root_hub_fini( uhci_root_hub_t* instance )
{
	assert( instance );
	// TODO:
	//destroy fibril here
	//disable access to registers
	return EOK;
}
/*----------------------------------------------------------------------------*/
/**
 * @}
 */
