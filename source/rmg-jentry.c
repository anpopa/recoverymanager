/* rmg-jentry.c
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

#include "rmg-jentry.h"

static void
action_entry_free (gpointer entry)
{
  g_free (entry);
}

RmgJEntry *
rmg_jentry_new (gulong version)
{
  RmgJEntry *jentry = g_new0 (RmgJEntry, 1);

  g_ref_count_init (&jentry->rc);

  jentry->hash = version;
  jentry->relaxing = FALSE;
  jentry->actions = NULL;

  /* the first action will start when rvector 1 */
  jentry->rvector = 1;

  return jentry;
}

RmgJEntry *
rmg_jentry_ref (RmgJEntry *jentry)
{
  g_assert (jentry);
  g_ref_count_inc (&jentry->rc);
  return jentry;
}

void
rmg_jentry_unref (RmgJEntry *jentry)
{
  g_assert (jentry);

  if (g_ref_count_dec (&jentry->rc) == TRUE)
    {
      if (jentry->name != NULL)
        g_free (jentry->name);

      if (jentry->private_data != NULL)
        g_free (jentry->private_data);

      if (jentry->public_data != NULL)
        g_free (jentry->public_data);

      if (jentry->hash_generator != NULL)
        g_rand_free (jentry->hash_generator);

      g_list_free_full (jentry->actions, action_entry_free);

      g_free (jentry);
    }
}

void
rmg_jentry_set_name (RmgJEntry *jentry, const gchar *name)
{
  g_assert (jentry);
  g_assert (name);

  jentry->name = g_strdup (name);

  /* we have the name so we can initialize the hash generator for actions */
  jentry->hash_generator = g_rand_new_with_seed ((guint32)jentry->hash);
}

void
rmg_jentry_set_private_data_path (RmgJEntry *jentry, const gchar *dpath)
{
  g_assert (jentry);
  g_assert (dpath);

  jentry->private_data = g_strdup (dpath);
}

void
rmg_jentry_set_public_data_path (RmgJEntry *jentry, const gchar *dpath)
{
  g_assert (jentry);
  g_assert (dpath);

  jentry->public_data = g_strdup (dpath);
}

void
rmg_jentry_set_rvector (RmgJEntry *jentry, glong rvector)
{
  g_assert (jentry);
  jentry->rvector = rvector;
}

void
rmg_jentry_set_relaxing (RmgJEntry *jentry, gboolean relaxing)
{
  g_assert (jentry);
  jentry->relaxing = relaxing;
}

void
rmg_jentry_set_timeout (RmgJEntry *jentry, glong timeout)
{
  g_assert (jentry);
  jentry->timeout = timeout;
}

void
rmg_jentry_add_action (RmgJEntry *jentry,
                       RmgActionType type,
                       glong trigger_level_min,
                       glong trigger_level_max)
{
  RmgAEntry *action = g_new0 (RmgAEntry, 1);

  g_assert (jentry);

  action->hash = (gulong)g_rand_int (jentry->hash_generator);

  action->type = type;
  action->trigger_level_min = trigger_level_min;
  action->trigger_level_max = trigger_level_max;

  jentry->actions = g_list_append (jentry->actions, RMG_AENTRY_TO_PTR (action));
}

gulong
rmg_jentry_get_hash (RmgJEntry *jentry)
{
  g_assert (jentry);
  return jentry->hash;
}

const gchar *
rmg_jentry_get_name (RmgJEntry *jentry)
{
  g_assert (jentry);
  return jentry->name;
}

const gchar *
rmg_jentry_get_private_data_path (RmgJEntry *jentry)
{
  g_assert (jentry);
  return jentry->private_data;
}

const gchar *
rmg_jentry_get_public_data_path (RmgJEntry *jentry)
{
  g_assert (jentry);
  return jentry->public_data;
}

glong
rmg_jentry_get_rvector (RmgJEntry *jentry)
{
  g_assert (jentry);
  return jentry->rvector;
}

gboolean
rmg_jentry_get_relaxing (RmgJEntry *jentry)
{
  g_assert (jentry);
  return jentry->relaxing;
}

glong
rmg_jentry_get_timeout (RmgJEntry *jentry)
{
  g_assert (jentry);
  return jentry->timeout;
}

const GList *
rmg_jentry_get_actions (RmgJEntry *jentry)
{
  g_assert (jentry);
  return jentry->actions;
}

