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
 * \file rmg-dispatcher.c
 */

#include "rmg-dispatcher.h"
#include "rmg-client.h"
#include "rmg-relaxtimer.h"
#include "rmg-utils.h"

/**
 * @brief Post new event
 */
static void post_dispatcher_event(RmgDispatcher *dispatcher, RmgDEvent *event);

/**
 * @brief GSource prepare function
 */
static gboolean dispatcher_source_prepare(GSource *source, gint *timeout);

/**
 * @brief GSource dispatch function
 */
static gboolean
dispatcher_source_dispatch(GSource *source, GSourceFunc callback, gpointer _dispatcher);

/**
 * @brief GSource callback function
 */
static gboolean dispatcher_source_callback(gpointer _dispatcher, gpointer _event);

/**
 * @brief GSource destroy notification callback function
 */
static void dispatcher_source_destroy_notify(gpointer _dispatcher);

/**
 * @brief Async queue destroy notification callback function
 */
static void dispatcher_queue_destroy_notify(gpointer _dispatcher);

/**
 * @brief Handle service crash event
 */
static void do_process_service_crash_event(RmgDispatcher *dispatcher, RmgDEvent *event);

/**
 * @brief Handle friend process crash event
 */
static void do_friend_process_crash_event(RmgDispatcher *dispatcher, RmgDEvent *event);

/**
 * @brief Handle friend service failed event
 */
static void do_friend_service_failed_event(RmgDispatcher *dispatcher, RmgDEvent *event);

/**
 * @brief Start relax timer for event
 */
static void do_relaxtimer_start(RmgJournal *journal, RmgDEvent *event);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs dispatcher_source_funcs = {
    dispatcher_source_prepare,
    NULL,
    dispatcher_source_dispatch,
    NULL,
    NULL,
    NULL,
};

static void post_dispatcher_event(RmgDispatcher *dispatcher, RmgDEvent *event)
{
    g_assert(dispatcher);
    g_assert(event);

    g_async_queue_push(dispatcher->queue, event);
}

static gboolean dispatcher_source_prepare(GSource *source, gint *timeout)
{
    RmgDispatcher *dispatcher = (RmgDispatcher *) source;

    RMG_UNUSED(timeout);

    return (g_async_queue_length(dispatcher->queue) > 0);
}

