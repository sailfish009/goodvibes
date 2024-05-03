/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2024 Arnaud Rebillout
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <glib.h>
#include <glib-object.h>

void log_init(const gchar *log_level, gboolean colorless, const gchar *output_file);
void log_cleanup(void);
void log_msg(GLogLevelFlags level, const gchar *file, const gchar *func, const gchar *fmt, ...);
void log_trace(const gchar *file, const gchar *func, const gchar *fmt, ...);
void log_trace_property_access(const gchar *file, const gchar *func, GObject *object,
                               guint property_id, const GValue *value, GParamSpec *pspec,
                               gboolean print_value);

/*
 * Wrappers to GLib message logging functions.
 * Use that for logs intended for developers.
 */

#define ERROR(fmt, ...)    do { \
                log_msg(G_LOG_LEVEL_ERROR,    __FILE__, __func__, fmt, ##__VA_ARGS__); \
                __builtin_unreachable(); \
        } while (0)

#define CRITICAL(fmt, ...) log_msg(G_LOG_LEVEL_CRITICAL, __FILE__, __func__, fmt, ##__VA_ARGS__)
#define WARNING(fmt, ...)  log_msg(G_LOG_LEVEL_WARNING,  __FILE__, __func__, fmt, ##__VA_ARGS__)
#define INFO(fmt, ...)     log_msg(G_LOG_LEVEL_INFO,     __FILE__, __func__, fmt, ##__VA_ARGS__)
#define DEBUG(fmt, ...)    log_msg(G_LOG_LEVEL_DEBUG,    __FILE__, __func__, fmt, ##__VA_ARGS__)
#define DEBUG_NO_CONTEXT(fmt, ...) log_msg(G_LOG_LEVEL_DEBUG, NULL, NULL, fmt, ##__VA_ARGS__)

#define TRACE(fmt, ...)    log_trace(__FILE__, __func__, fmt, ##__VA_ARGS__)
#define TRACE_GET_PROPERTY(obj, prop_id, value, pspec) \
        log_trace_property_access(__FILE__, __func__, obj, prop_id, value, pspec, FALSE)
#define TRACE_SET_PROPERTY(obj, prop_id, value, pspec) \
        log_trace_property_access(__FILE__, __func__, obj, prop_id, value, pspec, TRUE)
