/* PipeWire */
/* SPDX-FileCopyrightText: Copyright © 2018 Wim Taymans */
/* SPDX-License-Identifier: MIT */

#include "config.h"

#include <string.h>
#include <stdio.h>
#include <dlfcn.h>

#include <spa/utils/result.h>
#include <spa/param/props.h>
#include <spa/pod/iter.h>
#include <spa/debug/types.h>

#include "spa-device.h"

struct impl {
	struct pw_impl_device *this;

	enum pw_spa_device_flags flags;

	void *unload;
        struct spa_handle *handle;
        struct spa_device *device;

	struct spa_hook device_listener;

	void *user_data;
};

static void device_free(void *data)
{
	struct impl *impl = data;
	struct pw_impl_device *device = impl->this;

	pw_log_debug("spa-device %p: free", device);

	spa_hook_remove(&impl->device_listener);
	if (impl->handle)
		pw_unload_spa_handle(impl->handle);
}

static const struct pw_impl_device_events device_events = {
	PW_VERSION_IMPL_DEVICE_EVENTS,
	.free = device_free,
};

struct pw_impl_device *
pw_spa_device_new(struct pw_context *context,
		  enum pw_spa_device_flags flags,
		  struct spa_device *device,
		  struct spa_handle *handle,
		  struct pw_properties *properties,
		  size_t user_data_size)
{
	struct pw_impl_device *this;
	struct impl *impl;
	int res;

	this = pw_context_create_device(context, properties, sizeof(struct impl) + user_data_size);
	if (this == NULL)
		return NULL;

	impl = pw_impl_device_get_user_data(this);
	impl->this = this;
	impl->device = device;
	impl->handle = handle;
	impl->flags = flags;

	if (user_data_size > 0)
                impl->user_data = SPA_PTROFF(impl, sizeof(struct impl), void);

	pw_impl_device_add_listener(this, &impl->device_listener, &device_events, impl);
	pw_impl_device_set_implementation(this, impl->device);

	if (!SPA_FLAG_IS_SET(impl->flags, PW_SPA_DEVICE_FLAG_NO_REGISTER)) {
		if ((res = pw_impl_device_register(this, NULL)) < 0)
			goto error_register;
	}
	return this;

error_register:
	pw_impl_device_destroy(this);
	errno = -res;
	return NULL;
}

void *pw_spa_device_get_user_data(struct pw_impl_device *device)
{
	struct impl *impl = pw_impl_device_get_user_data(device);
	return impl->user_data;
}

struct match {
	struct pw_properties *props;
	int count;
};
#define MATCH_INIT(p) ((struct match){ .props = (p) })

static int execute_match(void *data, const char *location, const char *action,
		const char *val, size_t len)
{
	struct match *match = data;
	if (spa_streq(action, "update-props")) {
		match->count += pw_properties_update_string(match->props, val, len);
	}
	return 1;
}

struct pw_impl_device *pw_spa_device_load(struct pw_context *context,
				 const char *factory_name,
				 enum pw_spa_device_flags flags,
				 struct pw_properties *properties,
				 size_t user_data_size)
{
	struct pw_impl_device *this;
	struct spa_handle *handle;
	void *iface;
	int res;
	struct match match;

	if (properties) {
		match = MATCH_INIT(properties);
		pw_context_conf_section_match_rules(context, "device.rules",
				&properties->dict, execute_match, &match);
	}
	handle = pw_context_load_spa_handle(context, factory_name,
			properties ? &properties->dict : NULL);
	if (handle == NULL)
		goto error_load;

	if ((res = spa_handle_get_interface(handle, SPA_TYPE_INTERFACE_Device, &iface)) < 0)
		goto error_interface;

	this = pw_spa_device_new(context, flags,
			       iface, handle, properties, user_data_size);
	if (this == NULL)
		goto error_device;

	return this;

error_load:
	res = -errno;
	pw_log_debug("can't load device handle %s: %m", factory_name);
	goto error_exit;
error_interface:
	pw_log_debug("can't get device interface %s: %s", factory_name,
			spa_strerror(res));
	goto error_exit_unload;
error_device:
	properties = NULL;
	res = -errno;
	pw_log_debug("can't create device %s: %m", factory_name);
	goto error_exit_unload;

error_exit_unload:
	pw_unload_spa_handle(handle);
error_exit:
	errno = -res;
	pw_properties_free(properties);
	return NULL;
}
