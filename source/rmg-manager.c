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
 * \file rmg-manager.c
 */

#include "rmg-manager.h"
#include "rmg-defaults.h"
#include "rmg-dispatcher.h"

#include <errno.h>
#include <stdio.h>

/**
 * @brief GSource prepare function
 */
static gboolean manager_source_prepare(GSource *source, gint *timeout);

/**
 * @brief GSource check function
 */
static gboolean manager_source_check(GSource *source);

/**
 * @brief GSource dispatch function
 */
static gboolean manager_source_dispatch(GSource *source, GSourceFunc callback, gpointer user_data);

/**
 * @brief GSource callback function
 */
static gboolean manager_source_callback(gpointer data);

/**
 * @brief GSource destroy notification callback function
 */
static void manager_source_destroy_notify(gpointer data);

/**
 * @brief Process message from crashhandler instance
 */
static void process_message(RmgManager *manager, RmgMessage *msg);

/**
 * @brief Send initial instance descriptor
 */
static void send_descriptor(RmgManager *manager);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs manager_source_funcs = {
    manager_source_prepare,
    NULL,
    manager_source_dispatch,
    NULL,
    NULL,
    NULL,
};

static gboolean manager_source_prepare(GSource *source, gint *timeout)
{
    RMG_UNUSED(source);
    *timeout = -1;
    return FALSE;
}

static gboolean manager_source_check(GSource *source)
{
    RMG_UNUSED(source);
    return TRUE;
}

static gboolean manager_source_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
    RMG_UNUSED(source);

    if (callback == NULL)
        return G_SOURCE_CONTINUE;

    return callback(user_data) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean manager_source_callback(gpointer data)
{
    RmgManager *manager = (RmgManager *) data;

    g_autoptr(RmgMessage) msg = NULL;
    gboolean status = TRUE;

    g_assert(manager);

    msg = rmg_message_new(RMG_MESSAGE_UNKNOWN, 0);

    if (rmg_message_read(manager->sockfd, msg) != RMG_STATUS_OK) {
        g_debug("Cannot read from manager socket %d", manager->sockfd);
        status = FALSE;
    } else
        process_message(manager, msg);

    return status;
}

static void process_message(RmgManager *manager, RmgMessage *msg)
{
    RmgDispatcher *dispatcher = (RmgDispatcher *) manager->dispatcher;

    g_assert(msg);
    g_assert(manager);
    g_assert(dispatcher);

    if (!rmg_message_is_valid(msg))
        g_warning("Message malformat or with different protocol version");

    switch (rmg_message_get_type(msg)) {
    case RMG_MESSAGE_INFORM_PROCESS_CRASH: {
        RmgDEvent *event = rmg_devent_new(DEVENT_INFORM_PROCESS_CRASH);

        rmg_devent_set_process_name(event, rmg_message_get_process_name(msg));
        rmg_devent_set_context_name(event, rmg_message_get_context_name(msg));

        g_info("Primary instance informed about process '%s' crash in context '%s'",
               event->process_name,
               event->context_name);
        rmg_dispatcher_push_service_event(dispatcher, event);
    } break;

    case RMG_MESSAGE_INFORM_CLIENT_SERVICE_FAILED: {
        RmgDEvent *event = rmg_devent_new(DEVENT_INFORM_SERVICE_FAILED);

        rmg_devent_set_service_name(event, rmg_message_get_service_name(msg));
        rmg_devent_set_context_name(event, rmg_message_get_context_name(msg));

        g_info("Primary instance informed about service '%s' fail in context '%s'",
               event->service_name,
               event->context_name);
        rmg_dispatcher_push_service_event(dispatcher, event);
    } break;

    default:
        break;
    }
}

static void manager_source_destroy_notify(gpointer data)
{
    RmgManager *manager = (RmgManager *) data;

    g_assert(manager);
    g_debug("Manager %d disconnected", manager->sockfd);

    rmg_manager_unref(manager);
}

static void send_descriptor(RmgManager *manager)
{
    g_autoptr(RmgMessage) msg = NULL;

    g_assert(manager);

    msg = rmg_message_new(RMG_MESSAGE_REPLICA_DESCRIPTOR, 0);
    rmg_message_set_context_name(msg, g_get_host_name());

    if (rmg_manager_send(manager, msg) != RMG_STATUS_OK)
        g_warning("Fail to send replica instance descriptor to primary instance");
}