static gboolean
dispatcher_source_dispatch(GSource *source, GSourceFunc callback, gpointer _dispatcher)
{
    RmgDispatcher *dispatcher = (RmgDispatcher *) source;
    gpointer event = g_async_queue_try_pop(dispatcher->queue);

    RMG_UNUSED(callback);
    RMG_UNUSED(_dispatcher);

    if (event == NULL)
        return G_SOURCE_CONTINUE;

    return dispatcher->callback(dispatcher, event) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean dispatcher_source_callback(gpointer _dispatcher, gpointer _event)
{
    RmgDispatcher *dispatcher = (RmgDispatcher *) _dispatcher;
    RmgDEvent *event = (RmgDEvent *) _event;

    g_assert(dispatcher);
    g_assert(event);

    switch (event->type) {
    case DEVENT_INFORM_PROCESS_CRASH:
        g_debug("Dispatch process crash event for '%s' in context '%s'",
                event->process_name,
                event->context_name);
        do_friend_process_crash_event(dispatcher, event);
        break;

    case DEVENT_INFORM_SERVICE_FAILED:
        g_debug("Dispatch service failed for '%s' in context '%s'",
                event->service_name,
                event->context_name);
        do_friend_service_failed_event(dispatcher, event);
        break;

    case DEVENT_SERVICE_CRASHED:
        g_info("Service '%s' crash event detected", event->service_name);
        do_process_service_crash_event(dispatcher, event);
        break;

    case DEVENT_SERVICE_RESTARTED:
        g_info("Service '%s' restarted", event->service_name);
        break;

    case DEVENT_REMOTE_CONTEXT_RESTART:
        g_info("Service '%s' from container '%s' request container restart",
               event->service_name,
               event->context_name);

        rmg_executor_push_event(dispatcher->executor, EXECUTOR_EVENT_CONTEXT_RESTART, event);
        break;

    case DEVENT_REMOTE_PLATFORM_RESTART:
        g_info("Service '%s' from container '%s' request platform restart",
               event->service_name,
               event->context_name);

        rmg_executor_push_event(dispatcher->executor, EXECUTOR_EVENT_PLATFORM_RESTART, event);
        break;

    case DEVENT_REMOTE_FACTORY_RESET:
        g_info("Service '%s' from container '%s' request factory reset",
               event->service_name,
               event->context_name);

        rmg_executor_push_event(dispatcher->executor, EXECUTOR_EVENT_FACTORY_RESET, event);
        break;

    default:
        break;
    }

    rmg_devent_unref(event);

    return TRUE;
}

static void dispatcher_source_destroy_notify(gpointer _dispatcher)
{
    RmgDispatcher *dispatcher = (RmgDispatcher *) _dispatcher;

    g_assert(dispatcher);
    g_debug("Dispatcher destroy notification");

    rmg_dispatcher_unref(dispatcher);
}

static void dispatcher_queue_destroy_notify(gpointer _dispatcher)
{
    RMG_UNUSED(_dispatcher);
    g_debug("Dispatcher queue destroy notification");
}

static RmgStatus run_mode_specific_init(RmgDispatcher *dispatcher, GError **error)
{
    RmgStatus status = RMG_STATUS_OK;

    g_assert(dispatcher);

    if (g_run_mode == RUN_MODE_PRIMARY) {
        dispatcher->server = rmg_server_new(dispatcher->options, dispatcher, error);

        if (*error != NULL)
            status = RMG_STATUS_ERROR;
        else {
            status = rmg_server_bind_and_listen(dispatcher->server);

            if (status != RMG_STATUS_OK) {
                g_set_error(error,
                            g_quark_from_static_string("DispatcherNew"),
                            1,
                            "Cannot bind and listen in server mode");
            } else
                rmg_executor_set_primary_server(dispatcher->executor, dispatcher->server);
        }
    } else {
        dispatcher->manager = rmg_manager_new(dispatcher->options, dispatcher);

        status = rmg_manager_connect(dispatcher->manager);
        if (status != RMG_STATUS_OK) {
            g_set_error(
                error, g_quark_from_static_string("DispatcherNew"), 1, "Cannot connect to primary");
        } else
            rmg_executor_set_replica_manager(dispatcher->executor, dispatcher->manager);
    }

    return status;
}

static void do_relaxtimer_start(RmgJournal *journal, RmgDEvent *event)
{
    g_autoptr(GError) error = NULL;

    g_assert(journal);
    g_assert(event);

    rmg_relaxtimer_trigger(journal, event->service_name, &error);
    if (error != NULL) {
        g_warning("Fail to trigger relax timer for service %s. Error %s",
                  event->service_name,
                  error->message);
    }
}

static void do_process_service_crash_event(RmgDispatcher *dispatcher, RmgDEvent *event)
{
    RmgActionType action_type = ACTION_INVALID;
    gboolean action_reset_after = false;

    g_autoptr(GError) error = NULL;
    gulong service_hash = 0;
    glong rvector = 0;

    g_assert(dispatcher);
    g_assert(event);

    /* check if a recovery unit is available */
    service_hash = rmg_journal_get_hash(dispatcher->journal, event->service_name, &error);
    if (error != NULL) {
        g_warning("Fail to get service hash %s. Error %s", event->service_name, error->message);
        return;
    } else {
        if (service_hash == 0) {
            g_info("No recovery unit defined for crashed service='%s'", event->service_name);
            return;
        }
    }

    /* we increment the rvector for this service */
    rvector = rmg_journal_get_rvector(dispatcher->journal, event->service_name, &error);
    if (error != NULL) {
        g_warning("Fail to read the rvector for service %s. Error %s",
                  event->service_name,
                  error->message);
        return;
    }

    rmg_journal_set_rvector(dispatcher->journal, event->service_name, (rvector + 1), &error);
    if (error != NULL) {
        g_warning("Fail to increment the rvector for service %s. Error %s",
                  event->service_name,
                  error->message);
        return;
    }

    rvector += 1;

    /* read next applicable action and execute */
    action_type = rmg_journal_get_service_action(dispatcher->journal, event->service_name, &error);
    if (error != NULL) {
        g_warning(
            "Fail to read next service action %s. Error %s", event->service_name, error->message);
        return;
    }

    /* check if the action has reset_after flag set */
    action_reset_after = rmg_journal_get_service_action_reset_after(
        dispatcher->journal, event->service_name, &error);
    if (error != NULL) {
        g_warning("Fail to read action reset after for service %s. Error %s",
                  event->service_name,
                  error->message);
    }

    if (action_type != ACTION_INVALID) {
        g_info("Action '%s' requiered for service='%s' rvector=%ld",
               rmg_utils_action_name(action_type),
               event->service_name,
               rvector);
    }

    switch (action_type) {
    case ACTION_SERVICE_IGNORE:
        g_info("Service '%s' action is to ignore", event->service_name);
        rmg_journal_set_rvector(dispatcher->journal, event->service_name, 0, &error);
        if (error != NULL) {
            g_warning("Fail to reset rvector after service disable '%s'. Error: %s",
                      event->service_name,
                      error->message);
        }
        break;

    case ACTION_SERVICE_RESET:
        rmg_executor_push_event(dispatcher->executor, EXECUTOR_EVENT_SERVICE_RESTART, event);
        if (action_reset_after) {
            rmg_journal_set_rvector(dispatcher->journal, event->service_name, 0, &error);
            if (error != NULL) {
                g_warning("Fail to reset rvector for '%s'. Error: %s",
                          event->service_name,
                          error->message);
            }
        } else
            do_relaxtimer_start(dispatcher->journal, event);
        break;

    case ACTION_PUBLIC_DATA_RESET:
        rmg_executor_push_event(
            dispatcher->executor, EXECUTOR_EVENT_SERVICE_RESET_PUBLIC_DATA, event);
        if (action_reset_after) {
            rmg_journal_set_rvector(dispatcher->journal, event->service_name, 0, &error);
            if (error != NULL) {
                g_warning("Fail to reset rvector for '%s'. Error: %s",
                          event->service_name,
                          error->message);
            }
        } else
            do_relaxtimer_start(dispatcher->journal, event);
        break;

    case ACTION_PRIVATE_DATA_RESET:
        rmg_executor_push_event(
            dispatcher->executor, EXECUTOR_EVENT_SERVICE_RESET_PRIVATE_DATA, event);
        if (action_reset_after) {
            rmg_journal_set_rvector(dispatcher->journal, event->service_name, 0, &error);
            if (error != NULL) {
                g_warning("Fail to reset rvector for '%s'. Error: %s",
                          event->service_name,
                          error->message);
            }
        } else
            do_relaxtimer_start(dispatcher->journal, event);
        break;

    case ACTION_SERVICE_DISABLE:
        rmg_executor_push_event(dispatcher->executor, EXECUTOR_EVENT_SERVICE_DISABLE, event);
        if (action_reset_after) {
            rmg_journal_set_rvector(dispatcher->journal, event->service_name, 0, &error);
            if (error != NULL) {
                g_warning("Fail to reset rvector for '%s'. Error: %s",
                          event->service_name,
                          error->message);
            }
        } else
            do_relaxtimer_start(dispatcher->journal, event);
        break;

    case ACTION_CONTEXT_RESET:
        if (action_reset_after) {
            rmg_journal_set_rvector(dispatcher->journal, event->service_name, 0, &error);
            if (error != NULL) {
                g_warning("Fail to reset rvector for '%s'. Error: %s",
                          event->service_name,
                          error->message);
            }
        }

        rmg_executor_push_event(dispatcher->executor, EXECUTOR_EVENT_CONTEXT_RESTART, event);
        break;

    case ACTION_PLATFORM_RESTART:
        if (action_reset_after) {
            rmg_journal_set_rvector(dispatcher->journal, event->service_name, 0, &error);
            if (error != NULL) {
                g_warning("Fail to reset rvector for '%s'. Error: %s",
                          event->service_name,
                          error->message);
            }
        }
        rmg_executor_push_event(dispatcher->executor, EXECUTOR_EVENT_PLATFORM_RESTART, event);
        break;

    case ACTION_FACTORY_RESET:
        rmg_executor_push_event(dispatcher->executor, EXECUTOR_EVENT_FACTORY_RESET, event);
        /* no need to handle recovery vector state */
        break;

    case ACTION_INVALID:
    default:
        g_warning("Invalid action set for service '%s'. Please use 'ignoreService' if needed",
                  event->service_name);
        break;
    }

    /* We set our current context and inform about this service crash */
    event->context_name = g_strdup(g_get_host_name());
    do_friend_service_failed_event(dispatcher, event);
}

static void foreach_client_notify_process_crash(gpointer _client, gpointer _event)
{
    RmgClient *client = (RmgClient *) _client;
    RmgDEvent *event = (RmgDEvent *) _event;

    g_autoptr(RmgMessage) msg = NULL;

    g_assert(client);
    g_assert(event);

    msg = rmg_message_new(RMG_MESSAGE_INFORM_PROCESS_CRASH, 0);

    rmg_message_set_process_name(msg, event->process_name);
    rmg_message_set_context_name(msg, event->context_name);

    if (rmg_client_send(client, msg) != RMG_STATUS_OK) {
        g_warning("Fail to send crash information to replica instance %s", client->context_name);
    } else {
        g_debug("Replica instance '%s' informed about process '%s'",
                client->context_name,
                event->process_name);
    }
}

static void do_friend_process_crash_event(RmgDispatcher *dispatcher, RmgDEvent *event)
{
    g_assert(dispatcher);
    g_assert(event);

    /* Only primary instance is aware of process crashes and dispatch the information to replicas */
    if (g_run_mode == RUN_MODE_PRIMARY) {
        g_list_foreach(dispatcher->server->clients, foreach_client_notify_process_crash, event);
    }

    rmg_executor_push_event(dispatcher->executor, EXECUTOR_EVENT_FRIEND_PROCESS_CRASH, event);
}

static void foreach_client_notify_service_failed(gpointer _client, gpointer _event)
{
    RmgClient *client = (RmgClient *) _client;
    RmgDEvent *event = (RmgDEvent *) _event;

    g_assert(client);
    g_assert(event);

    /* We inform the replica instances only if is not the event originator */
    if (g_strcmp0(client->context_name, event->context_name) != 0) {
        g_autoptr(RmgMessage) msg = rmg_message_new(RMG_MESSAGE_INFORM_CLIENT_SERVICE_FAILED, 0);

        rmg_message_set_service_name(msg, event->service_name);
        rmg_message_set_context_name(msg, event->context_name);

        if (rmg_client_send(client, msg) != RMG_STATUS_OK) {
            g_warning("Fail to send service fail information to replica instance %s",
                      client->context_name);
        } else {
            g_debug("Replica instance '%s' informed about service failure for '%s'",
                    client->context_name,
                    event->service_name);
        }
    }
}

static void do_friend_service_failed_event(RmgDispatcher *dispatcher, RmgDEvent *event)
{
    g_assert(dispatcher);
    g_assert(event);

    if (g_run_mode == RUN_MODE_PRIMARY) {
        g_list_foreach(dispatcher->server->clients, foreach_client_notify_service_failed, event);
    } else {
        g_autoptr(RmgMessage) msg = rmg_message_new(RMG_MESSAGE_INFORM_PRIMARY_SERVICE_FAILED, 0);

        rmg_message_set_service_name(msg, event->service_name);
        rmg_message_set_context_name(msg, event->context_name);

        if (rmg_manager_send(dispatcher->manager, msg) != RMG_STATUS_OK)
            g_warning("Fail to send service failed information to primary");
        else
            g_debug("Primary instance informed about service failure for '%s'",
                    event->service_name);
    }

    /* If is not our service we process as a new friend service failure in executor */
    if (g_strcmp0(g_get_host_name(), event->context_name) != 0)
        rmg_executor_push_event(dispatcher->executor, EXECUTOR_EVENT_FRIEND_SERVICE_FAILED, event);
}

RmgDispatcher *
rmg_dispatcher_new(RmgOptions *options, RmgJournal *journal, RmgExecutor *executor, GError **error)
{
    RmgDispatcher *dispatcher
        = (RmgDispatcher *) g_source_new(&dispatcher_source_funcs, sizeof(RmgDispatcher));

    g_assert(dispatcher);

    g_ref_count_init(&dispatcher->rc);
    dispatcher->callback = dispatcher_source_callback;
    dispatcher->options = rmg_options_ref(options);
    dispatcher->journal = rmg_journal_ref(journal);
    dispatcher->executor = rmg_executor_ref(executor);
    dispatcher->queue = g_async_queue_new_full(dispatcher_queue_destroy_notify);

    if (run_mode_specific_init(dispatcher, error) == RMG_STATUS_OK) {
        g_source_set_callback(
            RMG_EVENT_SOURCE(dispatcher), NULL, dispatcher, dispatcher_source_destroy_notify);
        g_source_attach(RMG_EVENT_SOURCE(dispatcher), NULL);
    }

    return dispatcher;
}

RmgDispatcher *rmg_dispatcher_ref(RmgDispatcher *dispatcher)
{
    g_assert(dispatcher);
    g_ref_count_inc(&dispatcher->rc);
    return dispatcher;
}

void rmg_dispatcher_unref(RmgDispatcher *dispatcher)
{
    g_assert(dispatcher);

    if (g_ref_count_dec(&dispatcher->rc) == TRUE) {
        g_autofree gchar *sock_addr = NULL;

        if (g_run_mode == RUN_MODE_PRIMARY) {
            sock_addr = rmg_options_string_for(dispatcher->options, KEY_IPC_SOCK_ADDR);
            unlink(sock_addr);
        }

        if (dispatcher->options != NULL)
            rmg_options_unref(dispatcher->options);

        if (dispatcher->journal != NULL)
            rmg_journal_unref(dispatcher->journal);

        if (dispatcher->executor != NULL)
            rmg_executor_unref(dispatcher->executor);

        if (dispatcher->server != NULL)
            rmg_server_unref(dispatcher->server);

        if (dispatcher->manager != NULL)
            rmg_manager_unref(dispatcher->manager);

        g_async_queue_unref(dispatcher->queue);
        g_source_unref(RMG_EVENT_SOURCE(dispatcher));
    }
}

void rmg_dispatcher_push_service_event(RmgDispatcher *dispatcher, RmgDEvent *event)
{
    post_dispatcher_event(dispatcher, event);
}
