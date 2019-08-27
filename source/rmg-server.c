/* rmg-server.c
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

#include "rmg-server.h"
#include "rmg-client.h"
#include "rmg-dispatcher.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <time.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * @brief GSource prepare function
 */
static gboolean server_source_prepare (GSource *source, gint *timeout);

/**
 * @brief GSource check function
 */
static gboolean server_source_check (GSource *source);

/**
 * @brief GSource dispatch function
 */
static gboolean server_source_dispatch (GSource *source, GSourceFunc callback, gpointer rmgserver);

/**
 * @brief GSource callback function
 */
static gboolean server_source_callback (gpointer rmgserver);

/**
 * @brief GSource destroy notification callback function
 */
static void server_source_destroy_notify (gpointer rmgserver);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs server_source_funcs =
{
  server_source_prepare,
  NULL,
  server_source_dispatch,
  NULL,
  NULL,
  NULL,
};

static gboolean
server_source_prepare (GSource *source,
                       gint *timeout)
{
  RMG_UNUSED (source);
  *timeout = -1;
  return FALSE;
}

static gboolean
server_source_check (GSource *source)
{
  RMG_UNUSED (source);
  return TRUE;
}

static gboolean
server_source_dispatch (GSource *source, GSourceFunc callback, gpointer rmgserver)
{
  RMG_UNUSED (source);

  if (callback == NULL)
    {
      return G_SOURCE_CONTINUE;
    }

  return callback (rmgserver) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
server_source_callback (gpointer rmgserver)
{
  RmgServer *server = (RmgServer *)rmgserver;
  gint clientfd;

  g_assert (server);

  clientfd = accept (server->sockfd, (struct sockaddr*)NULL, NULL);

  if (clientfd >= 0)
    {
      RmgClient *client = rmg_client_new (clientfd, server->dispatcher);

      RMG_UNUSED (client);

      g_info ("New recoverymanager client connected %d", clientfd);
    }
  else
    {
      g_warning ("Server accept failed");
      return FALSE;
    }

  return TRUE;
}

static void
server_source_destroy_notify (gpointer rmgserver)
{
  RMG_UNUSED (rmgserver);
  g_info ("Server terminated");
}

RmgServer *
rmg_server_new (RmgOptions *options,
                gpointer dispatcher,
                GError **error)
{
  RmgServer *server = NULL;
  struct timeval tout;
  glong timeout;

  g_assert (options);

  server = (RmgServer *)g_source_new (&server_source_funcs, sizeof(RmgServer));
  g_assert (server);

  g_ref_count_init (&server->rc);

  server->options = rmg_options_ref (options);
  server->dispatcher = rmg_dispatcher_ref ((RmgDispatcher *)dispatcher);

  server->sockfd = socket (AF_UNIX, SOCK_STREAM, 0);
  if (server->sockfd < 0)
    {
      g_warning ("Cannot create server socket");
      g_set_error (error, g_quark_from_static_string ("ServerNew"), 1, "Fail to create server socket");
    }
  else
    {
      timeout = rmg_options_long_for (options, KEY_IPC_TIMEOUT_SEC);

      tout.tv_sec = timeout;
      tout.tv_usec = 0;

      if (setsockopt (server->sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tout, sizeof(tout)) == -1)
        g_warning ("Failed to set the socket receiving timeout: %s", strerror (errno));

      if (setsockopt (server->sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tout, sizeof(tout)) == -1)
        g_warning ("Failed to set the socket sending timeout: %s", strerror (errno));
    }

  g_source_set_callback (RMG_EVENT_SOURCE (server), G_SOURCE_FUNC (server_source_callback),
                         server, server_source_destroy_notify);
  g_source_attach (RMG_EVENT_SOURCE (server), NULL);

  return server;
}

RmgServer *
rmg_server_ref (RmgServer *server)
{
  g_assert (server);
  g_ref_count_inc (&server->rc);
  return server;
}

void
rmg_server_unref (RmgServer *server)
{
  g_assert (server);

  if (g_ref_count_dec (&server->rc) == TRUE)
    {
      if (server->options != NULL)
        rmg_options_unref (server->options);

      if (server->dispatcher != NULL)
        rmg_dispatcher_unref ((RmgDispatcher *)server->dispatcher);

      g_source_unref (RMG_EVENT_SOURCE (server));
    }
}

RmgStatus
rmg_server_bind_and_listen (RmgServer *server)
{
  g_autofree gchar *sock_addr = NULL;
  RmgStatus status = RMG_STATUS_OK;
  struct sockaddr_un saddr;

  g_assert (server);

  sock_addr = rmg_options_string_for (server->options, KEY_IPC_SOCK_ADDR);

  unlink (sock_addr);

  memset (&saddr, 0, sizeof(struct sockaddr_un));
  saddr.sun_family = AF_UNIX;
  strncpy (saddr.sun_path, sock_addr, sizeof(saddr.sun_path) - 1);

  g_debug ("Server socket path %s", saddr.sun_path);

  if (bind (server->sockfd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_un)) != -1)
    {
      if (listen (server->sockfd, 10) == -1)
        {
          g_warning ("Server listen failed for path %s", saddr.sun_path);
          status = RMG_STATUS_ERROR;
        }
    }
  else
    {
      g_warning ("Server bind failed for path %s", saddr.sun_path);
      status = RMG_STATUS_ERROR;
    }

  if (status == RMG_STATUS_ERROR)
    {
      if (server->sockfd > 0)
        close (server->sockfd);

      server->sockfd = -1;
    }
  else
    {
      server->tag = g_source_add_unix_fd (RMG_EVENT_SOURCE (server),
                                          server->sockfd,
                                          G_IO_IN | G_IO_PRI);
    }

  return status;
}
