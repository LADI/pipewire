/* Spa */
/* SPDX-FileCopyrightText: Copyright © 2018 Wim Taymans */
/* SPDX-License-Identifier: MIT */

#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <fnmatch.h>

#include <spa/support/log.h>
#include <spa/support/loop.h>
#include <spa/support/system.h>
#include <spa/support/plugin.h>
#include <spa/utils/ringbuffer.h>
#include <spa/utils/type.h>
#include <spa/utils/names.h>
#include <spa/utils/string.h>
#include <spa/utils/ansi.h>

#if defined(__FreeBSD__) || defined(__MidnightBSD__)
#define CLOCK_MONOTONIC_RAW CLOCK_MONOTONIC
#elif defined(_MSC_VER)
static inline void setlinebuf(FILE* stream) {
	setvbuf(stream, NULL, _IOLBF, 0);
}
#endif

#undef SPA_LOG_TOPIC_DEFAULT
#define SPA_LOG_TOPIC_DEFAULT &log_topic
SPA_LOG_TOPIC_DEFINE_STATIC(log_topic, "spa.logger");

#define DEFAULT_LOG_LEVEL SPA_LOG_LEVEL_INFO

#define TRACE_BUFFER (16*1024)

struct impl {
	struct spa_handle handle;
	struct spa_log log;

	FILE *file;
	bool close_file;

	struct spa_system *system;
	struct spa_source source;
	struct spa_ringbuffer trace_rb;
	uint8_t trace_data[TRACE_BUFFER];

	clockid_t clock_id;

	unsigned int have_source:1;
	unsigned int colors:1;
	unsigned int timestamp:1;
	unsigned int local_timestamp:1;
	unsigned int line:1;
};

static SPA_PRINTF_FUNC(7,0) void
impl_log_logtv(void *object,
	      enum spa_log_level level,
	      const struct spa_log_topic *topic,
	      const char *file,
	      int line,
	      const char *func,
	      const char *fmt,
	      va_list args)
{
#define RESERVED_LENGTH 24

	struct impl *impl = object;
	char timestamp[18] = {0};
	char topicstr[32] = {0};
	char filename[64] = {0};
	char location[1000 + RESERVED_LENGTH], *p, *s;
	static const char * const levels[] = { "-", "E", "W", "I", "D", "T", "*T*" };
	const char *prefix = "", *suffix = "";
	int size, len;
	bool do_trace;

	if ((do_trace = (level == SPA_LOG_LEVEL_TRACE && impl->have_source)))
		level++;

	if (impl->colors) {
		if (level <= SPA_LOG_LEVEL_ERROR)
			prefix = SPA_ANSI_BOLD_RED;
		else if (level <= SPA_LOG_LEVEL_WARN)
			prefix = SPA_ANSI_BOLD_YELLOW;
		else if (level <= SPA_LOG_LEVEL_INFO)
			prefix = SPA_ANSI_BOLD_GREEN;
		if (prefix[0])
			suffix = SPA_ANSI_RESET;
	}

	p = location;
	len = sizeof(location) - RESERVED_LENGTH;

	if (impl->local_timestamp) {
		char buf[64];
		struct timespec now;
		struct tm now_tm;

		clock_gettime(impl->clock_id, &now);
		localtime_r(&now.tv_sec, &now_tm);
		strftime(buf, sizeof(buf), "%H:%M:%S", &now_tm);
		spa_scnprintf(timestamp, sizeof(timestamp), "[%s.%06d]", buf,
				(int)(now.tv_nsec / SPA_NSEC_PER_USEC));
	} else if (impl->timestamp) {
		struct timespec now;
		clock_gettime(impl->clock_id, &now);
		spa_scnprintf(timestamp, sizeof(timestamp), "[%05jd.%06jd]",
			(intmax_t) (now.tv_sec & 0x1FFFFFFF) % 100000, (intmax_t) now.tv_nsec / 1000);
	}

	if (topic && topic->topic)
		spa_scnprintf(topicstr, sizeof(topicstr), " %-12s | ", topic->topic);


	if (impl->line && line != 0) {
		s = strrchr(file, '/');
		spa_scnprintf(filename, sizeof(filename), "[%16.16s:%5i %s()]",
			s ? s + 1 : file, line, func);
	}

	size = spa_scnprintf(p, len, "%s[%s]%s%s%s ", prefix, levels[level],
			     timestamp, topicstr, filename);
	/*
	 * it is assumed that at this point `size` <= `len`,
	 * which is reasonable as long as file names and function names
	 * don't become very long
	 */
	size += spa_vscnprintf(p + size, len - size, fmt, args);

	/*
	 * `RESERVED_LENGTH` bytes are reserved for printing the suffix
	 * (at the moment it's "... (truncated)\x1B[0m\n" at its longest - 21 bytes),
	 * its length must be less than `RESERVED_LENGTH` (including the null byte),
	 * otherwise a stack buffer overrun could ensue
	 */

	/* if the message could not fit entirely... */
	if (size >= len - 1) {
		size = len - 1; /* index of the null byte */
		len = sizeof(location);
		size += spa_scnprintf(p + size, len - size, "... (truncated)");
	}
	else {
		len = sizeof(location);
	}

	size += spa_scnprintf(p + size, len - size, "%s\n", suffix);

	if (SPA_UNLIKELY(do_trace)) {
		uint32_t index;

		spa_ringbuffer_get_write_index(&impl->trace_rb, &index);
		spa_ringbuffer_write_data(&impl->trace_rb, impl->trace_data, TRACE_BUFFER,
					  index & (TRACE_BUFFER - 1), location, size);
		spa_ringbuffer_write_update(&impl->trace_rb, index + size);

		if (spa_system_eventfd_write(impl->system, impl->source.fd, 1) < 0)
			fprintf(impl->file, "error signaling eventfd: %s\n", strerror(errno));
	} else
		fputs(location, impl->file);

#undef RESERVED_LENGTH
}

