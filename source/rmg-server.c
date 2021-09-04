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
 * \file rmg-server.c
 */

#include "rmg-server.h"
#include "rmg-client.h"
#include "rmg-dispatcher.h"

#include <errno.h>
#include <memory.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

/**
 * @brief GSource prepare function
 */
static gboolean server_source_prepare(GSource *source, gint *timeout);

/**
 * @brief GSource check function
 */
static gboolean server_source_check(GSource *source);

/**
 * @brief GSource dispatch function
 */
static gboolean server_source_dispatch(GSource *source, GSourceFunc callback, gpointer rmgserver);
/**
 * @brief GSource callback function
 */
static gboolean server_source_callback(gpointer rmgserver);

/**
 * @brief GSource destroy notification callback function
 */
static void server_source_destroy_notify(gpointer rmgserver);

/**
 * @brief Client compare by name helper
 */
static gint foreach_client_compare(gconstpointer _client, gconstpointer _context_name);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs server_source_funcs = {
    server_source_prepare,
    NULL,
    server_source_dispatch,
    NULL,
    NULL,
    NULL,
};

static gboolean server_source_prepare(GSource *source, gint *timeout)
{
    RMG_UNUSED(source);
    *timeout = -1;
    return FALSE;
}

static gboolean server_source_check(GSource *source)
{
    RMG_UNUSED(source);
    return TRUE;
}

static gboolean server_source_dispatch(GSource *source, GSourceFunc callback, gpointer rmgserver)
{
    RMG_UNUSED(source);

    if (callback == NULL)
        return G_SOURCE_CONTINUE;

    return callback(rmgserver) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean server_source_callback(gpointer rmgserver)
{
    RmgServer *server = (RmgServer *) rmgserver;
    gint clientfd;

    g_assert(server);

    clientfd = accept(server->sockfd, (struct sockaddr *) NULL, NULL);

    if (clientfd >= 0) {
        RmgClient *client = rmg_client_new(clientfd, server->dispatcher);

        rmg_client_set_server_ref(client, server);
        rmg_server_add_client(server, client);
        g_info("New replica client instance connected %d", clientfd);
    } else {
        g_warning("Primary server accept failed");
        return FALSE;
    }

    return TRUE;
}

static void server_source_destroy_notify(gpointer rmgserver)
{
    RMG_UNUSED(rmgserver);
    g_info("Server terminated");
}

RmgServer *rmg_server_new(RmgOptions *options, gpointer dispatcher, GError **error)
{
    RmgServer *server = NULL;
    struct timeval tout;
    glong timeout;

    g_assert(options);

    server = (RmgServer *) g_source_new(&server_source_funcs, sizeof(RmgServer));
    g_assert(server);

    g_ref_count_init(&server->rc);

    server->options = rmg_options_ref(options);
    server->dispatcher = rmg_dispatcher_ref((RmgDispatcher *) dispatcher);

    server->sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server->sockfd < 0) {
        g_warning("Cannot create primary server socket");
        g_set_error(error,
                    g_quark_from_static_string("ServerNew"),
                    1,
                    "Fail to create primary server socket");
    } else {
        timeout = (glong) rmg_options_long_for(options, KEY_IPC_TIMEOUT_SEC);

        tout.tv_sec = timeout;
        tout.tv_usec = 0;

        if (setsockopt(server->sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tout, sizeof(tout)) == -1)
            g_warning("Failed to set the primary server socket rcv timeout: %s", strerror(errno));

        if (setsockopt(server->sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *) &tout, sizeof(tout)) == -1)
            g_warning("Failed to set the primary server socket snd timeout: %s", strerror(errno));
    }

    g_source_set_callback(RMG_EVENT_SOURCE(server),
                          G_SOURCE_FUNC(server_source_callback),
                          server,
                          server_source_destroy_notify);
    g_source_attach(RMG_EVENT_SOURCE(server), NULL);

    return server;
}

RmgServer *rmg_server_ref(RmgServer *server)
{
    g_assert(server);
    g_ref_count_inc(&server->rc);
    return server;
}

static void clients_list_destroy_notify(gpointer _client)
{
    RmgClient *client = (RmgClient *) _client;

    g_assert(client);
    rmg_client_unref(client);
}

void rmg_server_unref(RmgServer *server)
{
    g_assert(server);

    if (g_ref_count_dec(&server->rc) == TRUE) {
        if (server->options != NULL)
            rmg_options_unref(server->options);

        if (server->dispatcher != NULL)
            rmg_dispatcher_unref((RmgDispatcher *) server->dispatcher);

        g_list_free_full(g_steal_pointer(&server->clients), clients_list_destroy_notify);

        g_source_unref(RMG_EVENT_SOURCE(server));
    }
}

void rmg_server_add_client(RmgServer *server, gpointer client)
{
    g_assert(server);
    g_assert(client);

    server->clients = g_list_append(server->clients, rmg_client_ref(client));
}

void rmg_server_rem_client(RmgServer *server, gpointer client)
{
    g_assert(server);
    g_assert(client);

    server->clients = g_list_remove(server->clients, client);
    rmg_client_unref(client);
}

static gint foreach_client_compare(gconstpointer _client, gconstpointer _context_name)
{
    const gchar *context_name = (const gchar *) _context_name;
    const RmgClient *client = (const RmgClient *) _client;

    g_assert(client);
    g_assert(context_name);

    if (g_strcmp0(client->context_name, context_name) == 0)
        return 0;

    return -1;
}

gpointer rmg_server_get_client(RmgServer *server, const gchar *context_name)
{
    GList *el = NULL;

    g_assert(server);
    g_assert(context_name);

    el = g_list_find_custom(server->clients, context_name, foreach_client_compare);
    if (el != NULL)
        return el->data;

    return NULL;
}

RmgStatus rmg_server_bind_and_listen(RmgServer *server)
{
    g_autofree gchar *sock_addr = NULL;
    RmgStatus status = RMG_STATUS_OK;
    struct sockaddr_un saddr;

    g_assert(server);

    sock_addr = rmg_options_string_for(server->options, KEY_IPC_SOCK_ADDR);

    unlink(sock_addr);

    memset(&saddr, 0, sizeof(struct sockaddr_un));
    saddr.sun_family = AF_UNIX;
    strncpy(saddr.sun_path, sock_addr, sizeof(saddr.sun_path) - 1);

    g_debug("Server socket path %s", saddr.sun_path);

    if (bind(server->sockfd, (struct sockaddr *) &saddr, sizeof(struct sockaddr_un)) != -1) {
        if (listen(server->sockfd, 10) == -1) {
            g_warning("Primary server listen failed for path %s", saddr.sun_path);
            status = RMG_STATUS_ERROR;
        }
    } else {
        g_warning("Primary server bind failed for path %s", saddr.sun_path);
        status = RMG_STATUS_ERROR;
    }

    if (status == RMG_STATUS_ERROR) {
        if (server->sockfd > 0)
            close(server->sockfd);

        server->sockfd = -1;
    } else {
        server->tag
            = g_source_add_unix_fd(RMG_EVENT_SOURCE(server), server->sockfd, G_IO_IN | G_IO_PRI);
    }

    return status;
}
