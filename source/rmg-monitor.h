/* rmg-monitor.h
 *
 * Copyright 2019 Alin Popa <alin.popa@fxdata.ro>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written
 * authorization.
 */

#pragma once

#include "rmg-dispatcher.h"
#include "rmg-types.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @enum monitor_event_data
 * @brief Monitor event data type
 */
typedef union _MonitorEventType {
  MONITOR_EVENT_SERVICE_UPDATE,
  MONITOR_EVENT_SERVICE_RELAXED
} MonitorEventType;

/**
 * @function RmgMonitorCallback
 * @brief Custom callback used internally by RmgMonitor as source callback
 */
typedef gboolean (*RmgMonitorCallback) (gpointer _monitor, gpointer _event);

/**
 * @struct RmgMonitorEvent
 * @brief The file transfer event
 */
typedef struct _RmgMonitorEvent {
  MonitorEventType type;     /**< The event type the element holds */
  gchar service_name;     /**< Service name for the event */
} RmgMonitorEvent;

/**
 * @struct RmgMonitor
 * @brief The RmgMonitor opaque data structure
 */
typedef struct _RmgMonitor {
  GSource source;  /**< Event loop source */
  GAsyncQueue    *queue;  /**< Async queue */
  RmgDispatcher *dispatcher;
  grefcount rc;     /**< Reference counter variable  */
} RmgMonitor;

/*
 * @brief Create a new monitor object
 * @return On success return a new RmgMonitor object otherwise return NULL
 */
RmgMonitor *rmg_monitor_new (RmgDispatcher *dispatcher, GError **error);

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

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgMonitor, rmg_monitor_unref);

G_END_DECLS
