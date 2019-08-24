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
#include <gio.h>

G_BEGIN_DECLS

typedef enum _ServiceActiveState {
    SERVICE_STATE_UNKNOWN,
    SERVICE_STATE_ACTIVE,
    SERVICE_STATE_RELOADING,
    SERVICE_STATE_INACTIVE,
    SERVICE_STATE_FAILED,
    SERVICE_STATE_ACTIVATING,
    SERVICE_STATE_DEACTIVATING
} ServiceActiveState;

typedef enum _ServiceActiveSubstate {
    SERVICE_SUBSTATE_UNKNOWN,
    SERVICE_SUBSTATE_RUNNING,
    SERVICE_SUBSTATE_DEAD,
    SERVICE_SUBSTATE_STOP_SIGTERM
} ServiceActiveSubstate;

/**
 * @struct Service entry
 * @brief Reprezentation of a service with state from systemd
 */
typedef struct _MonitorServiceEntry {
  GDBusProxy *proxy;
  gchar *service_name;
  gchar *object_path;
  ServiceActiveState active_state;
  ServiceActiveSubstate active_substate;
} MonitorServiceEntry;

/**
 * @struct RmgMonitor
 * @brief The RmgMonitor opaque data structure
 */
typedef struct _RmgMonitor {
  RmgDispatcher *dispatcher;
  grefcount rc;     /**< Reference counter variable  */
  GList *services;
  GDBusProxy *proxy;
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
