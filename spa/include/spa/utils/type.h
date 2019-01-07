/* Simple Plugin API
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

#ifndef __SPA_TYPE_H__
#define __SPA_TYPE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <spa/utils/defs.h>

enum {
	/* Basic types */
	SPA_TYPE_START = 0x00000,
	SPA_TYPE_None,
        SPA_TYPE_Bool,
        SPA_TYPE_Id,
        SPA_TYPE_Int,
        SPA_TYPE_Long,
        SPA_TYPE_Float,
        SPA_TYPE_Double,
        SPA_TYPE_String,
        SPA_TYPE_Bytes,
        SPA_TYPE_Rectangle,
        SPA_TYPE_Fraction,
        SPA_TYPE_Bitmap,
        SPA_TYPE_Array,
        SPA_TYPE_Struct,
        SPA_TYPE_Object,
        SPA_TYPE_Sequence,
        SPA_TYPE_Pointer,
        SPA_TYPE_Fd,
        SPA_TYPE_Choice,
        SPA_TYPE_Pod,
	SPA_TYPE_LAST,				/**< not part of ABI */

	/* Pointers */
	SPA_TYPE_POINTER_START = 0x10000,
	SPA_TYPE_POINTER_Buffer,
	SPA_TYPE_POINTER_Meta,
	SPA_TYPE_POINTER_Dict,
	SPA_TYPE_POINTER_LAST,			/**< not part of ABI */

	/* Interfaces */
	SPA_TYPE_INTERFACE_START = 0x20000,
	SPA_TYPE_INTERFACE_Handle,		/**< object handle */
	SPA_TYPE_INTERFACE_HandleFactory,	/**< factory for object handles */
	SPA_TYPE_INTERFACE_Log,			/**< log interface */
	SPA_TYPE_INTERFACE_Loop,		/**< poll loop support */
	SPA_TYPE_INTERFACE_LoopControl,		/**< control of loops */
	SPA_TYPE_INTERFACE_LoopUtils,		/**< loop utilities */
	SPA_TYPE_INTERFACE_DataLoop,		/**< a data loop */
	SPA_TYPE_INTERFACE_MainLoop,		/**< a main loop */
	SPA_TYPE_INTERFACE_DBus,		/**< dbus connection */
	SPA_TYPE_INTERFACE_Monitor,		/**< monitor of devices */
	SPA_TYPE_INTERFACE_Node,		/**< nodes for data processing */
	SPA_TYPE_INTERFACE_Device,		/**< device managing nodes */
	SPA_TYPE_INTERFACE_CPU,			/**< CPU functions */
	SPA_TYPE_INTERFACE_LAST,		/**< not part of ABI */

	/* Events */
	SPA_TYPE_EVENT_START = 0x30000,
	SPA_TYPE_EVENT_Monitor,
	SPA_TYPE_EVENT_Node,
	SPA_TYPE_EVENT_LAST,			/**< not part of ABI */

	/* Commands */
	SPA_TYPE_COMMAND_START = 0x40000,
	SPA_TYPE_COMMAND_Node,
	SPA_TYPE_COMMAND_LAST,			/**< not part of ABI */

	/* Objects */
	SPA_TYPE_OBJECT_START = 0x50000,
	SPA_TYPE_OBJECT_MonitorItem,
	SPA_TYPE_OBJECT_ParamList,
	SPA_TYPE_OBJECT_PropInfo,
	SPA_TYPE_OBJECT_Props,
	SPA_TYPE_OBJECT_Format,
	SPA_TYPE_OBJECT_ParamBuffers,
	SPA_TYPE_OBJECT_ParamMeta,
	SPA_TYPE_OBJECT_ParamIO,
	SPA_TYPE_OBJECT_ParamProfile,
	SPA_TYPE_OBJECT_LAST,			/**< not part of ABI */

	/* vendor extensions */
	SPA_TYPE_VENDOR_PipeWire	= 0x02000000,

	SPA_TYPE_VENDOR_Other		= 0x7f000000,
};


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* __SPA_TYPE_H__ */
