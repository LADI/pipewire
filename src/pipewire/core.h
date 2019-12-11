/* PipeWire
 *
 * Copyright © 2018 Wim Taymans
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef PIPEWIRE_CORE_H
#define PIPEWIRE_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <spa/utils/hook.h>

#define PW_VERSION_CORE		3
struct pw_core;
#define PW_VERSION_REGISTRY	3
struct pw_registry;

/**  The core information. Extra information can be added in later versions \memberof pw_introspect */
struct pw_core_info {
	uint32_t id;			/**< id of the global */
	uint32_t cookie;		/**< a random cookie for identifying this instance of PipeWire */
	const char *user_name;		/**< name of the user that started the core */
	const char *host_name;		/**< name of the machine the core is running on */
	const char *version;		/**< version of the core */
	const char *name;		/**< name of the core */
#define PW_CORE_CHANGE_MASK_PROPS      (1 << 0)
#define PW_CORE_CHANGE_MASK_ALL        ((1 << 1)-1)
	uint64_t change_mask;		/**< bitfield of changed fields since last call */
	struct spa_dict *props;		/**< extra properties */
};

#include <pipewire/context.h>
#include <pipewire/properties.h>
#include <pipewire/node.h>
#include <pipewire/proxy.h>

/** Update and existing \ref pw_core_info with \a update  \memberof pw_introspect */
struct pw_core_info *
pw_core_info_update(struct pw_core_info *info,
		    const struct pw_core_info *update);

/** Free a \ref pw_core_info  \memberof pw_introspect */
void pw_core_info_free(struct pw_core_info *info);

/**
 * \page page_iface_pw_core pw_core
 * \section page_iface_pw_core_desc Description
 *
 * The core global object.  This is a special singleton object.  It
 * is used for internal PipeWire protocol features.
 * \section page_iface_pw_core API
 */

/** Core */

#define PW_CORE_EVENT_INFO	0
#define PW_CORE_EVENT_DONE	1
#define PW_CORE_EVENT_PING	2
#define PW_CORE_EVENT_ERROR	3
#define PW_CORE_EVENT_REMOVE_ID	4
#define PW_CORE_EVENT_BOUND_ID	5
#define PW_CORE_EVENT_ADD_MEM	6
#define PW_CORE_EVENT_REMOVE_MEM	7
#define PW_CORE_EVENT_NUM		8

/** \struct pw_core_events
 *  \brief Core events
 *  \ingroup pw_core_interface The pw_core interface
 */
struct pw_core_events {
#define PW_VERSION_CORE_EVENTS	0
	uint32_t version;

	/**
	 * Notify new core info
	 *
	 * This event is emited when first bound to the core or when the
	 * hello method is called.
	 *
	 * \param info new core info
	 */
	void (*info) (void *object, const struct pw_core_info *info);
	/**
	 * Emit a done event
	 *
	 * The done event is emited as a result of a sync method with the
	 * same seq number.
	 *
	 * \param seq the seq number passed to the sync method call
	 */
	void (*done) (void *object, uint32_t id, int seq);

	/** Emit a ping event
	 *
	 * The client should reply with a pong reply with the same seq
	 * number.
	 */
	void (*ping) (void *object, uint32_t id, int seq);

	/**
	 * Fatal error event
         *
         * The error event is sent out when a fatal (non-recoverable)
         * error has occurred. The id argument is the proxy object where
         * the error occurred, most often in response to a request to that
         * object. The message is a brief description of the error,
         * for (debugging) convenience.
	 *
	 * This event is usually also emited on the proxy object with
	 * \a id.
	 *
         * \param id object where the error occurred
         * \param seq the sequence number that generated the error
         * \param res error code
         * \param message error description
	 */
	void (*error) (void *object, uint32_t id, int seq, int res, const char *message);
	/**
	 * Remove an object ID
         *
         * This event is used internally by the object ID management
         * logic. When a client deletes an object, the server will send
         * this event to acknowledge that it has seen the delete request.
         * When the client receives this event, it will know that it can
         * safely reuse the object ID.
	 *
         * \param id deleted object ID
	 */
	void (*remove_id) (void *object, uint32_t id);

	/**
	 * Notify an object binding
	 *
	 * This event is emited when a local object ID is bound to a
	 * global ID. It is emited before the global becomes visible in the
	 * registry.
	 *
	 * \param id bound object ID
	 * \param global_id the global id bound to
	 */
	void (*bound_id) (void *object, uint32_t id, uint32_t global_id);

