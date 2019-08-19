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

#include "rmn-application.h"
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

RmnApplication *
rmn_application_new (const gchar *config, GError **error)
{
  RmnApplication *app = g_new0 (RmnApplication, 1);
  g_autofree gchar *opt_dbdir = NULL;

  g_assert (app);
  g_assert (error);

  g_ref_count_init (&app->rc);

  /* construct sdnotify noexept */
  app->sdnotify = rmg_sdnotify_new ();

  /* construct options noexept */
  app->options = rmg_options_new (config);

  opt_dbdir = rmg_options_string_for (app->options, KEY_DATABASE_DIR);
  if (g_mkdir_with_parents (opt_dbdir, 0755) != 0)
    {
      g_set_error (error, 
                   g_quark_from_static_string ("ApplicationNew"), 
                   1, 
                   "Cannot create database directory");
      return app;
    }

  /* construct journal and return if an error is set */
  app->journal = rmg_journal_new (app->options, error);
  if (*error != NULL)
    return app;

  /* construct master manager and return if an error is set */
  app->manager = rmn_manager_new (app->options);
  if (rmn_manager_connect (app->manager) != RMG_STATUS_OK)
    {
      g_set_error (error, 
                   g_quark_from_static_string ("ApplicationNew"), 
                   1, 
                   "Cannot connect to master socket in slave mode");
          return app;
    }

  /* construct mainloop noexept */
  app->mainloop = g_main_loop_new (NULL, TRUE);

  return app;
}

RmnApplication *
rmn_application_ref (RmnApplication *app)
{
  g_assert (app);
  g_ref_count_inc (&app->rc);
  return app;
}

void
rmn_application_unref (RmnApplication *app)
{
  g_assert (app);

  if (g_ref_count_dec (&app->rc) == TRUE)
    {
      if (app->manager != NULL)
        rmn_manager_unref (app->manager);
      
      if (app->journal != NULL)
        rmg_journal_unref (app->journal);
      
      if (app->sdnotify != NULL)
        rmg_sdnotify_unref (app->sdnotify);
      
      if (app->options != NULL)
        rmg_options_unref (app->options);

      if (app->mainloop != NULL)
        g_main_loop_unref (app->mainloop);

      g_free (app);
    }
}

GMainLoop *
rmn_application_get_mainloop (RmnApplication *app)
{
  g_assert (app);
  return app->mainloop;
}

RmgStatus
rmn_application_execute (RmnApplication *app)
{
  /* run the main event loop */
  g_main_loop_run (app->mainloop);

  return RMG_STATUS_OK;
}
