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
 * \file rmg-devent.h
 */

#pragma once

#include "rmg-types.h"

#include <gio/gio.h>
#include <glib.h>

G_BEGIN_DECLS

/**
 * @enum devent_data
 * @brief Dispatcher event data type
 */
typedef enum _DispatcherEventType
{
  DEVENT_UNKNOWN,
  DEVENT_INFORM_PROCESS_CRASH,
  DEVENT_INFORM_SERVICE_FAILED,
  DEVENT_SERVICE_CRASHED,
  DEVENT_SERVICE_RESTARTED,
  DEVENT_REMOTE_CONTEXT_RESTART,
  DEVENT_REMOTE_PLATFORM_RESTART,
  DEVENT_REMOTE_FACTORY_RESET
} DispatcherEventType;

/**
 * @struct RmgDEvent
 * @brief The dispatcher event
 */
typedef struct _RmgDEvent
{
  DispatcherEventType type;  /**< The event type the element holds */
  gchar *service_name;       /**< Service name for the event */
  gchar *process_name;       /**< Proccess name for the event */
  gchar *object_path;        /**< Service object path */
  gchar *context_name;       /**< Service context name */
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
 * @brief Set dispatcher event process name
 */
void rmg_devent_set_process_name (RmgDEvent *event, const gchar *process_name);

/**
 * @brief Set dispatcher event object path
 */
void rmg_devent_set_object_path (RmgDEvent *event, const gchar *object_path);

/**
 * @brief Set dispatcher event context (container) name
 */
void rmg_devent_set_context_name (RmgDEvent *event, const gchar *context_name);

/**
 * @brief Set dispatcher event manager proxy
 */
void rmg_devent_set_manager_proxy (RmgDEvent *event, GDBusProxy *manager_proxy);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgDEvent, rmg_devent_unref);

G_END_DECLS
