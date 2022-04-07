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
 * \file rmg-executor.c
 */

#include "rmg-executor.h"
#include "rmg-friendtimer.h"
#include "rmg-utils.h"

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
static void post_executor_event (RmgExecutor *executor, ExecutorEventType type,
                                 RmgDEvent *dispatcher_event);

/**
 * @brief GSource prepare function
 */
static gboolean executor_source_prepare (GSource *source, gint *timeout);

/**
 * @brief GSource dispatch function
 */
static gboolean executor_source_dispatch (GSource *source, GSourceFunc callback,
                                          gpointer _executor);

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
 * @brief Process inform process crash event
 */
static void do_process_friend_crash_event (RmgExecutor *executor, RmgDEvent *dispatcher_event,
                                           RmgFriendType type);

/**
 * @brief Process service reset public data event
 */
static void do_process_service_reset_public_data_event (RmgExecutor *executor,
                                                        RmgDEvent *dispatcher_event);

/**
 * @brief Process service reset private data event
 */
static void do_process_service_reset_private_data_event (RmgExecutor *executor,
                                                         RmgDEvent *dispatcher_event);

/**
 * @brief Process service disable event
 */
static void do_process_service_disable_event (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Process service context restart event
 */
static void do_process_context_restart_event (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Process service context restart for primary instance
 */
static void do_process_context_restart_event_primary (RmgExecutor *executor,
                                                      RmgDEvent *dispatcher_event);

/**
 * @brief Process service context restart for replica instance
 */
static void do_process_context_restart_event_replica (RmgExecutor *executor,
                                                      RmgDEvent *dispatcher_event);

/**
 * @brief Process service platform restart event
 */
static void do_process_platform_restart_event (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Process service platform restart event for primary instance
 */
static void do_process_platform_restart_event_primary (RmgExecutor *executor,
                                                       RmgDEvent *dispatcher_event);

/**
 * @brief Process service platform restart event for replica instance
 */
static void do_process_platform_restart_event_replica (RmgExecutor *executor,
                                                       RmgDEvent *dispatcher_event);

/**
 * @brief Process service factory reset event
 */
static void do_process_factory_reset_event (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief Process service factory reset event for primary instance
 */
static void do_process_factory_reset_event_primary (RmgExecutor *executor,
                                                    RmgDEvent *dispatcher_event);

/**
 * @brief Process service factory reset event for replica instance
 */
static void do_process_factory_reset_event_replica (RmgExecutor *executor,
                                                    RmgDEvent *dispatcher_event);

/**
 * @brief Enter meditation state
 */
static void enter_meditation (RmgExecutor *executor, RmgDEvent *dispatcher_event);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs executor_source_funcs = {
  executor_source_prepare, NULL, executor_source_dispatch, NULL, NULL, NULL,
};

static void
post_executor_event (RmgExecutor *executor, ExecutorEventType type, RmgDEvent *dispatcher_event)
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
executor_source_prepare (GSource *source, gint *timeout)
{
  RmgExecutor *executor = (RmgExecutor *)source;

  RMG_UNUSED (timeout);

  return (g_async_queue_length (executor->queue) > 0);
}

static gboolean
executor_source_dispatch (GSource *source, GSourceFunc callback, gpointer _executor)
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
executor_source_callback (gpointer _executor, gpointer _event)
{
  RmgExecutor *executor = (RmgExecutor *)_executor;
  RmgExecutorEvent *event = (RmgExecutorEvent *)_event;

  g_assert (executor);
  g_assert (event);

  switch (event->type)
    {
    case EXECUTOR_EVENT_FRIEND_PROCESS_CRASH:
      do_process_friend_crash_event (executor, event->dispatcher_event, FRIEND_PROCESS);
      break;

    case EXECUTOR_EVENT_FRIEND_SERVICE_FAILED:
      do_process_friend_crash_event (executor, event->dispatcher_event, FRIEND_SERVICE);
      break;

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
foreach_service_friend (gpointer _friend, gpointer _executor)
{
  const RmgFriendResponseEntry *friend = (const RmgFriendResponseEntry *)_friend;

  g_assert (friend);

  rmg_friendtimer_trigger (friend->service_name, friend->action, friend->argument, _executor,
                           (guint) friend->delay);

  g_debug ("Friend timer started for service '%s' with action '%s'", friend->service_name,
           rmg_utils_friend_action_name (friend->action));
}

static void
foreach_service_friend_remove (gpointer data)
{
  RmgFriendResponseEntry *entry = (RmgFriendResponseEntry *)data;

  g_assert (entry);

  g_free (entry->service_name);
  g_free (entry);
}

static void
do_process_friend_crash_event (RmgExecutor *executor, RmgDEvent *dispatcher_event,
                               RmgFriendType friend_type)
{
  const gchar *target_name = NULL;

  g_autoptr (GError) error = NULL;
  GList *services = NULL;

  g_assert (executor);
  g_assert (dispatcher_event);

  target_name = (friend_type == FRIEND_PROCESS) ? dispatcher_event->process_name
                                                : dispatcher_event->service_name;

  services = rmg_journal_get_services_for_friend (
      executor->journal, target_name, dispatcher_event->context_name, friend_type, &error);
  if (error != NULL)
    {
      g_warning ("Fail to get services for friend %s. Error %s", dispatcher_event->process_name,
                 error->message);
      return;
    }

  if (services != NULL)
    {
      g_list_foreach (services, foreach_service_friend, executor);
      g_list_free_full (services, foreach_service_friend_remove);
    }
}

static void
do_process_service_reset_public_data_event (RmgExecutor *executor, RmgDEvent *dispatcher_event)
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

  reset_path = rmg_journal_get_public_data_path (executor->journal, dispatcher_event->service_name,
                                                 &error);
  if (error != NULL)
    {
      g_warning ("Fail to read public data path for service %s. Error %s",
                 dispatcher_event->service_name, error->message);
      return;
    }

  reset_cmd = rmg_options_string_for (executor->options, KEY_PUBLIC_DATA_RESET_CMD);

  path_tokens = g_strsplit (reset_cmd, "${path}", 3);
  reset_with_path_cmd = g_strjoinv (reset_path, path_tokens);
  g_strfreev (path_tokens);

  name_tokens = g_strsplit (reset_with_path_cmd, "${service_name}", 3);
  reset_with_name_cmd = g_strjoinv (dispatcher_event->service_name, name_tokens);
  g_strfreev (name_tokens);

  g_info ("Reset public data for service='%s' command='%s'", dispatcher_event->service_name,
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
do_process_service_reset_private_data_event (RmgExecutor *executor, RmgDEvent *dispatcher_event)
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

  reset_path = rmg_journal_get_private_data_path (executor->journal, dispatcher_event->service_name,
                                                  &error);
  if (error != NULL)
    {
      g_warning ("Fail to read private data path for service %s. Error %s",
                 dispatcher_event->service_name, error->message);
      return;
    }

  reset_cmd = rmg_options_string_for (executor->options, KEY_PRIVATE_DATA_RESET_CMD);

  path_tokens = g_strsplit (reset_cmd, "${path}", 3);
  reset_with_path_cmd = g_strjoinv (reset_path, path_tokens);
  g_strfreev (path_tokens);

  name_tokens = g_strsplit (reset_with_path_cmd, "${service_name}", 3);
  reset_with_name_cmd = g_strjoinv (dispatcher_event->service_name, name_tokens);
  g_strfreev (name_tokens);

  g_info ("Reset private data for service='%s' command='%s'", dispatcher_event->service_name,
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
do_process_service_disable_event (RmgExecutor *executor, RmgDEvent *dispatcher_event)
{
  RMG_UNUSED (executor);
  g_info ("Service '%s' remains disabled this lifecycle", dispatcher_event->service_name);
}

static void
do_process_context_restart_event (RmgExecutor *executor, RmgDEvent *dispatcher_event)
{
  if (g_run_mode == RUN_MODE_PRIMARY)
    {
      if (dispatcher_event->context_name == NULL)
        do_process_platform_restart_event_primary (executor, dispatcher_event);
      else
        do_process_context_restart_event_primary (executor, dispatcher_event);
    }
  else
    do_process_context_restart_event_replica (executor, dispatcher_event);
}

static void
do_process_context_restart_event_primary (RmgExecutor *executor, RmgDEvent *dispatcher_event)
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
do_process_context_restart_event_replica (RmgExecutor *executor, RmgDEvent *dispatcher_event)
{
  g_autoptr (RmgMessage) msg = NULL;

  g_assert (executor);
  g_assert (dispatcher_event);
  g_assert (executor->manager);

  msg = rmg_message_new (RMG_MESSAGE_REQUEST_CONTEXT_RESTART, 0);

  rmg_message_set_service_name (msg, dispatcher_event->service_name);
  rmg_message_set_context_name (msg, g_get_host_name ());

  if (rmg_manager_send (executor->manager, msg) != RMG_STATUS_OK)
    g_warning ("Fail to send context restart event to primary instance");
  else
    enter_meditation (executor, dispatcher_event);
}

static void
do_process_platform_restart_event_primary (RmgExecutor *executor, RmgDEvent *dispatcher_event)
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
          dispatcher_event->service_name, reset_cmd);

  command_line = g_strdup_printf ("sh -c \"%s\"", reset_cmd);

  g_spawn_command_line_sync (command_line, &standard_output, NULL, &exit_status, &error);
  if (error != NULL)
    g_warning ("Fail to spawn process. Error %s", error->message);
  else
    g_info ("Platform restart exitcode=%d output='%s'", exit_status, standard_output);
}

static void
do_process_platform_restart_event_replica (RmgExecutor *executor, RmgDEvent *dispatcher_event)
{
  g_autoptr (RmgMessage) msg = NULL;

  g_assert (executor);
  g_assert (dispatcher_event);
  g_assert (executor->manager);

  msg = rmg_message_new (RMG_MESSAGE_REQUEST_PLATFORM_RESTART, 0);

  rmg_message_set_service_name (msg, dispatcher_event->service_name);
  rmg_message_set_context_name (msg, g_get_host_name ());

  if (rmg_manager_send (executor->manager, msg) != RMG_STATUS_OK)
    g_warning ("Fail to send platform restart event to primary instance");
}

static void
do_process_platform_restart_event (RmgExecutor *executor, RmgDEvent *dispatcher_event)
{
  if (g_run_mode == RUN_MODE_PRIMARY)
    do_process_platform_restart_event_primary (executor, dispatcher_event);
  else
    do_process_platform_restart_event_replica (executor, dispatcher_event);

  enter_meditation (executor, dispatcher_event);
}

static void
do_process_factory_reset_event_primary (RmgExecutor *executor, RmgDEvent *dispatcher_event)
{
  g_autoptr (GError) error = NULL;
  g_autofree gchar *reset_cmd = NULL;
  g_autofree gchar *standard_output = NULL;
  g_autofree gchar *command_line = NULL;
  gint exit_status;

  g_assert (executor);
  g_assert (dispatcher_event);

  reset_cmd = rmg_options_string_for (executor->options, KEY_FACTORY_RESET_CMD);

  g_info ("Do factory reset on service='%s' request. Command='%s'", dispatcher_event->service_name,
          reset_cmd);

  command_line = g_strdup_printf ("sh -c \"%s\"", reset_cmd);

  g_spawn_command_line_sync (command_line, &standard_output, NULL, &exit_status, &error);
  if (error != NULL)
    g_warning ("Fail to spawn process. Error %s", error->message);
  else
    g_info ("Factory reset exitcode=%d output='%s'", exit_status, standard_output);
}

static void
do_process_factory_reset_event_replica (RmgExecutor *executor, RmgDEvent *dispatcher_event)
{
  g_autoptr (RmgMessage) msg = NULL;

  g_assert (executor);
  g_assert (dispatcher_event);
  g_assert (executor->manager);

  msg = rmg_message_new (RMG_MESSAGE_REQUEST_FACTORY_RESET, 0);

  rmg_message_set_service_name (msg, dispatcher_event->service_name);
  rmg_message_set_context_name (msg, g_get_host_name ());

  if (rmg_manager_send (executor->manager, msg) != RMG_STATUS_OK)
    g_warning ("Fail to send factory reset event to primary instance");
}

static void
do_process_factory_reset_event (RmgExecutor *executor, RmgDEvent *dispatcher_event)
{
  if (g_run_mode == RUN_MODE_PRIMARY)
    do_process_factory_reset_event_primary (executor, dispatcher_event);
  else
    do_process_factory_reset_event_replica (executor, dispatcher_event);

  enter_meditation (executor, dispatcher_event);
}

static void
do_process_service_restart_event (RmgExecutor *executor, RmgDEvent *dispatcher_event)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GVariant) response = NULL;

  g_assert (executor);
  g_assert (dispatcher_event);

  response
      = g_dbus_proxy_call_sync (dispatcher_event->manager_proxy, "RestartUnit",
                                g_variant_new ("(ss)", dispatcher_event->service_name, "replace"),
                                G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

  if (error != NULL)
    g_warning ("Fail to call RestartUnit on Manager proxy. Error %s", error->message);
  else
    g_info ("Request service restart for unit='%s'", dispatcher_event->service_name);
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
  RmgExecutor *executor
      = (RmgExecutor *)g_source_new (&executor_source_funcs, sizeof (RmgExecutor));

  g_assert (executor);

  g_ref_count_init (&executor->rc);
  executor->callback = executor_source_callback;
  executor->options = rmg_options_ref (options);
  executor->journal = rmg_journal_ref (journal);
  executor->queue = g_async_queue_new_full (executor_queue_destroy_notify);

  g_source_set_callback (RMG_EVENT_SOURCE (executor), NULL, executor,
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

      if (executor->server != NULL)
        rmg_server_unref (executor->server);

      if (executor->sd_manager_proxy != NULL)
        g_object_unref (executor->sd_manager_proxy);

      g_async_queue_unref (executor->queue);
      g_source_unref (RMG_EVENT_SOURCE (executor));
    }
}

void
rmg_executor_set_replica_manager (RmgExecutor *executor, RmgManager *manager)
{
  g_assert (executor);
  g_assert (manager);
  executor->manager = rmg_manager_ref (manager);
}

void
rmg_executor_set_primary_server (RmgExecutor *executor, RmgServer *server)
{
  g_assert (executor);
  g_assert (server);
  executor->server = rmg_server_ref (server);
}

void
rmg_executor_push_event (RmgExecutor *executor, ExecutorEventType type, RmgDEvent *dispatcher_event)
{
  post_executor_event (executor, type, dispatcher_event);
}

void
rmg_executor_set_proxy (RmgExecutor *executor, GDBusProxy *dbus_proxy)
{
  g_assert (executor);
  g_assert (dbus_proxy);
  g_assert (!executor->sd_manager_proxy);

  g_debug ("Proxy available for executor");
  executor->sd_manager_proxy = g_object_ref (dbus_proxy);
}
