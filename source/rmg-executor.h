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
 * \file rmg-executor.h
 */

#pragma once

#include "rmg-devent.h"
#include "rmg-journal.h"
#include "rmg-manager.h"
#include "rmg-options.h"
#include "rmg-server.h"
#include "rmg-types.h"

#include <gio/gio.h>
#include <glib.h>

G_BEGIN_DECLS

/**
 * @enum executor_event_data
 * @brief Executor event data type
 */
typedef enum _ExecutorEventType
{
  EXECUTOR_EVENT_FRIEND_PROCESS_CRASH,
  EXECUTOR_EVENT_FRIEND_SERVICE_FAILED,
  EXECUTOR_EVENT_SERVICE_RESTART,
  EXECUTOR_EVENT_SERVICE_RESET_PUBLIC_DATA,
  EXECUTOR_EVENT_SERVICE_RESET_PRIVATE_DATA,
  EXECUTOR_EVENT_SERVICE_DISABLE,
  EXECUTOR_EVENT_CONTEXT_RESTART,
  EXECUTOR_EVENT_PLATFORM_RESTART,
  EXECUTOR_EVENT_FACTORY_RESET
} ExecutorEventType;

/**
 * @function RmgExecutorCallback
 * @brief Custom callback used internally by RmgExecutor as source callback
 */
typedef gboolean (*RmgExecutorCallback) (gpointer _executor, gpointer _event);

/**
 * @struct RmgExecutorEvent
 * @brief The file transfer event
 */
typedef struct _RmgExecutorEvent
{
  ExecutorEventType type;      /**< The event type the element holds */
  RmgDEvent *dispatcher_event; /**< Event object from dispatcher */
} RmgExecutorEvent;

/**
 * @struct RmgExecutor
 * @brief The RmgExecutor opaque data structure
 */
typedef struct _RmgExecutor
{
  GSource source; /**< Event loop source */
  RmgOptions *options;
  RmgJournal *journal;
  RmgManager *manager;
  RmgServer *server;
  GDBusProxy *sd_manager_proxy;
  GAsyncQueue *queue;           /**< Async queue */
  RmgExecutorCallback callback; /**< Callback function */
  grefcount rc;
} RmgExecutor;

/*
 * @brief Create a new executor object
 * @return On success return a new RmgExecutor object otherwise return NULL
 */
RmgExecutor *rmg_executor_new (RmgOptions *options, RmgJournal *journal);

/**
 * @brief Aquire executor object
 * @param executor Pointer to the executor object
 */
RmgExecutor *rmg_executor_ref (RmgExecutor *executor);

/**
 * @brief Release executor object
 * @param executor Pointer to the executor object
 */
void rmg_executor_unref (RmgExecutor *executor);

/**
 * @brief Set replica manager
 * @param executor Pointer to the executor object
 * @param manager Pointer to the manager object
 */
void rmg_executor_set_replica_manager (RmgExecutor *executor, RmgManager *manager);

/**
 * @brief Set primary server
 * @param executor Pointer to the executor object
 * @param server Pointer to the server object
 */
void rmg_executor_set_primary_server (RmgExecutor *executor, RmgServer *server);

/**
 * @brief Push an executor event
 */
void rmg_executor_push_event (RmgExecutor *executor, ExecutorEventType type,
                              RmgDEvent *dispatcher_event);
/**
 * @brief Build DBus proxy
 * @param executor Pointer to the executor object
 */
void rmg_executor_set_proxy (RmgExecutor *executor, GDBusProxy *dbus_proxy);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgExecutor, rmg_executor_unref);

G_END_DECLS