	/**
	 * Add memory for a client
	 *
	 * Memory is given to a client as \a fd of a certain
	 * memory \a type.
	 *
	 * Further references to this fd will be made with the per memory
	 * unique identifier \a id.
	 *
	 * \param id the unique id of the memory
	 * \param type the memory type, one of enum spa_data_type
	 * \param fd the file descriptor
	 * \param flags extra flags
	 */
	void (*add_mem) (void *object, uint32_t id, uint32_t type, int fd, uint32_t flags);

	/**
	 * Remove memory for a client
	 *
	 * \param id the memory id to remove
	 */
	void (*remove_mem) (void *object, uint32_t id);
};

#define PW_CORE_METHOD_ADD_LISTENER	0
#define PW_CORE_METHOD_HELLO		1
#define PW_CORE_METHOD_SYNC		2
#define PW_CORE_METHOD_PONG		3
#define PW_CORE_METHOD_ERROR		4
#define PW_CORE_METHOD_GET_REGISTRY	5
#define PW_CORE_METHOD_CREATE_OBJECT	6
#define PW_CORE_METHOD_DESTROY		7
#define PW_CORE_METHOD_NUM		8

/**
 * \struct pw_core_methods
 * \brief Core methods
 *
 * The core global object. This is a singleton object used for
 * creating new objects in the remote PipeWire intance. It is
 * also used for internal features.
 */
struct pw_core_methods {
#define PW_VERSION_CORE_METHODS	0
	uint32_t version;

	int (*add_listener) (void *object,
			struct spa_hook *listener,
			const struct pw_core_events *events,
			void *data);
	/**
	 * Start a conversation with the server. This will send
	 * the core info and will destroy all resources for the client
	 * (except the core and client resource).
	 */
	int (*hello) (void *object, uint32_t version);
	/**
	 * Do server roundtrip
	 *
	 * Ask the server to emit the 'done' event with \a seq.
	 *
	 * Since methods are handled in-order and events are delivered
	 * in-order, this can be used as a barrier to ensure all previous
	 * methods and the resulting events have been handled.
	 *
	 * \param seq the seq number passed to the done event
	 */
	int (*sync) (void *object, uint32_t id, int seq);
	/**
	 * Reply to a server ping event.
	 *
	 * Reply to the server ping event with the same seq.
	 *
	 * \param seq the seq number received in the ping event
	 */
	int (*pong) (void *object, uint32_t id, int seq);
	/**
	 * Fatal error event
         *
         * The error method is sent out when a fatal (non-recoverable)
         * error has occurred. The id argument is the proxy object where
         * the error occurred, most often in response to an event on that
         * object. The message is a brief description of the error,
         * for (debugging) convenience.
	 *
	 * This method is usually also emited on the resource object with
	 * \a id.
	 *
         * \param id object where the error occurred
         * \param res error code
         * \param message error description
	 */
	int (*error) (void *object, uint32_t id, int seq, int res, const char *message);
	/**
	 * Get the registry object
	 *
	 * Create a registry object that allows the client to list and bind
	 * the global objects available from the PipeWire server
	 * \param version the client version
	 * \param user_data_size extra size
	 */
	struct pw_registry * (*get_registry) (void *object, uint32_t version,
			size_t user_data_size);

	/**
	 * Create a new object on the PipeWire server from a factory.
	 *
	 * \param factory_name the factory name to use
	 * \param type the interface to bind to
	 * \param version the version of the interface
	 * \param props extra properties
	 * \param user_data_size extra size
	 */
	void * (*create_object) (void *object,
			       const char *factory_name,
			       uint32_t type,
			       uint32_t version,
			       const struct spa_dict *props,
			       size_t user_data_size);
	/**
	 * Destroy an resource
	 *
	 * Destroy the server resource for the given proxy.
	 *
	 * \param obj the proxy to destroy
	 */
	int (*destroy) (void *object, void *proxy);
};

