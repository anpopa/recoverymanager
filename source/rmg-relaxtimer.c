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
 * \file rmg-relaxtimer.c
 */

#include "rmg-relaxtimer.h"

static gboolean
relaxtimer_callback (gpointer user_data)
{
  RmgRelaxTimer *relaxtimer = (RmgRelaxTimer *)user_data;

  g_autoptr (GError) error = NULL;
  glong current_rvector = 0;

  g_assert (relaxtimer);

  current_rvector = rmg_journal_get_rvector (relaxtimer->journal, relaxtimer->service_name, &error);
  if (error != NULL)
    {
      g_warning ("Fail to read rvector on timer callback for service '%s'. Error: %s",
                 relaxtimer->service_name, error->message);
    }
  else
    {
      if (current_rvector == relaxtimer->rvector)
        {
          rmg_journal_set_rvector (relaxtimer->journal, relaxtimer->service_name, 0, &error);
          if (error != NULL)
            {
              g_warning ("Fail to reset rvector on timer callback for service '%s'. Error: %s",
                         relaxtimer->service_name, error->message);
            }
          else
            {
              g_info ("Service '%s' passed the relaxation time and is considered recovered",
                      relaxtimer->service_name);
            }
        }
    }

  return FALSE;
}

void
relaxtimer_destroy_notify (gpointer data)
{
  RmgRelaxTimer *relaxtimer = (RmgRelaxTimer *)data;

  g_assert (relaxtimer);

  rmg_journal_unref (relaxtimer->journal);
  g_free (relaxtimer->service_name);
  g_free (relaxtimer);
}

guint
rmg_relaxtimer_trigger (RmgJournal *journal, const gchar *service_name, GError **error)
{
  RmgRelaxTimer *relaxtimer = g_new0 (RmgRelaxTimer, 1);
  guint source = 0;

  g_ref_count_init (&relaxtimer->rc);

  relaxtimer->journal = rmg_journal_ref (journal);
  relaxtimer->service_name = g_strdup (service_name);

  relaxtimer->rvector = rmg_journal_get_rvector (journal, service_name, error);
  if (relaxtimer->rvector <= 0)
    {
      g_free (relaxtimer);
      g_return_val_if_reached (source);
    }

  relaxtimer->timeout = rmg_journal_get_relaxing_timeout (journal, service_name, error);
  if (relaxtimer->timeout <= 0)
    {
      g_free (relaxtimer);
      g_return_val_if_reached (source);
    }

  source = g_timeout_add_seconds_full (G_PRIORITY_DEFAULT,
                                       (guint)(relaxtimer->rvector * relaxtimer->timeout),
                                       relaxtimer_callback, relaxtimer, relaxtimer_destroy_notify);

  g_info ("Relaxation timer started for service='%s' timeout=%usec", service_name,
          (guint)(relaxtimer->rvector * relaxtimer->timeout));

  return source;
}
