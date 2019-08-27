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

#ifdef WITH_LXC
#include <lxc/lxccontainer.h>
#endif

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
 * @brief Process service context restart master
 */
static void do_process_context_restart_event_master (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Process service context restart slave
 */
static void do_process_context_restart_event_slave (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Process service platform restart event
 */
static void do_process_platform_restart_event (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Process service platform restart event master
 */
static void do_process_platform_restart_event_master (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Process service platform restart event slave
 */
static void do_process_platform_restart_event_slave (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Process service factory reset event
 */
static void do_process_factory_reset_event (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Process service factory reset event master
 */
static void do_process_factory_reset_event_master (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Process service factory reset event slave
 */
static void do_process_factory_reset_event_slave (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Enter meditation state
 */
static void enter_meditation (RmgExecutor *executor, RmgDEvent *dispatcher_event);

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
enter_meditation (RmgExecutor *executor, RmgDEvent *dispatcher_event)
{
  g_assert (executor);
  g_assert (dispatcher_event);

  g_info ("Recoverymanager enter meditation state after executing action for service='%s'",
          dispatcher_event->service_name);

  raise (SIGTERM);
}

static void
do_process_service_reset_public_data_event (RmgExecutor *executor,
                                            RmgDEvent *dispatcher_event)
{
  g_autoptr (GError) error = NULL;
  g_autofree gchar *reset_path = NULL;
  g_autofree gchar *reset_cmd = NULL;
  g_autofree gchar *reset_with_path_cmd = NULL;
  g_autofree gchar *reset_with_name_cmd = NULL;
  g_autofree gchar *standard_output = NULL;
  g_autofree gchar *command_line = NULL;
  gchar **path_tokens = NULL;
  gchar **name_tokens = NULL;
  gint exit_status;

  g_assert (executor);
  g_assert (dispatcher_event);

  reset_path = rmg_journal_get_public_data_path (executor->journal,
                                                 dispatcher_event->service_name,
                                                 &error);
  if (error != NULL)
    {
      g_warning ("Fail to read public data path for service %s. Error %s",
                 dispatcher_event->service_name,
                 error->message);
      return;
    }

  reset_cmd = rmg_options_string_for (executor->options, KEY_PUBLIC_DATA_RESET_CMD);

  path_tokens = g_strsplit (reset_cmd, "${path}", 3);
  reset_with_path_cmd = g_strjoinv (reset_path, path_tokens);
  g_strfreev (path_tokens);

  name_tokens = g_strsplit (reset_with_path_cmd, "${service_name}", 3);
  reset_with_name_cmd = g_strjoinv (dispatcher_event->service_name, name_tokens);
  g_strfreev (name_tokens);

  g_info ("Reset public data for service='%s' command='%s'",
          dispatcher_event->service_name,
          reset_with_name_cmd);

  command_line = g_strdup_printf ("sh -c \"%s\"", reset_with_name_cmd);

  g_spawn_command_line_sync (command_line, &standard_output, NULL, &exit_status, &error);
  if (error != NULL)
    g_warning ("Fail to spawn process. Error %s", error->message);
  else
    g_info ("Public data reset exitcode=%d output='%s'", exit_status, standard_output);

  /* do service restart */
  do_process_service_restart_event (executor, dispatcher_event);
}

static void
do_process_service_reset_private_data_event (RmgExecutor *executor,
                                             RmgDEvent *dispatcher_event)
{
  g_autoptr (GError) error = NULL;
  g_autofree gchar *reset_path = NULL;
  g_autofree gchar *reset_cmd = NULL;
  g_autofree gchar *reset_with_path_cmd = NULL;
  g_autofree gchar *reset_with_name_cmd = NULL;
  g_autofree gchar *standard_output = NULL;
  g_autofree gchar *command_line = NULL;
  gchar **path_tokens = NULL;
  gchar **name_tokens = NULL;
  gint exit_status;

  g_assert (executor);
  g_assert (dispatcher_event);

  reset_path = rmg_journal_get_private_data_path (executor->journal,
                                                  dispatcher_event->service_name,
                                                  &error);
  if (error != NULL)
    {
      g_warning ("Fail to read private data path for service %s. Error %s",
                 dispatcher_event->service_name,
                 error->message);
      return;
    }

  reset_cmd = rmg_options_string_for (executor->options, KEY_PRIVATE_DATA_RESET_CMD);

  path_tokens = g_strsplit (reset_cmd, "${path}", 3);
  reset_with_path_cmd = g_strjoinv (reset_path, path_tokens);
  g_strfreev (path_tokens);

  name_tokens = g_strsplit (reset_with_path_cmd, "${service_name}", 3);
  reset_with_name_cmd = g_strjoinv (dispatcher_event->service_name, name_tokens);
  g_strfreev (name_tokens);

  g_info ("Reset private data for service='%s' command='%s'",
          dispatcher_event->service_name,
          reset_with_name_cmd);

  command_line = g_strdup_printf ("sh -c \"%s\"", reset_with_name_cmd);

  g_spawn_command_line_sync (command_line, &standard_output, NULL, &exit_status, &error);
  if (error != NULL)
    g_warning ("Fail to spawn process. Error %s", error->message);
  else
    g_info ("Private data reset exitcode=%d output='%s'", exit_status, standard_output);

  /* do service restart */
  do_process_service_restart_event (executor, dispatcher_event);
}

static void
do_process_service_disable_event (RmgExecutor *executor,
                                  RmgDEvent *dispatcher_event)
{
  RMG_UNUSED (executor);
  g_info ("Service '%s' remain disable this lifecycle", dispatcher_event->service_name);
}

static void
do_process_context_restart_event (RmgExecutor *executor,
                                  RmgDEvent *dispatcher_event)
{
  if (g_run_mode == RUN_MODE_MASTER)
    {
      if (dispatcher_event->context_name == NULL)
        do_process_platform_restart_event_master (executor, dispatcher_event);
      else
        do_process_context_restart_event_master (executor, dispatcher_event);
    }
  else
    {
      do_process_context_restart_event_slave (executor, dispatcher_event);
    }
}

static void
do_process_context_restart_event_master (RmgExecutor *executor,
                                         RmgDEvent *dispatcher_event)
{
  g_autofree gchar *service_name = NULL;

  g_autoptr (GError) error = NULL;
#ifdef WITH_LXC
  g_autofree struct lxc_container *container = NULL;
#endif
  gulong service_hash = 0;

  g_assert (executor);
  g_assert (dispatcher_event);

  service_name = g_strdup_printf ("%s.service", dispatcher_event->context_name);

  /* check if a recovery unit is available */
  service_hash = rmg_journal_get_hash (executor->journal, service_name, &error);
  if (error != NULL)
    {
      g_warning ("Fail to get service hash %s. Error %s", service_name, error->message);
      return;
    }
  else
    {
      if (service_hash == 0)
        g_info ("No recovery unit defined for container service='%s'", service_name);
    }

  g_info ("Request container '%s' reboot", dispatcher_event->context_name);
#ifdef WITH_LXC
  container = lxc_container_new (dispatcher_event->context_name, NULL);
  if (container != NULL)
    {
      if (!container->is_running (container))
        g_warning ("Container %s not running", dispatcher_event->context_name);

      if (!container->shutdown (container, 10))
        g_warning ("Fail to reboot container %s", dispatcher_event->context_name);
    }
#endif
}

static void
do_process_context_restart_event_slave (RmgExecutor *executor,
                                        RmgDEvent *dispatcher_event)
{
  RmgMessage msg;
  RmgMessageRequestContextRestart data;
  const gchar *hostname = g_get_host_name ();

  g_assert (executor);
  g_assert (dispatcher_event);
  g_assert (executor->manager);

  memset (&data, 0, sizeof(RmgMessageRequestContextRestart));

  if (!rmg_manager_connected (executor->manager))
    {
      if (rmg_manager_connect (executor->manager) != RMG_STATUS_OK)
        {
          g_warning ("Fail to connect to master instance");
          return;
        }
    }

  rmg_message_init (&msg, RMG_REQUEST_CONTEXT_RESTART, 0);

  memcpy (&data.service_name,
          dispatcher_event->service_name,
          strlen (dispatcher_event->service_name) + 1);
  memcpy (&data.context_name,
          hostname,
          strlen (hostname) + 1);

  rmg_message_set_data (&msg, &data, sizeof(data));

  if (rmg_manager_send (executor->manager, &msg) != RMG_STATUS_OK)
    g_warning ("Fail to send context restart event to master");
  else
    enter_meditation (executor, dispatcher_event);
}

static void
do_process_platform_restart_event_master (RmgExecutor *executor,
                                          RmgDEvent *dispatcher_event)
{
  g_autoptr (GError) error = NULL;
  g_autofree gchar *reset_cmd = NULL;
  g_autofree gchar *standard_output = NULL;
  g_autofree gchar *command_line = NULL;
  gint exit_status;

  g_assert (executor);
  g_assert (dispatcher_event);

  reset_cmd = rmg_options_string_for (executor->options, KEY_PLATFORM_RESTART_CMD);

  g_info ("Do platform restart on service='%s' request. Command='%s'",
          dispatcher_event->service_name,
          reset_cmd);

  command_line = g_strdup_printf ("sh -c \"%s\"", reset_cmd);

  g_spawn_command_line_sync (command_line, &standard_output, NULL, &exit_status, &error);
  if (error != NULL)
    g_warning ("Fail to spawn process. Error %s", error->message);
  else
    g_info ("Platform restart exitcode=%d output='%s'", exit_status, standard_output);
}

static void
do_process_platform_restart_event_slave (RmgExecutor *executor,
                                         RmgDEvent *dispatcher_event)
{
  RmgMessage msg;
  RmgMessageRequestPlatformRestart data;
  const gchar *hostname = g_get_host_name ();

  g_assert (executor);
  g_assert (dispatcher_event);
  g_assert (executor->manager);

  memset (&data, 0, sizeof(RmgMessageRequestPlatformRestart));

  if (!rmg_manager_connected (executor->manager))
    {
      if (rmg_manager_connect (executor->manager) != RMG_STATUS_OK)
        {
          g_warning ("Fail to connect to master instance");
          return;
        }
    }

  rmg_message_init (&msg, RMG_REQUEST_PLATFORM_RESTART, 0);

  memcpy (&data.service_name,
          dispatcher_event->service_name,
          strlen (dispatcher_event->service_name) + 1);
  memcpy (&data.context_name,
          hostname,
          strlen (hostname) + 1);

  rmg_message_set_data (&msg, &data, sizeof(data));

  if (rmg_manager_send (executor->manager, &msg) != RMG_STATUS_OK)
    g_warning ("Fail to send platform restart event to master");
}

static void
do_process_platform_restart_event (RmgExecutor *executor,
                                   RmgDEvent *dispatcher_event)
{
  if (g_run_mode == RUN_MODE_MASTER)
    do_process_platform_restart_event_master (executor, dispatcher_event);
  else
    do_process_platform_restart_event_slave (executor, dispatcher_event);

  enter_meditation (executor, dispatcher_event);
}

static void
do_process_factory_reset_event_master (RmgExecutor *executor,
                                       RmgDEvent *dispatcher_event)
{
  g_autoptr (GError) error = NULL;
  g_autofree gchar *reset_cmd = NULL;
  g_autofree gchar *standard_output = NULL;
  g_autofree gchar *command_line = NULL;
  gint exit_status;

  g_assert (executor);
  g_assert (dispatcher_event);

  reset_cmd = rmg_options_string_for (executor->options, KEY_FACTORY_RESET_CMD);

  g_info ("Do factory reset on service='%s' request. Command='%s'",
          dispatcher_event->service_name,
          reset_cmd);

  command_line = g_strdup_printf ("sh -c \"%s\"", reset_cmd);

  g_spawn_command_line_sync (command_line, &standard_output, NULL, &exit_status, &error);
  if (error != NULL)
    g_warning ("Fail to spawn process. Error %s", error->message);
  else
    g_info ("Factory reset exitcode=%d output='%s'", exit_status, standard_output);
}

static void
do_process_factory_reset_event_slave (RmgExecutor *executor,
                                      RmgDEvent *dispatcher_event)
{
  RmgMessage msg;
  RmgMessageRequestFactoryReset data;
  const gchar *hostname = g_get_host_name ();

  g_assert (executor);
  g_assert (dispatcher_event);
  g_assert (executor->manager);

  memset (&data, 0, sizeof(RmgMessageRequestFactoryReset));

  if (!rmg_manager_connected (executor->manager))
    {
      if (rmg_manager_connect (executor->manager) != RMG_STATUS_OK)
        {
          g_warning ("Fail to connect to master instance");
          return;
        }
    }

  rmg_message_init (&msg, RMG_REQUEST_FACTORY_RESET, 0);

  memcpy (&data.service_name,
          dispatcher_event->service_name,
          strlen (dispatcher_event->service_name) + 1);
  memcpy (&data.context_name,
          hostname,
          strlen (hostname) + 1);

  rmg_message_set_data (&msg, &data, sizeof(data));

  if (rmg_manager_send (executor->manager, &msg) != RMG_STATUS_OK)
    g_warning ("Fail to send factory reset event to master");
}

static void
do_process_factory_reset_event (RmgExecutor *executor,
                                RmgDEvent *dispatcher_event)
{
  if (g_run_mode == RUN_MODE_MASTER)
    do_process_factory_reset_event_master (executor, dispatcher_event);
  else
    do_process_factory_reset_event_slave (executor, dispatcher_event);

  enter_meditation (executor, dispatcher_event);
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
