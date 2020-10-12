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
 * \file rmg-application.c
 */

#include "rmg-application.h"
#include "rmg-utils.h"

#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

RmgRunMode g_run_mode;

static RmgRunMode
get_run_mode (RmgOptions *options)
{
  g_autofree gchar *opt_runmode = NULL;
  g_autofree gchar *opt_sockaddr = NULL;

  g_assert (options);

  opt_runmode = rmg_options_string_for (options, KEY_RUN_MODE);

  if (g_strcmp0 (opt_runmode, "primary") == 0)
    return RUN_MODE_PRIMARY;
  else if (g_strcmp0 (opt_runmode, "replica") == 0)
    return RUN_MODE_REPLICA;

  /* auto is set so try auto detect */
  opt_sockaddr = rmg_options_string_for (options, KEY_IPC_SOCK_ADDR);

  if (g_access (opt_sockaddr, R_OK) == 0)
    return RUN_MODE_REPLICA;

  return RUN_MODE_PRIMARY;
}

static void
dbus_proxy_available_for_checker (gpointer _dbus_proxy, gpointer _checker)
{
  GDBusProxy *proxy = (GDBusProxy *)_dbus_proxy;
  RmgChecker *checker = (RmgChecker *)_checker;

  g_assert (proxy);
  g_assert (checker);

  rmg_checker_set_proxy (checker, proxy);
}

static void
dbus_proxy_available_for_executor (gpointer _dbus_proxy, gpointer _executor)
{
  GDBusProxy *proxy = (GDBusProxy *)_dbus_proxy;
  RmgExecutor *executor = (RmgExecutor *)_executor;

  g_assert (proxy);
  g_assert (executor);

  rmg_executor_set_proxy (executor, proxy);
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

  /* construct options noexept */
  app->options = rmg_options_new (config);

  /* construct journal and return if an error is set */
  app->journal = rmg_journal_new (app->options, error);
  if (*error != NULL)
    return app;

  rmg_journal_reload_units (app->journal, NULL);

  /* set global run mode flag */
  g_run_mode = get_run_mode (app->options);
  if (g_run_mode == RUN_MODE_PRIMARY)
    g_info ("Recovery manager running as primary");
  else
    g_info ("Recovery manager running as replica");

  /* construct executor */
  app->executor = rmg_executor_new (app->options, app->journal);

  /* once the executor is constructed the socket is created in server mode */
  rmg_sdnotify_send_ready (app->sdnotify);

  /* construct dispatcher and return if an error is set */
  app->dispatcher = rmg_dispatcher_new (app->options, app->journal, app->executor, error);
  if (*error != NULL)
    return app;

  /* construct monitor */
  app->monitor = rmg_monitor_new (app->dispatcher);
  rmg_monitor_build_proxy (app->monitor);
  rmg_monitor_read_services (app->monitor);

  app->checker = rmg_checker_new (app->journal, app->options);
  rmg_checker_check_services (app->checker);

  /* rmg-monitor builds the dbus proxy and we register a callback to get it for
   * checker and executor */
  rmg_monitor_register_proxy_available_callback (app->monitor,
                                                 dbus_proxy_available_for_checker,
                                                 (gpointer)app->checker);
  rmg_monitor_register_proxy_available_callback (app->monitor,
                                                 dbus_proxy_available_for_executor,
                                                 (gpointer)app->executor);

  if (g_run_mode == RUN_MODE_PRIMARY)
    {
      app->crashmonitor = rmg_crashmonitor_new (app->dispatcher);
      rmg_crashmonitor_build_proxy (app->crashmonitor);
    }

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

      if (app->checker != NULL)
        rmg_checker_unref (app->checker);

      if (app->crashmonitor != NULL)
        rmg_crashmonitor_unref (app->crashmonitor);

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
