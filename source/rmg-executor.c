/* rmg-executor.c
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

#include "rmg-executor.h"

/**
 * @brief Post new event
 *
 * @param executor A pointer to the executor object
 * @param type The type of the new event to be posted
 * @param service_name The service name having this event
 */
static void post_executor_event (RmgExecutor *executor, ExecutorEventType type, const gchar *service_name);

/**
 * @brief GSource prepare function
 */
static gboolean executor_source_prepare (GSource *source, gint *timeout);

/**
 * @brief GSource dispatch function
 */
static gboolean executor_source_dispatch (GSource *source, GSourceFunc callback, gpointer _executor);

/**
 * @brief GSource callback function
 */
static gboolean executor_source_callback (gpointer _executor, gpointer _event);

/**
 * @brief GSource destroy notification callback function
 */
static void executor_source_destroy_notify (gpointer _executor);

/**
 * @brief Async queue destroy notification callback function
 */
static void executor_queue_destroy_notify (gpointer _executor);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs executor_source_funcs =
{
  executor_source_prepare,
  NULL,
  executor_source_dispatch,
  NULL,
  NULL,
  NULL,
};

static void
post_executor_event (RmgExecutor *executor,
                     ExecutorEventType type,
                     const gchar *service_name)
{
  RmgExecutorEvent *e = NULL;

  g_assert (executor);
  g_assert (service_name);

  e = g_new0 (RmgExecutorEvent, 1);

  e->type = type;
  e->service_name = g_strdup (service_name);

  g_async_queue_push (executor->queue, e);
}

static gboolean
executor_source_prepare (GSource *source,
                         gint *timeout)
{
  RmgExecutor *executor = (RmgExecutor *)source;

  RMG_UNUSED (timeout);

  return(g_async_queue_length (executor->queue) > 0);
}

static gboolean
executor_source_dispatch (GSource *source,
                          GSourceFunc callback,
                          gpointer _executor)
{
  RmgExecutor *executor = (RmgExecutor *)source;
  gpointer event = g_async_queue_try_pop (executor->queue);

  RMG_UNUSED (callback);
  RMG_UNUSED (_executor);

  if (event == NULL)
    return G_SOURCE_CONTINUE;

  return executor->callback (executor, event) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
executor_source_callback (gpointer _executor,
                          gpointer _event)
{
  RmgExecutor *executor = (RmgExecutor *)_executor;
  RmgExecutorEvent *event = (RmgExecutorEvent *)_event;

  g_assert (executor);
  g_assert (event);


  /* TODO: Process the event */

  g_free (event->service_name);
  g_free (event);

  return TRUE;
}

static void
executor_source_destroy_notify (gpointer _executor)
{
  RmgExecutor *executor = (RmgExecutor *)_executor;

  g_assert (executor);
  g_debug ("Executor destroy notification");

  rmg_executor_unref (executor);
}

static void
executor_queue_destroy_notify (gpointer _executor)
{
  RMG_UNUSED (_executor);
  g_debug ("Executor queue destroy notification");
}

RmgExecutor *
rmg_executor_new (void)
{
  RmgExecutor *executor = (RmgExecutor *)g_source_new (&executor_source_funcs, sizeof(RmgExecutor));

  g_assert (executor);

  g_ref_count_init (&executor->rc);

  executor->callback = executor_source_callback;
  executor->queue = g_async_queue_new_full (executor_queue_destroy_notify);

  g_source_set_callback (RMG_EVENT_SOURCE (executor),
                         NULL,
                         executor,
                         executor_source_destroy_notify);
  g_source_attach (RMG_EVENT_SOURCE (executor), NULL);

  return executor;
}

RmgExecutor *
rmg_executor_ref (RmgExecutor *executor)
{
  g_assert (executor);
  g_ref_count_inc (&executor->rc);
  return executor;
}

void
rmg_executor_unref (RmgExecutor *executor)
{
  g_assert (executor);

  if (g_ref_count_dec (&executor->rc) == TRUE)
    {
      g_async_queue_unref (executor->queue);
      g_source_unref (RMG_EVENT_SOURCE (executor));
    }
}

