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
 * \file rmg-sdnotify.c
 */

#include "rmg-sdnotify.h"

#include <sys/timerfd.h>
#include <systemd/sd-daemon.h>

#define USEC2SEC(x) (guint) (x / 1000000)
#define USEC2SECHALF(x) (guint) (x / 1000000 / 2)

/**
 * @brief GSource callback function
 */
static gboolean source_timer_callback (gpointer data);

/**
 * @brief GSource destroy notification callback function
 */
static void source_destroy_notify (gpointer data);

static gboolean
source_timer_callback (gpointer data)
{
  RMG_UNUSED (data);

  if (sd_notify (0, "WATCHDOG=1") < 0)
    g_warning ("Fail to send the heartbeet to systemd");
  else
    g_debug ("Watchdog heartbeat sent");

  return TRUE;
}

static void
source_destroy_notify (gpointer data)
{
  RMG_UNUSED (data);
  g_info ("System watchdog disabled");
}

void
rmg_sdnotify_send_ready (RmgSDNotify *sdnotify)
{
  RMG_UNUSED (sdnotify);

  if (sd_notify (0, "READY=1") < 0)
    g_warning ("Fail to send ready state to systemd");
}

RmgSDNotify *
rmg_sdnotify_new (void)
{
  RmgSDNotify *sdnotify = g_new0 (RmgSDNotify, 1);
  gint sdw_status;
  guint64 usec = 0;

  g_assert (sdnotify);

  g_ref_count_init (&sdnotify->rc);

  sdw_status = sd_watchdog_enabled (0, &usec);

  if (sdw_status > 0)
    {
      g_info ("Systemd watchdog enabled with timeout %u seconds", USEC2SEC (usec));

      sdnotify->source = g_timeout_source_new_seconds (USEC2SECHALF (usec));
      g_source_ref (sdnotify->source);

      g_source_set_callback (sdnotify->source, G_SOURCE_FUNC (source_timer_callback), sdnotify,
                             source_destroy_notify);
      g_source_attach (sdnotify->source, NULL);
    }
  else
    {
      if (sdw_status == 0)
        g_info ("Systemd watchdog disabled");
      else
        g_warning ("Fail to get the systemd watchdog status");
    }

  return sdnotify;
}

RmgSDNotify *
rmg_sdnotify_ref (RmgSDNotify *sdnotify)
{
  g_assert (sdnotify);
  g_ref_count_inc (&sdnotify->rc);
  return sdnotify;
}

void
rmg_sdnotify_unref (RmgSDNotify *sdnotify)
{
  g_assert (sdnotify);

  if (g_ref_count_dec (&sdnotify->rc) == TRUE)
    {
      if (sdnotify->source != NULL)
        g_source_unref (sdnotify->source);

      g_free (sdnotify);
    }
}
