/* rmg-dispatcher.c
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

#include "rmg-dispatcher.h"
#include "rmg-relaxtimer.h"
#include "rmg-utils.h"

/**
 * @brief Post new event
 */
static void post_dispatcher_event (RmgDispatcher *dispatcher, RmgDEvent *event);

/**
 * @brief GSource prepare function
 */
static gboolean dispatcher_source_prepare (GSource *source, gint *timeout);

/**
 * @brief GSource dispatch function
 */
static gboolean dispatcher_source_dispatch (GSource *source, GSourceFunc callback, gpointer _dispatcher);

/**
 * @brief GSource callback function
 */
static gboolean dispatcher_source_callback (gpointer _dispatcher, gpointer _event);

/**
 * @brief GSource destroy notification callback function
 */
static void dispatcher_source_destroy_notify (gpointer _dispatcher);

/**
 * @brief Async queue destroy notification callback function
 */
static void dispatcher_queue_destroy_notify (gpointer _dispatcher);

/**
 * @brief Handle service crash event
 */
static void do_process_service_crash_event (RmgDispatcher *dispatcher, RmgDEvent *event);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs dispatcher_source_funcs =
{
  dispatcher_source_prepare,
  NULL,
  dispatcher_source_dispatch,
  NULL,
  NULL,
  NULL,
};

static void
post_dispatcher_event (RmgDispatcher *dispatcher, RmgDEvent *event)
{
  g_assert (dispatcher);
  g_assert (event);

  g_async_queue_push (dispatcher->queue, event);
}

static gboolean
dispatcher_source_prepare (GSource *source,
                           gint *timeout)
{
  RmgDispatcher *dispatcher = (RmgDispatcher *)source;

  RMG_UNUSED (timeout);

  return(g_async_queue_length (dispatcher->queue) > 0);
}

