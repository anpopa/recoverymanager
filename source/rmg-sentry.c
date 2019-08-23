/* rmg-sentry.c
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

#include "rmg-sentry.h"

static void
action_entry_free (gpointer entry)
{
  g_free (entry);
}

RmgSEntry *
rmg_sentry_new (gulong version)
{
  RmgSEntry *sentry = g_new0 (RmgSEntry, 1);

  g_ref_count_init (&sentry->rc);

  sentry->hash = version;
  sentry->relaxing = FALSE;
  sentry->rvector = 0;
  sentry->actions = NULL;

  return sentry;
}

RmgSEntry *
rmg_sentry_ref (RmgSEntry *sentry)
{
  g_assert (sentry);
  g_ref_count_inc (&sentry->rc);
  return sentry;
}

void
rmg_sentry_unref (RmgSEntry *sentry)
{
  g_assert (sentry);

  if (g_ref_count_dec (&sentry->rc) == TRUE)
    {
      if (sentry->name != NULL)
        g_free (sentry->name);

      if (sentry->private_data != NULL)
        g_free (sentry->private_data);

      if (sentry->public_data != NULL)
        g_free (sentry->public_data);

      if (sentry->hash_generator != NULL)
        g_rand_free (sentry->hash_generator);

      g_list_free_full (sentry->actions, action_entry_free);

      g_free (sentry);
    }
}

void
rmg_sentry_set_name (RmgSEntry *sentry, const gchar *name)
{
  g_assert (sentry);
  g_assert (name);

  sentry->name = g_strdup (name);

  /* we have the name so we can initialize the hash generator for actions */
  sentry->hash_generator = g_rand_new_with_seed ((guint32)sentry->hash);
}

void
rmg_sentry_set_private_data_path (RmgSEntry *sentry, const gchar *dpath)
{
  g_assert (sentry);
  g_assert (dpath);

  sentry->private_data = g_strdup (dpath);
}

void
rmg_sentry_set_public_data_path (RmgSEntry *sentry, const gchar *dpath)
{
  g_assert (sentry);
  g_assert (dpath);

  sentry->public_data = g_strdup (dpath);
}

void
rmg_sentry_set_rvector (RmgSEntry *sentry, glong rvector)
{
  g_assert (sentry);
  sentry->rvector = rvector;
}

void
rmg_sentry_set_relaxing (RmgSEntry *sentry, gboolean relaxing)
{
  g_assert (sentry);
  sentry->relaxing = relaxing;
}

void
rmg_sentry_set_timeout (RmgSEntry *sentry, glong timeout)
{
  g_assert (sentry);
  sentry->timeout = timeout;
}

void
rmg_sentry_add_action (RmgSEntry *sentry,
                       RmgActionType type,
                       glong trigger_level_min,
                       glong trigger_level_max)
{
  RmgAEntry *action = g_new0 (RmgAEntry, 1);

  g_assert (sentry);

  action->hash = (gulong)g_rand_int (sentry->hash_generator);

  action->type = type;
  action->trigger_level_min = trigger_level_min;
  action->trigger_level_max = trigger_level_max;

  sentry->actions = g_list_append (sentry->actions, RMG_AENTRY_TO_PTR (action));
}

gulong
rmg_sentry_get_hash (RmgSEntry *sentry)
{
  g_assert (sentry);
  return sentry->hash;
}

const gchar *
rmg_sentry_get_name (RmgSEntry *sentry)
{
  g_assert (sentry);
  return sentry->name;
}

const gchar *
rmg_sentry_get_private_data_path (RmgSEntry *sentry)
{
  g_assert (sentry);
  return sentry->private_data;
}

const gchar *
rmg_sentry_get_public_data_path (RmgSEntry *sentry)
{
  g_assert (sentry);
  return sentry->public_data;
}

glong
rmg_sentry_get_rvector (RmgSEntry *sentry)
{
  g_assert (sentry);
  return sentry->rvector;
}

gboolean
rmg_sentry_get_relaxing (RmgSEntry *sentry)
{
  g_assert (sentry);
  return sentry->relaxing;
}

glong
rmg_sentry_get_timeout (RmgSEntry *sentry)
{
  g_assert (sentry);
  return sentry->timeout;
}

const GList *
rmg_sentry_get_actions (RmgSEntry *sentry)
{
  g_assert (sentry);
  return sentry->actions;
}

