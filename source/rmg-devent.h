/* rmg-devent.h
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

#include "rmg-types.h"

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

/**
 * @enum devent_data
 * @brief Dispatcher event data type
 */
typedef enum _DispatcherEventType {
  DISPATCHER_EVENT_UNKNOWN,
  DISPATCHER_EVENT_SERVICE_INACTIVE,
  DISPATCHER_EVENT_SERVICE_ACTIVE,
  DISPATCHER_EVENT_SERVICE_RELAXED
} DispatcherEventType;

/**
 * @struct RmgDEvent
 * @brief The dispatcher event
 */
typedef struct _RmgDEvent {
  DispatcherEventType type;  /**< The event type the element holds */
  gchar *service_name;       /**< Service name for the event */
  gchar *object_path;        /**< Service object path */
  GDBusProxy *manager_proxy; /**< Systemd manager proxy */
  grefcount rc;              /**< Reference counter variable  */
} RmgDEvent;

/*
 * @brief Create a new dispatcher event object
 * @return The new RmgDispatcher object
 */
RmgDEvent *rmg_devent_new (DispatcherEventType type);

/**
 * @brief Aquire dispatcher event object
 * @param event Pointer to the dispatcher eventobject
 */
RmgDEvent *rmg_devent_ref (RmgDEvent *event);

/**
 * @brief Release dispatcher event object
 * @param event Pointer to the dispatcher object
 */
void rmg_devent_unref (RmgDEvent *event);

/**
 * @brief Set dispatcher event type
 */
void rmg_devent_set_type (RmgDEvent *event, DispatcherEventType type);

/**
 * @brief Set dispatcher event service name
 */
void rmg_devent_set_service_name (RmgDEvent *event, const gchar *service_name);

/**
 * @brief Set dispatcher event object path
 */
void rmg_devent_set_object_path (RmgDEvent *event, const gchar *object_path);

/**
 * @brief Set dispatcher event manager proxy
 */
void rmg_devent_set_manager_proxy (RmgDEvent *event, GDBusProxy *manager_proxy);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgDEvent, rmg_devent_unref);

G_END_DECLS