static gboolean
dispatcher_source_dispatch (GSource *source,
                            GSourceFunc callback,
                            gpointer _dispatcher)
{
  RmgDispatcher *dispatcher = (RmgDispatcher *)source;
  gpointer event = g_async_queue_try_pop (dispatcher->queue);

  RMG_UNUSED (callback);
  RMG_UNUSED (_dispatcher);

  if (event == NULL)
    return G_SOURCE_CONTINUE;

  return dispatcher->callback (dispatcher, event) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
dispatcher_source_callback (gpointer _dispatcher,
                            gpointer _event)
{
  RmgDispatcher *dispatcher = (RmgDispatcher *)_dispatcher;
  RmgDEvent *event = (RmgDEvent *)_event;

  g_assert (dispatcher);
  g_assert (event);

  switch (event->type)
    {
    case DISPATCHER_EVENT_SERVICE_CRASHED:
      g_info ("Service '%s' crash event detected", event->service_name);
      do_process_service_crash_event (dispatcher, event);
      break;

    case DISPATCHER_EVENT_SERVICE_RESTARTED:
      g_info ("Service '%s' restarted", event->service_name);
      break;

    case DISPATCHER_EVENT_REMOTE_CONTEXT_RESTART:
      g_info ("Service '%s' from container '%s' request container restart",
              event->service_name,
              event->context_name);

      rmg_executor_push_event (dispatcher->executor, EXECUTOR_EVENT_CONTEXT_RESTART, event);
      break;

    case DISPATCHER_EVENT_REMOTE_PLATFORM_RESTART:
      g_info ("Service '%s' from container '%s' request platform restart",
              event->service_name,
              event->context_name);

      rmg_executor_push_event (dispatcher->executor, EXECUTOR_EVENT_PLATFORM_RESTART, event);
      break;

    case DISPATCHER_EVENT_REMOTE_FACTORY_RESET:
      g_info ("Service '%s' from container '%s' request factory reset",
              event->service_name,
              event->context_name);

      rmg_executor_push_event (dispatcher->executor, EXECUTOR_EVENT_FACTORY_RESET, event);
      break;

    default:
      break;
    }

  rmg_devent_unref (event);

  return TRUE;
}

static void
dispatcher_source_destroy_notify (gpointer _dispatcher)
{
  RmgDispatcher *dispatcher = (RmgDispatcher *)_dispatcher;

  g_assert (dispatcher);
  g_debug ("Dispatcher destroy notification");

  rmg_dispatcher_unref (dispatcher);
}

static void
dispatcher_queue_destroy_notify (gpointer _dispatcher)
{
  RMG_UNUSED (_dispatcher);
  g_debug ("Dispatcher queue destroy notification");
}

static RmgStatus
run_mode_specific_init (RmgDispatcher *dispatcher, GError **error)
{
  RmgStatus status = RMG_STATUS_OK;

  g_assert (dispatcher);

  if (g_run_mode == RUN_MODE_MASTER)
    {
      dispatcher->server = rmg_server_new (dispatcher->options, dispatcher, error);

      if (*error != NULL)
        {
          status = RMG_STATUS_ERROR;
        }
      else
        {
          status = rmg_server_bind_and_listen (dispatcher->server);

          if (status != RMG_STATUS_OK)
            {
              g_set_error (error,
                           g_quark_from_static_string ("DispatcherNew"),
                           1,
                           "Cannot bind and listen in server mode");
            }
        }
    }

  return status;
}

static void
do_process_service_crash_event (RmgDispatcher *dispatcher, RmgDEvent *event)
{
  RmgActionType action_type = ACTION_INVALID;

  g_autoptr (GError) error = NULL;
  gulong service_hash = 0;
  glong rvector = 0;

  g_assert (dispatcher);
  g_assert (event);

  /* check if a recovery unit is available */
  service_hash = rmg_journal_get_hash (dispatcher->journal, event->service_name, &error);
  if (error != NULL)
    {
      g_warning ("Fail to get service hash %s. Error %s", event->service_name, error->message);
      return;
    }
  else
    {
      if (service_hash == 0)
        {
          g_info ("No recovery unit defined for crashed service='%s'", event->service_name);
          return;
        }
    }

  /* we increment the rvector for this service */
  rvector = rmg_journal_get_rvector (dispatcher->journal, event->service_name, &error);
  if (error != NULL)
    {
      g_warning ("Fail to read the rvector for service %s. Error %s",
                 event->service_name,
                 error->message);
      return;
    }

  rmg_journal_set_rvector (dispatcher->journal, event->service_name, (rvector + 1), &error);
  if (error != NULL)
    {
      g_warning ("Fail to increment the rvector for service %s. Error %s",
                 event->service_name,
                 error->message);
      return;
    }

  rvector += 1;

  /* read next applicable action and execute */
  action_type = rmg_journal_get_service_action (dispatcher->journal, event->service_name, &error);
  if (error != NULL)
    {
      g_warning ("Fail to read next service action %s. Error %s",
                 event->service_name,
                 error->message);
      return;
    }

  if (action_type != ACTION_INVALID)
    {
      g_info ("Action '%s' requiered for service='%s' rvector=%ld",
              rmg_utils_action_name (action_type),
              event->service_name,
              rvector);
    }

  switch (action_type)
    {
    case ACTION_SERVICE_RESET:
      rmg_executor_push_event (dispatcher->executor, EXECUTOR_EVENT_SERVICE_RESTART, event);
      break;

    case ACTION_PUBLIC_DATA_RESET:
      rmg_executor_push_event (dispatcher->executor, EXECUTOR_EVENT_SERVICE_RESET_PUBLIC_DATA, event);
      break;

    case ACTION_PRIVATE_DATA_RESET:
      rmg_executor_push_event (dispatcher->executor, EXECUTOR_EVENT_SERVICE_RESET_PRIVATE_DATA, event);
      break;

    case ACTION_SERVICE_DISABLE:
      rmg_executor_push_event (dispatcher->executor, EXECUTOR_EVENT_SERVICE_DISABLE, event);
      break;

    case ACTION_CONTEXT_RESET:
      rmg_executor_push_event (dispatcher->executor, EXECUTOR_EVENT_CONTEXT_RESTART, event);
      break;

    case ACTION_PLATFORM_RESTART:
      rmg_executor_push_event (dispatcher->executor, EXECUTOR_EVENT_PLATFORM_RESTART, event);
      break;

    case ACTION_FACTORY_RESET:
      rmg_executor_push_event (dispatcher->executor, EXECUTOR_EVENT_FACTORY_RESET, event);
      break;

    case ACTION_INVALID:
    default:
      break;
    }

  rmg_relaxtimer_trigger (dispatcher->journal, event->service_name, &error);
  if (error != NULL)
    {
      g_warning ("Fail to trigger relax timer for service %s. Error %s",
                 event->service_name,
                 error->message);
    }
}

RmgDispatcher *
rmg_dispatcher_new (RmgOptions *options,
                    RmgJournal *journal,
                    RmgExecutor *executor,
                    GError **error)
{
  RmgDispatcher *dispatcher = (RmgDispatcher *)g_source_new (&dispatcher_source_funcs,
                                                             sizeof(RmgDispatcher));

  g_assert (dispatcher);

  g_ref_count_init (&dispatcher->rc);

  dispatcher->callback = dispatcher_source_callback;
  dispatcher->options = rmg_options_ref (options);
  dispatcher->journal = rmg_journal_ref (journal);
  dispatcher->executor = rmg_executor_ref (executor);
  dispatcher->queue = g_async_queue_new_full (dispatcher_queue_destroy_notify);

  if (run_mode_specific_init (dispatcher, error) == RMG_STATUS_OK)
    {
      g_source_set_callback (RMG_EVENT_SOURCE (dispatcher),
                             NULL,
                             dispatcher,
                             dispatcher_source_destroy_notify);
      g_source_attach (RMG_EVENT_SOURCE (dispatcher), NULL);
    }

  return dispatcher;
}

RmgDispatcher *
rmg_dispatcher_ref (RmgDispatcher *dispatcher)
{
  g_assert (dispatcher);
  g_ref_count_inc (&dispatcher->rc);
  return dispatcher;
}

void
rmg_dispatcher_unref (RmgDispatcher *dispatcher)
{
  g_assert (dispatcher);

  if (g_ref_count_dec (&dispatcher->rc) == TRUE)
    {
      g_autofree gchar *sock_addr = NULL;

      if (g_run_mode == RUN_MODE_MASTER)
        {
          sock_addr = rmg_options_string_for (dispatcher->options, KEY_IPC_SOCK_ADDR);
          unlink (sock_addr);
        }

      if (dispatcher->options != NULL)
        rmg_options_unref (dispatcher->options);

      if (dispatcher->journal != NULL)
        rmg_journal_unref (dispatcher->journal);

      if (dispatcher->executor != NULL)
        rmg_executor_unref (dispatcher->executor);

      if (dispatcher->server != NULL)
        rmg_server_unref (dispatcher->server);

      g_async_queue_unref (dispatcher->queue);
      g_source_unref (RMG_EVENT_SOURCE (dispatcher));
    }
}

void
rmg_dispatcher_push_service_event (RmgDispatcher *dispatcher, RmgDEvent *event)
{
  post_dispatcher_event (dispatcher, event);
}
