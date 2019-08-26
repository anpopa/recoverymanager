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
static void post_executor_event (RmgExecutor *executor, ExecutorEventType type, RmgDEvent *dispatcher_event);

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
 * @brief Process service restart event
 */
static void do_process_service_restart_event (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Process service reset public data event
 */
static void do_process_service_reset_public_data_event (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Process service reset private data event
 */
static void do_process_service_reset_private_data_event (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Process service disable event
 */
static void do_process_service_disable_event (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Process service context restart event
 */
static void do_process_context_restart_event (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Process service platform restart event
 */
static void do_process_platform_restart_event (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Process service factory reset event
 */
static void do_process_factory_reset_event (RmgExecutor *executor, RmgDEvent *dispatcher_event);

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
                     RmgDEvent *dispatcher_event)
{
  RmgExecutorEvent *e = NULL;

  g_assert (executor);
  g_assert (dispatcher_event);

  e = g_new0 (RmgExecutorEvent, 1);

  e->type = type;
  e->dispatcher_event = rmg_devent_ref (dispatcher_event);

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

  switch (event->type)
    {
    case EXECUTOR_EVENT_SERVICE_RESTART:
      do_process_service_restart_event (executor, event->dispatcher_event);
      break;

    case EXECUTOR_EVENT_SERVICE_RESET_PUBLIC_DATA:
      do_process_service_reset_public_data_event (executor, event->dispatcher_event);
      break;

    case EXECUTOR_EVENT_SERVICE_RESET_PRIVATE_DATA:
      do_process_service_reset_private_data_event (executor, event->dispatcher_event);
      break;

    case EXECUTOR_EVENT_SERVICE_DISABLE:
      do_process_service_disable_event (executor, event->dispatcher_event);
      break;

    case EXECUTOR_EVENT_CONTEXT_RESTART:
      do_process_context_restart_event (executor, event->dispatcher_event);
      break;

    case EXECUTOR_EVENT_PLATFORM_RESTART:
      do_process_platform_restart_event (executor, event->dispatcher_event);
      break;

    case EXECUTOR_EVENT_FACTORY_RESET:
      do_process_factory_reset_event (executor, event->dispatcher_event);
      break;

    default:
      break;
    }

  rmg_devent_unref (event->dispatcher_event);
  g_free (event);

  return TRUE;
}

static void
do_process_service_reset_public_data_event (RmgExecutor *executor,
                                            RmgDEvent *dispatcher_event)
{
  RMG_UNUSED (executor);
  RMG_UNUSED (dispatcher_event);
}

static void
do_process_service_reset_private_data_event (RmgExecutor *executor,
                                             RmgDEvent *dispatcher_event)
{
  RMG_UNUSED (executor);
  RMG_UNUSED (dispatcher_event);
}

static void
do_process_service_disable_event (RmgExecutor *executor,
                                  RmgDEvent *dispatcher_event)
{
  RMG_UNUSED (executor);
  RMG_UNUSED (dispatcher_event);
}

static void
do_process_context_restart_event (RmgExecutor *executor,
                                  RmgDEvent *dispatcher_event)
{
  RMG_UNUSED (executor);
  RMG_UNUSED (dispatcher_event);
}

static void
do_process_platform_restart_event (RmgExecutor *executor,
                                   RmgDEvent *dispatcher_event)
{
  RMG_UNUSED (executor);
  RMG_UNUSED (dispatcher_event);
}

static void
do_process_factory_reset_event (RmgExecutor *executor,
                                RmgDEvent *dispatcher_event)
{
  RMG_UNUSED (executor);
  RMG_UNUSED (dispatcher_event);
}

static void
do_process_service_restart_event (RmgExecutor *executor,
                                  RmgDEvent *dispatcher_event)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GVariant) response;

  g_assert (executor);
  g_assert (dispatcher_event);

  response = g_dbus_proxy_call_sync (dispatcher_event->manager_proxy,
                                     "RestartUnit",
                                     g_variant_new ("(ss)",
                                                    dispatcher_event->service_name,
                                                    "replace"),
                                     G_DBUS_CALL_FLAGS_NONE,
                                     -1,
                                     NULL,
                                     &error);


  if (error != NULL)
    {
      g_warning ("Fail to call RestartUnit on Manager proxy. Error %s",
                 error->message);
    }
  else
    {
      g_info ("Request service restart for unit='%s'", dispatcher_event->service_name);
    }
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
rmg_executor_new (RmgOptions *options, RmgJournal *journal)
{
  RmgExecutor *executor = (RmgExecutor *)g_source_new (&executor_source_funcs, sizeof(RmgExecutor));

  g_assert (executor);

  g_ref_count_init (&executor->rc);

  executor->callback = executor_source_callback;
  executor->options = rmg_options_ref (options);
  executor->journal = rmg_journal_ref (journal);

  if (g_run_mode == RUN_MODE_SLAVE)
    executor->manager = rmg_manager_new (options);

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
      if (executor->options != NULL)
        rmg_options_unref (executor->options);

      if (executor->journal != NULL)
        rmg_journal_unref (executor->journal);

      if (executor->manager != NULL)
        rmg_manager_unref (executor->manager);

      g_async_queue_unref (executor->queue);
      g_source_unref (RMG_EVENT_SOURCE (executor));
    }
}

void
rmg_executor_push_event (RmgExecutor *executor,
                         ExecutorEventType type,
                         RmgDEvent *dispatcher_event)
{
  post_executor_event (executor, type, dispatcher_event);
}
