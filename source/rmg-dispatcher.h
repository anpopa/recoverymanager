/* rmg-dispatcher.h
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

#include "rmg-options.h"
#include "rmg-journal.h"
#include "rmg-executor.h"
#include "rmg-server.h"
#include "rmg-manager.h"
#include "rmg-types.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @enum dispatcher_event_data
 * @brief Dispatcher event data type
 */
typedef enum _DispatcherEventType {
  DISPATCHER_EVENT_SERVICE_UPDATE,
  DISPATCHER_EVENT_SERVICE_RELAXED
} DispatcherEventType;

/**
 * @function RmgDispatcherCallback
 * @brief Custom callback used internally by RmgDispatcher as source callback
 */
typedef gboolean (*RmgDispatcherCallback) (gpointer _dispatcher, gpointer _event);

/**
 * @struct RmgDispatcherEvent
 * @brief The file transfer event
 */
typedef struct _RmgDispatcherEvent {
  DispatcherEventType type;     /**< The event type the element holds */
  gchar *service_name;     /**< Service name for the event */
} RmgDispatcherEvent;
/**
 * @struct RmgDispatcher
 * @brief The RmgDispatcher opaque data structure
 */
typedef struct _RmgDispatcher {
  GSource source;  /**< Event loop source */
  GAsyncQueue    *queue;  /**< Async queue */
  RmgDispatcherCallback callback; /**< Dispatcher callback function */
  RmgOptions *options;
  RmgJournal *journal;
  RmgExecutor *executor;
  RmgServer *server;
  RmgManager *manager;
  grefcount rc;     /**< Reference counter variable  */
} RmgDispatcher;

/*
 * @brief Create a new dispatcher object
 * @return On success return a new RmgDispatcher object otherwise return NULL
 */
RmgDispatcher *rmg_dispatcher_new (RmgOptions *options, RmgJournal *journal, RmgExecutor *executor, GError **error);

/**
 * @brief Aquire dispatcher object
 * @param c Pointer to the dispatcher object
 */
RmgDispatcher *rmg_dispatcher_ref (RmgDispatcher *dispatcher);

/**
 * @brief Release dispatcher object
 * @param c Pointer to the dispatcher object
 */
void rmg_dispatcher_unref (RmgDispatcher *dispatcher);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgDispatcher, rmg_dispatcher_unref);

G_END_DECLS
