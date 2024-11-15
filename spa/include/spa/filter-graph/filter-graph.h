/* Simple Plugin API */
/* SPDX-FileCopyrightText: Copyright © 2024 Wim Taymans */
/* SPDX-License-Identifier: MIT */

#ifndef SPA_FILTER_GRAPH_H
#define SPA_FILTER_GRAPH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#include <spa/utils/defs.h>
#include <spa/utils/hook.h>
#include <spa/pod/builder.h>

/** \defgroup spa_filter_graph Filter Graph
 * a graph of filters
 */

/**
 * \addtogroup spa_filter_graph
 * \{
 */

/**
 * A graph of filters
 */
#define SPA_TYPE_INTERFACE_FilterGraph	SPA_TYPE_INFO_INTERFACE_BASE "FilterGraph"

#define SPA_VERSION_FILTER_GRAPH		0
struct spa_filter_graph { struct spa_interface iface; };

struct spa_filter_graph_info {
	uint32_t n_inputs;
	uint32_t n_outputs;

#define SPA_FILTER_GRAPH_CHANGE_MASK_FLAGS		(1u<<0)
#define SPA_FILTER_GRAPH_CHANGE_MASK_PROPS		(1u<<1)
	uint64_t change_mask;

	uint64_t flags;
	struct spa_dict *props;
};

struct spa_filter_graph_events {
#define SPA_VERSION_FILTER_GRAPH_EVENTS	0
	uint32_t version;

	void (*info) (void *object, const struct spa_filter_graph_info *info);

	void (*apply_props) (void *object, enum spa_direction direction, const struct spa_pod *props);

	void (*props_changed) (void *object, enum spa_direction direction);
};

struct spa_filter_graph_chunk {
	void *data;
	size_t size;
};

struct spa_filter_graph_methods {
#define SPA_VERSION_FILTER_GRAPH_METHODS	0
	uint32_t version;

	int (*add_listener) (void *object,
			struct spa_hook *listener,
			const struct spa_filter_graph_events *events,
			void *data);

	int (*enum_prop_info) (void *object, uint32_t idx, struct spa_pod_builder *b);
	int (*get_props) (void *object, struct spa_pod_builder *b, const struct spa_pod **props);
	int (*set_props) (void *object, enum spa_direction direction, const struct spa_pod *props);

	int (*activate) (void *object, const struct spa_fraction *rate);
	int (*deactivate) (void *object);

	int (*reset) (void *object);

	int (*process) (void *object,
			const struct spa_filter_graph_chunk in[], uint32_t n_in,
			struct spa_filter_graph_chunk out[], uint32_t n_out);
};

#define spa_filter_graph_method_r(o,method,version,...)			\
({									\
	volatile int _res = -ENOTSUP;					\
	struct spa_filter_graph *_o = o;				\
	spa_interface_call_fast_res(&_o->iface,				\
			struct spa_filter_graph_methods, _res,		\
			method, version, ##__VA_ARGS__);		\
	_res;								\
})

#define spa_filter_graph_add_listener(o,...)	spa_filter_graph_method_r(o,add_listener,0,__VA_ARGS__)

#define spa_filter_graph_enum_prop_info(o,...)	spa_filter_graph_method_r(o,enum_prop_info,0,__VA_ARGS__)
#define spa_filter_graph_get_props(o,...)	spa_filter_graph_method_r(o,get_props,0,__VA_ARGS__)
#define spa_filter_graph_set_props(o,...)	spa_filter_graph_method_r(o,set_props,0,__VA_ARGS__)

#define spa_filter_graph_activate(o,...)	spa_filter_graph_method_r(o,activate,0,__VA_ARGS__)
#define spa_filter_graph_deactivate(o)		spa_filter_graph_method_r(o,deactivate,0)

#define spa_filter_graph_reset(o)		spa_filter_graph_method_r(o,reset,0)

#define spa_filter_graph_process(o,...)		spa_filter_graph_method_r(o,process,0,__VA_ARGS__)

/**
 * \}
 */

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* SPA_FILTER_GRAPH_H */

