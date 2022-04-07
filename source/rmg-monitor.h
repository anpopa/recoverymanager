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
 * \file rmg-monitor.c
 */

#pragma once

#include "rmg-dispatcher.h"
#include "rmg-types.h"

#include <gio/gio.h>
#include <glib.h>

G_BEGIN_DECLS

/**
 * @enum monitor_event_data
 * @brief Monitor event data type
 */
typedef enum _MonitorEventType
{
  MONITOR_EVENT_BUILD_PROXY,
  MONITOR_EVENT_READ_SERVICES
} MonitorEventType;

typedef gboolean (*RmgMonitorCallback) (gpointer _monitor, gpointer _event);

typedef void (*RmgMonitorProxyAvailableCallback) (gpointer _dbus_proxy, gpointer _data);

/**
 * @struct RmgMonitorEvent
 * @brief The file transfer event
 */
typedef struct _RmgMonitorEvent
{
  MonitorEventType type; /**< The event type the element holds */
} RmgMonitorEvent;

/**
 * @struct RmgMonitorNotifyProxy
 * @brief The file transfer event
 */
typedef struct _RmgMonitorNotifyProxy
{
  RmgMonitorProxyAvailableCallback callback; /**< The callback for proxy available */
  gpointer data;                             /**< Data object to pass with the callback */
} RmgMonitorNotifyProxy;

/**
 * @struct RmgMonitor
 * @brief The RmgMonitor opaque data structure
 */
typedef struct _RmgMonitor
{
  GSource source;              /**< Event loop source */
  GAsyncQueue *queue;          /**< Async queue */
  RmgMonitorCallback callback; /**< Callback function */
  RmgDispatcher *dispatcher;
  grefcount rc; /**< Reference counter variable  */
  GList *notify_proxy;
  GList *services;
  GDBusProxy *proxy;
} RmgMonitor;

/*
 * @brief Create a new monitor object
 * @return On success return a new RmgMonitor object otherwise return NULL
 */
RmgMonitor *rmg_monitor_new (RmgDispatcher *dispatcher);

/**
 * @brief Aquire monitor object
 * @param c Pointer to the monitor object
 */
RmgMonitor *rmg_monitor_ref (RmgMonitor *monitor);

/**
 * @brief Release monitor object
 * @param c Pointer to the monitor object
 */
void rmg_monitor_unref (RmgMonitor *monitor);

/**
 * @brief Build DBus proxy
 * @param monitor Pointer to the monitor object
 */
void rmg_monitor_build_proxy (RmgMonitor *monitor);

/**
 * @brief Get a reference to manager proxy
 * @param monitor Pointer to the monitor object
 */
GDBusProxy *rmg_monitor_get_manager_proxy (RmgMonitor *monitor);

/**
 * @brief Get existing services
 * @param monitor Pointer to the monitor object
 */
void rmg_monitor_read_services (RmgMonitor *monitor);

/**
 * @brief Get existing services
 * @param monitor Pointer to the monitor object
 */
void rmg_monitor_register_proxy_available_callback (RmgMonitor *monitor,
                                                    RmgMonitorProxyAvailableCallback cb,
                                                    gpointer data);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgMonitor, rmg_monitor_unref);

G_END_DECLS
