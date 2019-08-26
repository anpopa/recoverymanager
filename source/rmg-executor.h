/* rmg-executor.h
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

#include "rmg-devent.h"
#include "rmg-options.h"
#include "rmg-journal.h"
#include "rmg-manager.h"
#include "rmg-types.h"

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

/**
 * @enum executor_event_data
 * @brief Executor event data type
 */
typedef enum _ExecutorEventType {
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
typedef struct _RmgExecutorEvent {
  ExecutorEventType type;      /**< The event type the element holds */
  RmgDEvent *dispatcher_event; /**< Event object from dispatcher */
} RmgExecutorEvent;

/**
 * @struct RmgExecutor
 * @brief The RmgExecutor opaque data structure
 */
typedef struct _RmgExecutor {
  GSource source;  /**< Event loop source */
  RmgOptions *options;
  RmgJournal *journal;
  RmgManager *manager;
  GAsyncQueue    *queue;  /**< Async queue */
  RmgExecutorCallback callback; /**< Callback function */
  grefcount rc;     /**< Reference counter variable  */
} RmgExecutor;

/*
 * @brief Create a new executor object
 * @return On success return a new RmgExecutor object otherwise return NULL
 */
RmgExecutor *rmg_executor_new (RmgOptions *options, RmgJournal *journal);

/**
 * @brief Aquire executor object
 * @param c Pointer to the executor object
 */
RmgExecutor *rmg_executor_ref (RmgExecutor *executor);

/**
 * @brief Release executor object
 * @param c Pointer to the executor object
 */
void rmg_executor_unref (RmgExecutor *executor);

/**
 * @brief Push an executor event
 */
void rmg_executor_push_event (RmgExecutor *executor, ExecutorEventType type, RmgDEvent *dispatcher_event);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgExecutor, rmg_executor_unref);

G_END_DECLS