#define pw_core_method(o,method,version,...)			\
({									\
	int _res = -ENOTSUP;						\
	spa_interface_call_res((struct spa_interface*)o,		\
			struct pw_core_methods, _res,		\
			method, version, ##__VA_ARGS__);		\
	_res;								\
})

#define pw_core_add_listener(c,...)	pw_core_method(c,add_listener,0,__VA_ARGS__)
#define pw_core_hello(c,...)		pw_core_method(c,hello,0,__VA_ARGS__)
#define pw_core_sync(c,...)		pw_core_method(c,sync,0,__VA_ARGS__)
#define pw_core_pong(c,...)		pw_core_method(c,pong,0,__VA_ARGS__)
#define pw_core_error(c,...)		pw_core_method(c,error,0,__VA_ARGS__)

static inline int
pw_core_errorv(struct pw_core *core, uint32_t id, int seq,
		int res, const char *message, va_list args)
{
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), message, args);
	buffer[1023] = '\0';
	return pw_core_error(core, id, seq, res, buffer);
}

static inline int
pw_core_errorf(struct pw_core *core, uint32_t id, int seq,
		int res, const char *message, ...)
{
        va_list args;
	int r;
	va_start(args, message);
	r = pw_core_errorv(core, id, seq, res, message, args);
	va_end(args);
	return r;
}

static inline struct pw_registry *
pw_core_get_registry(struct pw_core *core, uint32_t version, size_t user_data_size)
{
	struct pw_registry *res = NULL;
	spa_interface_call_res((struct spa_interface*)core,
			struct pw_core_methods, res,
			get_registry, 0, version, user_data_size);
	return res;
}

static inline void *
pw_core_create_object(struct pw_core *core,
			    const char *factory_name,
			    uint32_t type,
			    uint32_t version,
			    const struct spa_dict *props,
			    size_t user_data_size)
{
	void *res = NULL;
	spa_interface_call_res((struct spa_interface*)core,
			struct pw_core_methods, res,
			create_object, 0, factory_name,
			type, version, props, user_data_size);
	return res;
}

#define pw_core_destroy(c,...)		pw_core_method(c,destroy,0,__VA_ARGS__)

/** \page page_registry Registry
 *
 * \section page_registry_overview Overview
 *
 * The registry object is a singleton object that keeps track of
 * global objects on the PipeWire instance. See also \ref page_global.
 *
 * Global objects typically represent an actual object in PipeWire
 * (for example, a module or node) or they are singleton
 * objects such as the core.
 *
 * When a client creates a registry object, the registry object
 * will emit a global event for each global currently in the
 * registry.  Globals come and go as a result of device hotplugs or
 * reconfiguration or other events, and the registry will send out
 * global and global_remove events to keep the client up to date
 * with the changes.  To mark the end of the initial burst of
 * events, the client can use the pw_core.sync methosd immediately
 * after calling pw_core.get_registry.
 *
 * A client can bind to a global object by using the bind
 * request.  This creates a client-side proxy that lets the object
 * emit events to the client and lets the client invoke methods on
 * the object. See \ref page_proxy
 *
 * Clients can also change the permissions of the global objects that
 * it can see. This is interesting when you want to configure a
 * pipewire session before handing it to another application. You
 * can, for example, hide certain existing or new objects or limit
 * the access permissions on an object.
 */

#define PW_REGISTRY_EVENT_GLOBAL             0
#define PW_REGISTRY_EVENT_GLOBAL_REMOVE      1
#define PW_REGISTRY_EVENT_NUM                2

/** Registry events */
struct pw_registry_events {
#define PW_VERSION_REGISTRY_EVENTS	0
	uint32_t version;
	/**
	 * Notify of a new global object
	 *
	 * The registry emits this event when a new global object is
	 * available.
	 *
	 * \param id the global object id
	 * \param permissions the permissions of the object
	 * \param type the type of the interface
	 * \param version the version of the interface
	 * \param props extra properties of the global
	 */
	void (*global) (void *object, uint32_t id,
		       uint32_t permissions, uint32_t type, uint32_t version,
		       const struct spa_dict *props);
	/**
	 * Notify of a global object removal
	 *
	 * Emited when a global object was removed from the registry.
	 * If the client has any bindings to the global, it should destroy
	 * those.
	 *
	 * \param id the id of the global that was removed
	 */
	void (*global_remove) (void *object, uint32_t id);
};

#define PW_REGISTRY_METHOD_ADD_LISTENER	0
#define PW_REGISTRY_METHOD_BIND		1
#define PW_REGISTRY_METHOD_DESTROY	2
#define PW_REGISTRY_METHOD_NUM		3

/** Registry methods */
struct pw_registry_methods {
#define PW_VERSION_REGISTRY_METHODS	0
	uint32_t version;