RmgManager *rmg_manager_new(RmgOptions *opts, gpointer dispatcher)
{
    RmgManager *manager = (RmgManager *) g_source_new(&manager_source_funcs, sizeof(RmgManager));

    g_assert(manager);
    g_assert(opts);

    g_ref_count_init(&manager->rc);
    manager->dispatcher = rmg_dispatcher_ref((RmgDispatcher *) dispatcher);
    manager->sockfd = -1;
    manager->connected = false;
    manager->opts = rmg_options_ref(opts);

    g_source_set_callback(RMG_EVENT_SOURCE(manager),
                          G_SOURCE_FUNC(manager_source_callback),
                          manager,
                          manager_source_destroy_notify);
    g_source_attach(RMG_EVENT_SOURCE(manager), NULL);

    return manager;
}

RmgManager *rmg_manager_ref(RmgManager *manager)
{
    g_assert(manager);
    g_ref_count_inc(&manager->rc);
    return manager;
}

void rmg_manager_unref(RmgManager *manager)
{
    g_assert(manager);

    if (g_ref_count_dec(&manager->rc) == TRUE) {
        if (rmg_manager_connected(manager) == TRUE)
            (void) rmg_manager_disconnect(manager);

        if (manager->opts != NULL)
            rmg_options_unref(manager->opts);

        if (manager->dispatcher != NULL)
            rmg_dispatcher_unref(manager->dispatcher);

        g_source_unref(RMG_EVENT_SOURCE(manager));
    }
}

RmgStatus rmg_manager_connect(RmgManager *manager)
{
    g_autofree gchar *opt_sock_addr = NULL;
    struct timeval tout;
    glong opt_timeout;

    g_assert(manager);

    if (manager->connected)
        return RMG_STATUS_ERROR;

    manager->sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (manager->sockfd < 0) {
        g_warning("Cannot create connection socket");
        return RMG_STATUS_ERROR;
    }

    opt_sock_addr = rmg_options_string_for(manager->opts, KEY_IPC_SOCK_ADDR);
    opt_timeout = (glong) rmg_options_long_for(manager->opts, KEY_IPC_TIMEOUT_SEC);

    memset(&manager->saddr, 0, sizeof(struct sockaddr_un));
    manager->saddr.sun_family = AF_UNIX;

    snprintf(manager->saddr.sun_path, (sizeof(manager->saddr.sun_path) - 1), "%s", opt_sock_addr);

    if (connect(manager->sockfd, (struct sockaddr *) &manager->saddr, sizeof(struct sockaddr_un))
        < 0) {
        g_info("Primary instance not available: %s", manager->saddr.sun_path);
        close(manager->sockfd);
        return RMG_STATUS_ERROR;
    }

    tout.tv_sec = opt_timeout;
    tout.tv_usec = 0;

    manager->tag
        = g_source_add_unix_fd(RMG_EVENT_SOURCE(manager), manager->sockfd, G_IO_IN | G_IO_PRI);

    if (setsockopt(manager->sockfd, SOL_SOCKET, SO_RCVTIMEO, (gchar *) &tout, sizeof(tout)) == -1)
        g_warning("Failed to set the socket receiving timeout: %s", strerror(errno));

    if (setsockopt(manager->sockfd, SOL_SOCKET, SO_SNDTIMEO, (gchar *) &tout, sizeof(tout)) == -1)
        g_warning("Failed to set the socket sending timeout: %s", strerror(errno));

    manager->connected = true;
    send_descriptor(manager);

    return RMG_STATUS_OK;
}

RmgStatus rmg_manager_disconnect(RmgManager *manager)
{
    if (!manager->connected)
        return RMG_STATUS_ERROR;

    if (manager->sockfd > 0) {
        close(manager->sockfd);
        manager->sockfd = -1;
    }

    manager->connected = false;

    return RMG_STATUS_OK;
}

gboolean rmg_manager_connected(RmgManager *manager)
{
    g_assert(manager);
    return manager->connected;
}

RmgStatus rmg_manager_send(RmgManager *manager, RmgMessage *m)
{
    fd_set wfd;
    struct timeval tv;
    RmgStatus status = RMG_STATUS_OK;

    g_assert(manager);
    g_assert(m);

    if (manager->sockfd < 0 || !manager->connected) {
        g_warning("No connection to manager");
        return RMG_STATUS_ERROR;
    }

    FD_ZERO(&wfd);

    tv.tv_sec = MANAGER_SELECT_TIMEOUT;
    tv.tv_usec = 0;
    FD_SET(manager->sockfd, &wfd);

    status = select(manager->sockfd + 1, NULL, &wfd, NULL, &tv);
    if (status == -1)
        g_warning("Server socket select failed");
    else {
        if (status > 0)
            status = rmg_message_write(manager->sockfd, m);
    }

    return status;
}
