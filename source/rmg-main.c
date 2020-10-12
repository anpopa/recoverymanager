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
 * \file rmg-main.c
 */

#include "rmg-application.h"
#include "rmg-utils.h"

#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef WITH_EPILOG
#include <cdh-epilog.h>
#endif

static GMainLoop *g_mainloop = NULL;

static void
terminate (int signum)
{
  g_info ("Recoverymanager terminate with signal %d", signum);
  if (g_mainloop != NULL)
    g_main_loop_quit (g_mainloop);
}

gint
main (gint argc, gchar *argv[])
{
  g_autoptr (GOptionContext) context = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (RmgApplication) app = NULL;
  g_autofree gchar *config_path = NULL;
  g_autofree gchar *logid = NULL;
  gboolean version = FALSE;
  RmgStatus status = RMG_STATUS_OK;

  GOptionEntry main_entries[] =
  { { "version", 'v', 0, G_OPTION_ARG_NONE, &version, "Show program version", "" },
    { "config", 'c', 0, G_OPTION_ARG_FILENAME, &config_path, "Override configuration file", "" },
    { "logid", 'i', 0, G_OPTION_ARG_STRING, &logid, "Use string as log id (default to RMGR)", "" },
    { 0 } };

  signal (SIGINT, terminate);
  signal (SIGTERM, terminate);
#ifdef WITH_EPILOG
  cdh_epilog_register_crash_handlers (NULL);
#endif

  context = g_option_context_new ("- Recovery manager service daemon");
  g_option_context_set_summary (context, "The system recovery managerdaemon");
  g_option_context_add_main_entries (context, main_entries, NULL);

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s\n", error->message);
      return EXIT_FAILURE;
    }

  if (version)
    {
      g_printerr ("%s\n", RMG_VERSION);
      return EXIT_SUCCESS;
    }

  if (logid == NULL)
    logid = g_strdup ("RMGR");

  rmg_logging_open (logid, "Recoverymanager service", "RMG", "Default context");

  if (config_path == NULL)
    config_path = g_build_filename (RMG_CONFIG_DIRECTORY, RMG_CONFIG_FILE_NAME, NULL);

  if (g_access (config_path, R_OK) == 0)
    {
      app = rmg_application_new (config_path, &error);

      if (error != NULL)
        {
          g_printerr ("%s\n", error->message);
          status = RMG_STATUS_ERROR;
        }
      else
        {
          g_info ("Recoverymanager service started for OS version '%s'",
                  rmg_utils_get_osversion ());
          g_mainloop = rmg_application_get_mainloop (app);
          status = rmg_application_execute (app);
        }
    }
  else
    g_warning ("Cannot open configuration file %s", config_path);

  rmg_logging_close ();

  return status == RMG_STATUS_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
