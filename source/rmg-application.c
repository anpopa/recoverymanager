/* rmg-application.c
 *
 * Copyright 2019 Alin Popa <alin.popa@fxapp.ro>
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

#include "rmg-application.h"
#include "rmg-utils.h"

#include <glib.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

RmgRunMode g_run_mode;

static RmgRunMode
get_run_mode (RmgOptions *options)
{
  g_autofree gchar *opt_runmode = NULL;
  g_autofree gchar *opt_sockaddr = NULL;

  g_assert (options);

  opt_runmode = rmg_options_string_for (options, KEY_RUN_MODE);

  if (g_strcmp0 (opt_runmode, "master") == 0)
    return RUN_MODE_MASTER;
  else if (g_strcmp0 (opt_runmode, "slave") == 0)
    return RUN_MODE_SLAVE;

  /* auto is set so try auto detect */
  opt_sockaddr = rmg_options_string_for (options, KEY_IPC_SOCK_ADDR);

  if (g_access (opt_sockaddr, R_OK) == 0)
    return RUN_MODE_SLAVE;

  return RUN_MODE_MASTER;
}

RmgApplication *
rmg_application_new (const gchar *config, GError **error)
{
  RmgApplication *app = g_new0 (RmgApplication, 1);

  g_assert (app);
  g_assert (error);

  g_ref_count_init (&app->rc);

  /* construct sdnotify noexept */
  app->sdnotify = rmg_sdnotify_new ();

  /* construct executor */
  app->executor = rmg_executor_new ();

  /* construct options noexept */
  app->options = rmg_options_new (config);

  /* construct journal and return if an error is set */
  app->journal = rmg_journal_new (app->options, error);
  if (*error != NULL)
    return app;

  rmg_journal_reload_units (app->journal, NULL);

  /* set global run mode flag */
  g_run_mode = get_run_mode (app->options);
  if (g_run_mode == RUN_MODE_MASTER)
    g_info ("Recovery manager running as master");
  else
    g_info ("Recovery manager running as slave");

  /* construct dispatcher and return if an error is set */
  app->dispatcher = rmg_dispatcher_new (app->options,
                                        app->journal,
                                        app->executor,
                                        error);
  if (*error != NULL)
    return app;

  /* construct dispatcher and return if an error is set */
  app->monitor = rmg_monitor_new (app->dispatcher, error);
  if (*error != NULL)
    return app;

  /* construct mainloop noexept */
  app->mainloop = g_main_loop_new (NULL, TRUE);

  return app;
}

RmgApplication *
rmg_application_ref (RmgApplication *app)
{
  g_assert (app);
  g_ref_count_inc (&app->rc);
  return app;
}

void
rmg_application_unref (RmgApplication *app)
{
  g_assert (app);

  if (g_ref_count_dec (&app->rc) == TRUE)
    {
      if (app->journal != NULL)
        rmg_journal_unref (app->journal);

      if (app->sdnotify != NULL)
        rmg_sdnotify_unref (app->sdnotify);

      if (app->options != NULL)
        rmg_options_unref (app->options);

      if (app->dispatcher != NULL)
        rmg_dispatcher_unref (app->dispatcher);

      if (app->monitor != NULL)
        rmg_monitor_unref (app->monitor);

      if (app->mainloop != NULL)
        g_main_loop_unref (app->mainloop);

      g_free (app);
    }
}

GMainLoop *
rmg_application_get_mainloop (RmgApplication *app)
{
  g_assert (app);
  return app->mainloop;
}

RmgStatus
rmg_application_execute (RmgApplication *app)
{
  /* run the main event loop */
  g_main_loop_run (app->mainloop);

  return RMG_STATUS_OK;
}
