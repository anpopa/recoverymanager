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
 * \file rmg-client.c
 */

#include "rmg-client.h"
#include "rmg-server.h"
#include "rmg-dispatcher.h"
#include "rmg-defaults.h"
#include "rmg-utils.h"

#include <sys/types.h>
#include <unistd.h>

/**
 * @brief GSource prepare function
 */
static gboolean client_source_prepare (GSource *source, gint *timeout);

/**
 * @brief GSource check function
 */
static gboolean client_source_check (GSource *source);

/**
 * @brief GSource dispatch function
 */
static gboolean client_source_dispatch (GSource *source, GSourceFunc callback, gpointer user_data);

/**
 * @brief GSource callback function
 */
static gboolean client_source_callback (gpointer data);

/**
 * @brief GSource destroy notification callback function
 */
static void client_source_destroy_notify (gpointer data);

/**
 * @brief Process message from crashhandler instance
 */
static void process_message (RmgClient *c, RmgMessage *msg);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs client_source_funcs = {
  client_source_prepare,
  NULL,
  client_source_dispatch,
  NULL,
  NULL,
  NULL,
};

static gboolean
client_source_prepare (GSource *source, gint *timeout)
{
  RMG_UNUSED (source);
  *timeout = -1;
  return FALSE;
}

static gboolean
client_source_check (GSource *source)
{
  RMG_UNUSED (source);
  return TRUE;
}

static gboolean
client_source_dispatch (GSource *source, GSourceFunc callback, gpointer user_data)
{
  RMG_UNUSED (source);

  if (callback == NULL)
    return G_SOURCE_CONTINUE;

  return callback (user_data) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
client_source_callback (gpointer data)
{
  RmgClient *client = (RmgClient *)data;

  g_autoptr (RmgMessage) msg = NULL;
  gboolean status = TRUE;

  g_assert (client);

  msg = rmg_message_new (RMG_MESSAGE_UNKNOWN, 0);

  if (rmg_message_read (client->sockfd, msg) != RMG_STATUS_OK)
    {
      g_debug ("Cannot read from client socket %d", client->sockfd);
      status = FALSE;
    }
  else
    process_message (client, msg);

  return status;
}

static void
client_source_destroy_notify (gpointer data)
{
  RmgClient *client = (RmgClient *)data;

  g_assert (client);

  g_debug ("Client %d disconnected", client->sockfd);
  rmg_server_rem_client ((RmgServer *)client->server, client);
  rmg_client_unref (client);
}

static void
process_message (RmgClient *c, RmgMessage *msg)
{
  RmgDispatcher *dispatcher = (RmgDispatcher *)c->dispatcher;

  g_assert (c);
  g_assert (msg);

  if (!rmg_message_is_valid (msg))
    g_warning ("Message malformat or with different protocol version");

  switch (rmg_message_get_type (msg))
    {
    case RMG_MESSAGE_REPLICA_DESCRIPTOR: {
      c->context_name = g_strdup (rmg_message_get_context_name (msg));
      g_info ("Replica instance id=%d identify with name=%s", c->sockfd, c->context_name);
    } break;

    case RMG_MESSAGE_REQUEST_CONTEXT_RESTART: {
      RmgDEvent *event = rmg_devent_new (DEVENT_REMOTE_CONTEXT_RESTART);

      rmg_devent_set_service_name (event, rmg_message_get_service_name (msg));
      rmg_devent_set_context_name (event, rmg_message_get_context_name (msg));

      g_info ("Dispatch replica instance context restart request from %s", event->context_name);
      rmg_dispatcher_push_service_event (dispatcher, event);
    } break;

    case RMG_MESSAGE_REQUEST_PLATFORM_RESTART: {
      RmgDEvent *event = rmg_devent_new (DEVENT_REMOTE_CONTEXT_RESTART);

      rmg_devent_set_service_name (event, rmg_message_get_service_name (msg));
      rmg_devent_set_context_name (event, rmg_message_get_context_name (msg));

      g_info ("Dispatch replica instance platform restart request from %s", event->context_name);
      rmg_dispatcher_push_service_event (dispatcher, event);
    } break;

    case RMG_MESSAGE_REQUEST_FACTORY_RESET: {
      RmgDEvent *event = rmg_devent_new (DEVENT_REMOTE_FACTORY_RESET);

      rmg_devent_set_service_name (event, rmg_message_get_service_name (msg));
      rmg_devent_set_context_name (event, rmg_message_get_context_name (msg));

      g_info ("Dispatch replica instance factory reset request from %s", event->context_name);
      rmg_dispatcher_push_service_event (dispatcher, event);
    } break;

    case RMG_MESSAGE_INFORM_PRIMARY_SERVICE_FAILED: {
      RmgDEvent *event = rmg_devent_new (DEVENT_INFORM_SERVICE_FAILED);

      rmg_devent_set_service_name (event, rmg_message_get_service_name (msg));
      rmg_devent_set_context_name (event, rmg_message_get_context_name (msg));

      g_info ("Dispatch replica instance service failed '%s' for '%s'",
              event->service_name,
              event->context_name);
      rmg_dispatcher_push_service_event (dispatcher, event);
    } break;

    default:
      break;
    }
}

RmgClient *
rmg_client_new (gint clientfd, gpointer dispatcher)
{
  RmgClient *client = (RmgClient *)g_source_new (&client_source_funcs, sizeof(RmgClient));

  g_assert (client);

  g_ref_count_init (&client->rc);

  client->sockfd = clientfd;
  client->dispatcher = rmg_dispatcher_ref ((RmgDispatcher *)dispatcher);

  g_source_set_callback (RMG_EVENT_SOURCE (client),
                         G_SOURCE_FUNC (client_source_callback),
                         client,
                         client_source_destroy_notify);
  g_source_attach (RMG_EVENT_SOURCE (client), NULL);

  client->tag = g_source_add_unix_fd (RMG_EVENT_SOURCE (client),
                                      client->sockfd,
                                      G_IO_IN | G_IO_PRI);

  return client;
}

RmgClient *
rmg_client_ref (RmgClient *client)
{
  g_assert (client);
  g_ref_count_inc (&client->rc);
  return client;
}

void
rmg_client_set_server_ref (RmgClient *client, gpointer server)
{
  g_assert (client);
  g_assert (server);

  client->server = rmg_server_ref (server);
}

RmgStatus
rmg_client_send (RmgClient *client, RmgMessage *msg)
{
  fd_set wfd;
  struct timeval tv;
  RmgStatus status = RMG_STATUS_OK;

  g_assert (client);
  g_assert (msg);

  if (client->sockfd < 0)
    {
      g_warning ("No connection to client");
      return RMG_STATUS_ERROR;
    }

  FD_ZERO (&wfd);

  tv.tv_sec = CLIENT_SELECT_TIMEOUT;
  tv.tv_usec = 0;
  FD_SET (client->sockfd, &wfd);

  status = select (client->sockfd + 1, NULL, &wfd, NULL, &tv);
  if (status == -1)
    g_warning ("Client socket select failed");
  else
    {
      if (status > 0)
        status = rmg_message_write (client->sockfd, msg);
    }

  return status;
}

void
rmg_client_unref (RmgClient *client)
{
  g_assert (client);

  if (g_ref_count_dec (&client->rc) == TRUE)
    {
      if (client->dispatcher != NULL)
        rmg_dispatcher_unref ((RmgDispatcher *)client->dispatcher);

      if (client->server != NULL)
        rmg_server_unref ((RmgServer *)client->server);

      if (client->context_name != NULL)
        g_free (client->context_name);

      g_source_unref (RMG_EVENT_SOURCE (client));
    }
}
