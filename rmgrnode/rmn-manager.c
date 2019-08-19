/* rmg-manager.c
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

#include "rmn-manager.h"
#include "rmg-defaults.h"

#include <errno.h>
#include <stdio.h>

RmnManager *
rmn_manager_new (RmgOptions *opts)
{
  RmnManager *c = g_new0 (RmnManager, 1);

  g_assert (opts);

  g_ref_count_init (&c->rc);

  c->sfd = -1;
  c->connected = false;
  c->opts = rmg_options_ref (opts);

  return c;
}

RmnManager *
rmn_manager_ref (RmnManager *c)
{
  g_assert (c);
  g_ref_count_inc (&c->rc);
  return c;
}

void
rmn_manager_unref (RmnManager *c)
{
  g_assert (c);

  if (g_ref_count_dec (&c->rc) == TRUE)
    {
      if (rmn_manager_connected (c) == TRUE)
        (void)rmn_manager_disconnect (c);

      rmg_options_unref (c->opts);
      g_free (c);
    }
}

RmgStatus
rmn_manager_connect (RmnManager *c)
{
  g_autofree gchar *opt_sock_addr = NULL;
  g_autofree gchar *opt_run_dir = NULL;
  struct timeval tout;
  glong opt_timeout;

  g_assert (c);

  if (c->connected)
    return RMG_STATUS_ERROR;

  c->sfd = socket (AF_UNIX, SOCK_STREAM, 0);
  if (c->sfd < 0)
    {
      g_warning ("Cannot create connection socket");
      return RMG_STATUS_ERROR;
    }

  opt_run_dir = rmg_options_string_for (c->opts, KEY_RUN_DIR);
  opt_sock_addr = rmg_options_string_for (c->opts, KEY_IPC_SOCK_ADDR);
  opt_timeout = rmg_options_long_for (c->opts, KEY_IPC_TIMEOUT_SEC);

  memset (&c->saddr, 0, sizeof(struct sockaddr_un));
  c->saddr.sun_family = AF_UNIX;

  snprintf (c->saddr.sun_path, (sizeof(c->saddr.sun_path) - 1), "%s/%s", opt_run_dir,
            opt_sock_addr);

  if (connect (c->sfd, (struct sockaddr *)&c->saddr, sizeof(struct sockaddr_un)) < 0)
    {
      g_info ("Core manager not available: %s", c->saddr.sun_path);
      close (c->sfd);
      return RMG_STATUS_ERROR;
    }

  tout.tv_sec = opt_timeout;
  tout.tv_usec = 0;

  if (setsockopt (c->sfd, SOL_SOCKET, SO_RCVTIMEO, (gchar*)&tout, sizeof(tout)) == -1)
    g_warning ("Failed to set the socket receiving timeout: %s", strerror (errno));

  if (setsockopt (c->sfd, SOL_SOCKET, SO_SNDTIMEO, (gchar*)&tout, sizeof(tout)) == -1)
    g_warning ("Failed to set the socket sending timeout: %s", strerror (errno));

  c->connected = true;

  return RMG_STATUS_OK;
}

RmgStatus
rmn_manager_disconnect (RmnManager *c)
{
  if (!c->connected)
    return RMG_STATUS_ERROR;

  if (c->sfd > 0)
    {
      close (c->sfd);
      c->sfd = -1;
    }

  c->connected = false;

  return RMG_STATUS_OK;
}

gboolean
rmn_manager_connected (RmnManager *c)
{
  g_assert (c);
  return c->connected;
}

RmgStatus
rmn_manager_send (RmnManager *c,
                  RmgMessage *m)
{
  fd_set wfd;
  struct timeval tv;
  RmgStatus status = RMG_STATUS_OK;

  g_assert (c);
  g_assert (m);

  if (c->sfd < 0 || !c->connected)
    {
      g_warning ("No connection to manager");
      return RMG_STATUS_ERROR;
    }

  rmg_message_set_version (m, RMG_VERSION);

  FD_ZERO (&wfd);

  tv.tv_sec = MANAGER_SELECT_TIMEOUT;
  tv.tv_usec = 0;
  FD_SET (c->sfd, &wfd);

  status = select (c->sfd + 1, NULL, &wfd, NULL, &tv);
  if (status == -1)
    {
      g_warning ("Server socket select failed");
    }
  else
    {
      if (status > 0)
        status = rmg_message_write (c->sfd, m);
    }

  return status;
}
