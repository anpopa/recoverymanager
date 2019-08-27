/* rmg-relaxtimer.c
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
                 relaxtimer->service_name,
                 error->message);
    }
  else
    {
      if (current_rvector == relaxtimer->rvector)
        {
          rmg_journal_set_rvector (relaxtimer->journal, relaxtimer->service_name, 0, &error);
          if (error != NULL)
            {
              g_warning ("Fail to reset rvector on timer callback for service '%s'. Error: %s",
                         relaxtimer->service_name,
                         error->message);
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
                                       relaxtimer_callback,
                                       relaxtimer,
                                       relaxtimer_destroy_notify);

  g_info ("Relaxation timer started for service='%s' timeout=%usec",
          service_name,
          (guint)(relaxtimer->rvector * relaxtimer->timeout));

  return source;
}
