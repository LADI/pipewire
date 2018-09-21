/* PipeWire
 * Copyright (C) 2018 Wim Taymans <wim.taymans@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>

#include "config.h"

#include "pipewire/core.h"
#include "pipewire/interfaces.h"
#include "pipewire/log.h"
#include "pipewire/module.h"
#include "pipewire/link.h"
#include "pipewire/private.h"

static const struct spa_dict_item module_props[] = {
	{ PW_MODULE_PROP_AUTHOR, "Wim Taymans <wim.taymans@gmail.com>" },
	{ PW_MODULE_PROP_DESCRIPTION, "Allow clients to create links" },
	{ PW_MODULE_PROP_VERSION, PACKAGE_VERSION },
};

struct factory_data {
	struct pw_factory *this;
	struct pw_properties *properties;

	struct spa_list link_list;

	struct spa_hook module_listener;
};

struct link_data {
	struct factory_data *data;
	struct pw_link *link;
	struct spa_list l;
	struct spa_hook link_listener;
	struct spa_hook resource_listener;
};

static void resource_destroy(void *data)
{
	struct link_data *ld = data;
	if (ld->link)
		pw_link_destroy(ld->link);
}

static const struct pw_resource_events resource_events = {
	PW_VERSION_RESOURCE_EVENTS,
	.destroy = resource_destroy
};

static void link_destroy(void *data)
{
	struct link_data *ld = data;
	spa_list_remove(&ld->l);
	ld->link = NULL;
}

static const struct pw_link_events link_events = {
	PW_VERSION_LINK_EVENTS,
	.destroy = link_destroy
};

static struct pw_port *get_port(struct pw_node *node, enum spa_direction direction)
{
	struct pw_port *p;
	int res;

	p = pw_node_find_port(node, direction, SPA_ID_INVALID);

	if (p == NULL || pw_port_is_linked(p)) {
		uint32_t port_id;

		port_id = pw_node_get_free_port_id(node, direction);
		if (port_id == SPA_ID_INVALID)
			return NULL;

		p = pw_port_new(direction, port_id, NULL, 0);
		if (p == NULL)
			return NULL;

		if ((res = pw_port_add(p, node)) < 0) {
			pw_log_warn("can't add port: %s", spa_strerror(res));
			return NULL;
		}
	}
	return p;
}


static void *create_object(void *_data,
			   struct pw_resource *resource,
			   uint32_t type,
			   uint32_t version,
			   struct pw_properties *properties,
			   uint32_t new_id)
{
	struct factory_data *d = _data;
	struct pw_client *client;
	struct pw_node *output_node, *input_node;
	struct pw_port *outport, *inport;
	struct pw_core *core;
	struct pw_global *global;
	struct pw_link *link;
	uint32_t output_node_id, input_node_id;
	uint32_t output_port_id, input_port_id;
	struct link_data *ld;
	struct pw_resource *bound_resource;
	char *error;
	const char *str;
	int res;
	bool linger;

	if (resource == NULL)
		goto no_resource;

	if (properties == NULL)
		goto no_properties;

	if ((str = pw_properties_get(properties, PW_LINK_OUTPUT_NODE_ID)) == NULL)
		goto no_properties;

	output_node_id = pw_properties_parse_int(str);

	if ((str = pw_properties_get(properties, PW_LINK_INPUT_NODE_ID)) == NULL)
		goto no_properties;

	input_node_id = pw_properties_parse_int(str);

	str = pw_properties_get(properties, PW_LINK_OUTPUT_PORT_ID);
	output_port_id = str ? pw_properties_parse_int(str) : -1;

	str = pw_properties_get(properties, PW_LINK_INPUT_PORT_ID);
	input_port_id = str ? pw_properties_parse_int(str) : -1;

	client = pw_resource_get_client(resource);
	core = pw_client_get_core(client);

	global = pw_core_find_global(core, output_node_id);
	if (global == NULL || pw_global_get_type(global) != PW_TYPE_INTERFACE_Node)
		goto no_output;

	output_node = pw_global_get_object(global);

	global = pw_core_find_global(core, input_node_id);
	if (global == NULL || pw_global_get_type(global) != PW_TYPE_INTERFACE_Node)
		goto no_input;

	input_node = pw_global_get_object(global);

	if (output_port_id == -1) {
		outport = get_port(output_node, SPA_DIRECTION_OUTPUT);
	}
	else {
		global = pw_core_find_global(core, output_port_id);
		if (global == NULL || pw_global_get_type(global) != PW_TYPE_INTERFACE_Port)
			goto no_output_port;

		outport = pw_global_get_object(global);
	}
	if (outport == NULL)
		goto no_output_port;

	if (input_port_id == -1)
		inport = get_port(input_node, SPA_DIRECTION_INPUT);
	else {
		global = pw_core_find_global(core, input_port_id);
		if (global == NULL || pw_global_get_type(global) != PW_TYPE_INTERFACE_Port)
			goto no_output_port;

		inport = pw_global_get_object(global);
	}
	if (inport == NULL)
		goto no_input_port;

	str = pw_properties_get(properties, "object.linger");
	linger = str ? pw_properties_parse_bool(str) : false;

	link = pw_link_new(core, outport, inport, NULL, properties, &error, sizeof(struct link_data));
	if (link == NULL)
		goto no_mem;

	ld = pw_link_get_user_data(link);
	ld->data = d;
	ld->link = link;
	spa_list_append(&d->link_list, &ld->l);

	pw_link_add_listener(link, &ld->link_listener, &link_events, ld);
	pw_link_register(link,
			linger ? NULL : client,
			linger ? NULL : pw_client_get_global(client),
			NULL);

	properties = NULL;

	res = pw_global_bind(pw_link_get_global(link), client, PW_PERM_RWX, PW_VERSION_LINK, new_id);
	if (res < 0)
		goto no_bind;

	if (!linger) {
		if ((bound_resource = pw_client_find_resource(client, new_id)) == NULL)
			goto no_bind;

		pw_resource_add_listener(bound_resource, &ld->resource_listener, &resource_events, ld);
	}

	return link;

      no_resource:
	pw_log_error("link factory needs a resource");
	pw_resource_error(resource, -EINVAL, "no resource");
	goto done;
      no_properties:
	pw_log_error("link-factory needs properties");
	pw_resource_error(resource, -EINVAL, "no properties");
	goto done;
      no_output:
	pw_log_error("link-factory unknown output node %d", output_node_id);
	pw_resource_error(resource, -EINVAL, "unknown output node");
	goto done;
      no_input:
	pw_log_error("link-factory unknown input node %d", input_node_id);
	pw_resource_error(resource, -EINVAL, "unknown input node");
	goto done;
      no_output_port:
	pw_log_error("link-factory unknown output port %d", output_port_id);
	pw_resource_error(resource, -EINVAL, "unknown output port");
	goto done;
      no_input_port:
	pw_log_error("link-factory unknown input port %d", input_port_id);
	pw_resource_error(resource, -EINVAL, "unknown input port");
	goto done;
      no_mem:
	pw_log_error("can't create link");
	pw_resource_error(resource, -ENOMEM, "no memory");
	goto done;
      no_bind:
	pw_resource_error(resource, res, "can't bind link");
	goto done;
      done:
	if (properties)
		pw_properties_free(properties);
	return NULL;
}

static const struct pw_factory_implementation impl_factory = {
	PW_VERSION_FACTORY_IMPLEMENTATION,
	.create_object = create_object,
};

static void module_destroy(void *data)
{
	struct factory_data *d = data;
	struct link_data *ld, *t;

	spa_hook_remove(&d->module_listener);

	spa_list_for_each_safe(ld, t, &d->link_list, l)
		pw_link_destroy(ld->link);

	if (d->properties)
		pw_properties_free(d->properties);

	pw_factory_destroy(d->this);
}

static const struct pw_module_events module_events = {
	PW_VERSION_MODULE_EVENTS,
	.destroy = module_destroy,
};

static int module_init(struct pw_module *module, struct pw_properties *properties)
{
	struct pw_core *core = pw_module_get_core(module);
	struct pw_factory *factory;
	struct factory_data *data;

	factory = pw_factory_new(core,
				 "link-factory",
				 PW_TYPE_INTERFACE_Link,
				 PW_VERSION_LINK,
				 NULL,
				 sizeof(*data));
	if (factory == NULL)
		return -ENOMEM;

	data = pw_factory_get_user_data(factory);
	data->this = factory;
	data->properties = properties;
	spa_list_init(&data->link_list);

	pw_log_debug("module %p: new", module);

	pw_factory_set_implementation(factory,
				      &impl_factory,
				      data);

	pw_factory_register(factory, NULL, pw_module_get_global(module), NULL);

	pw_module_add_listener(module, &data->module_listener, &module_events, data);

	pw_module_update_properties(module, &SPA_DICT_INIT_ARRAY(module_props));

	return 0;
}

int pipewire__module_init(struct pw_module *module, const char *args)
{
	return module_init(module, NULL);
}
