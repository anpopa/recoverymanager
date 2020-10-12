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
 * \file rmg-dispatcher.h
 */

#pragma once

#include "rmg-devent.h"
#include "rmg-executor.h"
#include "rmg-journal.h"
#include "rmg-manager.h"
#include "rmg-options.h"
#include "rmg-server.h"
#include "rmg-types.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @function RmgDispatcherCallback
 * @brief Custom callback used internally by RmgDispatcher as source callback
 */
typedef gboolean        (*RmgDispatcherCallback)            (gpointer _dispatcher,
                                                             gpointer _event);

/**
 * @struct RmgDispatcher
 * @brief The RmgDispatcher opaque data structure
 */
typedef struct _RmgDispatcher {
  GSource source;                 /**< Event loop source */
  GAsyncQueue *queue;             /**< Async queue */
  RmgDispatcherCallback callback; /**< Dispatcher callback function */
  RmgOptions *options;
  RmgJournal *journal;
  RmgExecutor *executor;
  RmgServer *server;
  RmgManager *manager;
  grefcount rc;
} RmgDispatcher;

/*
 * @brief Create a new dispatcher object
 * @return On success return a new RmgDispatcher object otherwise return NULL
 */
RmgDispatcher *         rmg_dispatcher_new                  (RmgOptions *options,
                                                             RmgJournal *journal,
                                                             RmgExecutor *executor,
                                                             GError **error);

/**
 * @brief Aquire dispatcher object
 * @param c Pointer to the dispatcher object
 */
RmgDispatcher *         rmg_dispatcher_ref                  (RmgDispatcher *dispatcher);

/**
 * @brief Release dispatcher object
 * @param c Pointer to the dispatcher object
 */
void                    rmg_dispatcher_unref                (RmgDispatcher *dispatcher);

/**
 * @brief Push a service event
 * @param c Pointer to the dispatcher object
 */
void                    rmg_dispatcher_push_service_event   (RmgDispatcher *dispatcher,
                                                             RmgDEvent *event);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgDispatcher, rmg_dispatcher_unref);

G_END_DECLS
