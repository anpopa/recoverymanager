/*
 * SPDX license identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2019-2020 Alin Popa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * \author Alin Popa <alin.popa@fxdata.ro>
 * \file rmg-crashmonitor.h
 */

#pragma once

#include "rmg-dispatcher.h"
#include "rmg-types.h"

#include <gio/gio.h>
#include <glib.h>

G_BEGIN_DECLS

/**
 * @enum crashmonitor_event_data
 * @brief CrashMonitor event data type
 */
typedef enum _CrashMonitorEventType { CRASHMONITOR_EVENT_BUILD_PROXY } CrashMonitorEventType;

typedef gboolean (*RmgCrashMonitorCallback)(gpointer _crashmonitor, gpointer _event);

typedef void (*RmgCrashMonitorProxyAvailableCb)(gpointer _dbus_proxy, gpointer _data);

/**
 * @struct RmgCrashMonitorEvent
 * @brief The file transfer event
 */
typedef struct _RmgCrashMonitorEvent {
    CrashMonitorEventType type; /**< The event type the element holds */
} RmgCrashMonitorEvent;

/**
 * @struct RmgCrashMonitorNotifyProxy
 * @brief The file transfer event
 */
typedef struct _RmgCrashMonitorNotifyProxy {
    RmgCrashMonitorProxyAvailableCb callback;
    gpointer data; /**< Data object to pass with the callback */
} RmgCrashMonitorNotifyProxy;

/**
 * @struct RmgCrashMonitor
 * @brief The RmgCrashMonitor opaque data structure
 */
typedef struct _RmgCrashMonitor {
    GSource source;                   /**< Event loop source */
    GAsyncQueue *queue;               /**< Async queue */
    RmgCrashMonitorCallback callback; /**< Callback function */
    RmgDispatcher *dispatcher;
    grefcount rc;
    GList *notify_proxy;
    GDBusProxy *proxy;
} RmgCrashMonitor;

/*
 * @brief Create a new crashmonitor object
 * @return On success return a new RmgCrashMonitor object otherwise return NULL
 */
RmgCrashMonitor *rmg_crashmonitor_new(RmgDispatcher *dispatcher);

/**
 * @brief Aquire crashmonitor object
 * @param c Pointer to the crashmonitor object
 */
RmgCrashMonitor *rmg_crashmonitor_ref(RmgCrashMonitor *crashmonitor);

/**
 * @brief Release crashmonitor object
 * @param c Pointer to the crashmonitor object
 */
void rmg_crashmonitor_unref(RmgCrashMonitor *crashmonitor);

/**
 * @brief Build DBus proxy
 * @param crashmonitor Pointer to the crashmonitor object
 */
void rmg_crashmonitor_build_proxy(RmgCrashMonitor *crashmonitor);

/**
 * @brief Get a reference to manager proxy
 * @param crashmonitor Pointer to the crashmonitor object
 */
GDBusProxy *rmg_crashmonitor_get_manager_proxy(RmgCrashMonitor *crashmonitor);

/**
 * @brief Get existing services
 * @param crashmonitor Pointer to the crashmonitor object
 */
void rmg_crashmonitor_register_proxy_cb(RmgCrashMonitor *crashmonitor,
                                        RmgCrashMonitorProxyAvailableCb cb,
                                        gpointer data);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(RmgCrashMonitor, rmg_crashmonitor_unref);

G_END_DECLS