static SPA_PRINTF_FUNC(6,0) void
impl_log_logv(void *object,
	      enum spa_log_level level,
	      const char *file,
	      int line,
	      const char *func,
	      const char *fmt,
	      va_list args)
{
	impl_log_logtv(object, level, NULL, file, line, func, fmt, args);
}

static SPA_PRINTF_FUNC(7,8) void
impl_log_logt(void *object,
	     enum spa_log_level level,
	     const struct spa_log_topic *topic,
	     const char *file,
	     int line,
	     const char *func,
	     const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	impl_log_logtv(object, level, topic, file, line, func, fmt, args);
	va_end(args);
}

static SPA_PRINTF_FUNC(6,7) void
impl_log_log(void *object,
	     enum spa_log_level level,
	     const char *file,
	     int line,
	     const char *func,
	     const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	impl_log_logtv(object, level, NULL, file, line, func, fmt, args);
	va_end(args);
}

static void on_trace_event(struct spa_source *source)
{
	struct impl *impl = source->data;
	int32_t avail;
	uint32_t index;
	uint64_t count;

	if (spa_system_eventfd_read(impl->system, source->fd, &count) < 0)
		fprintf(impl->file, "failed to read event fd: %s", strerror(errno));

	while ((avail = spa_ringbuffer_get_read_index(&impl->trace_rb, &index)) > 0) {
		int32_t offset, first;

		if (avail > TRACE_BUFFER) {
			index += avail - TRACE_BUFFER;
			avail = TRACE_BUFFER;
		}
		offset = index & (TRACE_BUFFER - 1);
		first = SPA_MIN(avail, TRACE_BUFFER - offset);

		fwrite(impl->trace_data + offset, first, 1, impl->file);
		if (SPA_UNLIKELY(avail > first)) {
			fwrite(impl->trace_data, avail - first, 1, impl->file);
		}
		spa_ringbuffer_read_update(&impl->trace_rb, index + avail);
        }
}

static const struct spa_log_methods impl_log = {
	SPA_VERSION_LOG_METHODS,
	.log = impl_log_log,
	.logv = impl_log_logv,
	.logt = impl_log_logt,
	.logtv = impl_log_logtv,
};

static int impl_get_interface(struct spa_handle *handle, const char *type, void **interface)
{
	struct impl *this;

	spa_return_val_if_fail(handle != NULL, -EINVAL);
	spa_return_val_if_fail(interface != NULL, -EINVAL);

	this = (struct impl *) handle;

	if (spa_streq(type, SPA_TYPE_INTERFACE_Log))
		*interface = &this->log;
	else
		return -ENOENT;

	return 0;
}

static int impl_clear(struct spa_handle *handle)
{
	struct impl *this;

	spa_return_val_if_fail(handle != NULL, -EINVAL);

	this = (struct impl *) handle;

	if (this->close_file && this->file != NULL)
		fclose(this->file);

	if (this->have_source) {
		spa_loop_remove_source(this->source.loop, &this->source);
		spa_system_close(this->system, this->source.fd);
		this->have_source = false;
	}
	return 0;
}

static size_t
impl_get_size(const struct spa_handle_factory *factory,
	      const struct spa_dict *params)
{
	return sizeof(struct impl);
}