	int (*add_listener) (void *object,
			struct spa_hook *listener,
			const struct pw_registry_events *events,
			void *data);
	/**
	 * Bind to a global object
	 *
	 * Bind to the global object with \a id and use the client proxy
	 * with new_id as the proxy. After this call, methods can be
	 * send to the remote global object and events can be received
	 *
	 * \param id the global id to bind to
	 * \param type the interface type to bind to
	 * \param version the interface version to use
	 * \returns the new object
	 */
	void * (*bind) (void *object, uint32_t id, uint32_t type, uint32_t version,
			size_t use_data_size);

	/**
	 * Attempt to destroy a global object
	 *
	 * Try to destroy the global object.
	 *
	 * \param id the global id to destroy
	 */
	int (*destroy) (void *object, uint32_t id);
};

#define pw_registry_method(o,method,version,...)			\
({									\
	int _res = -ENOTSUP;						\
	spa_interface_call_res((struct spa_interface*)o,		\
			struct pw_registry_methods, _res,		\
			method, version, ##__VA_ARGS__);		\
	_res;								\
})

/** Registry */
#define pw_registry_add_listener(p,...)	pw_registry_method(p,add_listener,0,__VA_ARGS__)

static inline void *
pw_registry_bind(struct pw_registry *registry,
		       uint32_t id, uint32_t type, uint32_t version,
		       size_t user_data_size)
{
	void *res = NULL;
	spa_interface_call_res((struct spa_interface*)registry,
			struct pw_registry_methods, res,
			bind, 0, id, type, version, user_data_size);
	return res;
}

#define pw_registry_destroy(p,...)	pw_registry_method(p,destroy,0,__VA_ARGS__)


/** Connect to a PipeWire instance \memberof pw_core
 * \return a pw_core on success or NULL with errno set on error */
struct pw_core *
pw_context_connect(struct pw_context *context,		/**< a \ref pw_context */
	      struct pw_properties *properties,	/**< optional properties, ownership of
						  *  the properties is taken.*/
	      size_t user_data_size		/**< extra user data size */);

/** Connect to a PipeWire instance on the given socket \memberof pw_core
 * \param fd the connected socket to use, the socket will be closed
 *	automatically on disconnect or error.
 * \return a pw_core on success or NULL with errno set on error */
struct pw_core *
pw_context_connect_fd(struct pw_context *context,	/**< a \ref pw_context */
	      int fd,				/**< an fd */
	      struct pw_properties *properties,	/**< optional properties, ownership of
						  *  the properties is taken.*/
	      size_t user_data_size		/**< extra user data size */);

/** Connect to a given PipeWire instance \memberof pw_core
 * \return a pw_core on success or NULL with errno set on error */
struct pw_core *
pw_context_connect_self(struct pw_context *context,	/**< a \ref pw_context to connect to */
	      struct pw_properties *properties,	/**< optional properties, ownership of
						  *  the properties is taken.*/
	      size_t user_data_size		/**< extra user data size */);

/** Steal the fd of the core connection or < 0 on error. The core
  * will be disconnected after this call. */
int pw_core_steal_fd(struct pw_core *core);

/** disconnect and destroy a core */
int pw_core_disconnect(struct pw_core *core);

/** Get the user_data. It is of the size specified when this object was
 * constructed */
void *pw_core_get_user_data(struct pw_core *core);

/** Get the client proxy */
struct pw_client_proxy * pw_core_get_client_proxy(struct pw_core *core);

/** Get the context object used to created this core */
struct pw_context * pw_core_get_context(struct pw_core *core);

/** Get properties from the core */
const struct pw_properties *pw_core_get_properties(struct pw_core *core);

/** Update the core properties. This updates the properties
 * of the associated client.
 * \return the number of properties that were updated */
int pw_core_update_properties(struct pw_core *core, const struct spa_dict *dict);

/** Get the core mempool object */
struct pw_mempool * pw_core_get_mempool(struct pw_core *core);

/** Get the proxy with the given id */
struct pw_proxy *pw_core_find_proxy(struct pw_core *core, uint32_t id);

/** Export an object into the PipeWire instance associated with core */
struct pw_proxy *pw_core_export(struct pw_core *core,			/**< the core */
				  uint32_t type,			/**< the type of object */
				  struct pw_properties *properties,	/**< extra properties */
				  void *object,				/**< object to export */
				  size_t user_data_size			/**< extra user data */);


#ifdef __cplusplus
}
#endif

#endif /* PIPEWIRE_CORE_H */
