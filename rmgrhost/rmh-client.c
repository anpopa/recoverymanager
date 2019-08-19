/* rmh-client.c
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

#include "rmh-client.h"
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
static void process_message (RmhClient *c, RmgMessage *msg);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs client_source_funcs =
{
  client_source_prepare,
  NULL,
  client_source_dispatch,
  NULL,
  NULL,
  NULL,
};

static gboolean
client_source_prepare (GSource *source,
                       gint *timeout)
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
client_source_dispatch (GSource *source,
                        GSourceFunc callback,
                        gpointer user_data)
{
  RMG_UNUSED (source);

  if (callback == NULL)
    return G_SOURCE_CONTINUE;

  return callback (user_data) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
client_source_callback (gpointer data)
{
  RmhClient *client = (RmhClient *)data;
  gboolean status = TRUE;
  RmgMessage msg;

  g_assert (client);

  rmg_message_init (&msg, RMG_CORE_UNKNOWN, 0);

  if (rmg_message_read (client->sockfd, &msg) != RMG_STATUS_OK)
    {
      g_debug ("Cannot read from client socket %d", client->sockfd);
      status = FALSE;
    }
  else
    {
      RmgMessageType type = rmg_message_get_type (&msg);

      process_message (client, &msg);

      /* TODO: More actions based on msg type */
    }

  return status;
}

static void
client_source_destroy_notify (gpointer data)
{
  RmhClient *client = (RmhClient *)data;

  g_assert (client);
  g_debug ("Client %d disconnected", client->sockfd);

  rmh_client_unref (client);
}

static void
process_message (RmhClient *c,
                 RmgMessage *msg)
{
  g_autofree gchar *tmp_id = NULL;
  g_autofree gchar *tmp_name = NULL;

  g_assert (c);
  g_assert (msg);

  if (strncmp ((char *)msg->hdr.version, RMG_BUILDTIME_VERSION, RMG_VERSION_STRING_LEN) != 0)
    g_warning ("Using different cdh header than coredumper");

  switch (msg->hdr.type)
    {
    case RMG_REQUEST_ACTION:
      g_assert (msg->data);
      break;

    default:
      break;
    }
}

RmhClient *
rmh_client_new (gint clientfd,
                RmgJournal *journal)
{
  RmhClient *client = (RmhClient *)g_source_new (&client_source_funcs, sizeof(RmhClient));

  g_assert (client);

  g_ref_count_init (&client->rc);

  client->sockfd = clientfd;
  client->journal = rmg_journal_ref (journal);

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

RmhClient *
rmh_client_ref (RmhClient *client)
{
  g_assert (client);
  g_ref_count_inc (&client->rc);
  return client;
}

void
rmh_client_unref (RmhClient *client)
{
  g_assert (client);

  if (g_ref_count_dec (&client->rc) == TRUE)
    {
      rmg_journal_unref (client->journal);
      g_source_unref (RMG_EVENT_SOURCE (client));
    }
}