static int
impl_init(const struct spa_handle_factory *factory,
	  struct spa_handle *handle,
	  const struct spa_dict *info,
	  const struct spa_support *support,
	  uint32_t n_support)
{
	struct impl *this;
	struct spa_loop *loop = NULL;
	const char *str, *dest = "";
	bool linebuf = false;
	bool force_colors = false;

	spa_return_val_if_fail(factory != NULL, -EINVAL);
	spa_return_val_if_fail(handle != NULL, -EINVAL);

	handle->get_interface = impl_get_interface;
	handle->clear = impl_clear;

	this = (struct impl *) handle;

	this->log.iface = SPA_INTERFACE_INIT(
			SPA_TYPE_INTERFACE_Log,
			SPA_VERSION_LOG,
			&impl_log, this);
	this->log.level = DEFAULT_LOG_LEVEL;

	loop = spa_support_find(support, n_support, SPA_TYPE_INTERFACE_Loop);
	this->system = spa_support_find(support, n_support, SPA_TYPE_INTERFACE_System);

	if (loop != NULL && this->system != NULL) {
		this->source.func = on_trace_event;
		this->source.data = this;
		this->source.fd = spa_system_eventfd_create(this->system, SPA_FD_CLOEXEC | SPA_FD_NONBLOCK);
		this->source.mask = SPA_IO_IN;
		this->source.rmask = 0;

		if (this->source.fd < 0) {
			fprintf(stderr, "Warning: failed to create eventfd: %m");
		} else {
			spa_loop_add_source(loop, &this->source);
			this->have_source = true;
		}
	}
	if (info) {
		str = spa_dict_lookup(info, SPA_KEY_LOG_TIMESTAMP);
		if (spa_atob(str) || spa_streq(str, "local")) {
			this->clock_id = CLOCK_REALTIME;
			this->local_timestamp = true;
		} else if (spa_streq(str, "monotonic")) {
			this->clock_id = CLOCK_MONOTONIC;
			this->timestamp = true;
		} else if (spa_streq(str, "monotonic-raw")) {
			this->clock_id = CLOCK_MONOTONIC_RAW;
			this->timestamp = true;
		} else if (spa_streq(str, "realtime")) {
			this->clock_id = CLOCK_REALTIME;
			this->timestamp = true;
		}
		if ((str = spa_dict_lookup(info, SPA_KEY_LOG_LINE)) != NULL)
			this->line = spa_atob(str);
		if ((str = spa_dict_lookup(info, SPA_KEY_LOG_COLORS)) != NULL) {
			if (spa_streq(str, "force")) {
				this->colors = true;
				force_colors = true;
			} else {
				this->colors = spa_atob(str);
			}
		}
		if ((str = spa_dict_lookup(info, SPA_KEY_LOG_LEVEL)) != NULL)
			this->log.level = atoi(str);
		if ((str = spa_dict_lookup(info, SPA_KEY_LOG_FILE)) != NULL) {
			dest = str;
			if (spa_streq(str, "stderr"))
				this->file = stderr;
			else if (spa_streq(str, "stdout"))
				this->file = stdout;
			else {
				this->file = fopen(str, "we");
				if (this->file == NULL)
					fprintf(stderr, "Warning: failed to open file %s: (%m)", str);
				else
					this->close_file = true;
			}
		}
	}
	if (this->file == NULL) {
		this->file = stderr;
		dest = "stderr";
	} else {
		linebuf = true;
	}
	if (linebuf)
		setlinebuf(this->file);

	if (this->colors && !force_colors && !isatty(fileno(this->file)) ) {
		this->colors = false;
	}

	spa_ringbuffer_init(&this->trace_rb);

	spa_log_debug(&this->log, "%p: initialized to %s linebuf:%u", this, dest, linebuf);

	return 0;
}

static const struct spa_interface_info impl_interfaces[] = {
	{SPA_TYPE_INTERFACE_Log,},
};

static int
impl_enum_interface_info(const struct spa_handle_factory *factory,
			 const struct spa_interface_info **info,
			 uint32_t *index)
{
	spa_return_val_if_fail(factory != NULL, -EINVAL);
	spa_return_val_if_fail(info != NULL, -EINVAL);
	spa_return_val_if_fail(index != NULL, -EINVAL);

	switch (*index) {
	case 0:
		*info = &impl_interfaces[*index];
		break;
	default:
		return 0;
	}
	(*index)++;

	return 1;
}

const struct spa_handle_factory spa_support_logger_factory = {
	SPA_VERSION_HANDLE_FACTORY,
	.name = SPA_NAME_SUPPORT_LOG,
	.info = NULL,
	.get_size = impl_get_size,
	.init = impl_init,
	.enum_interface_info = impl_enum_interface_info,
};
