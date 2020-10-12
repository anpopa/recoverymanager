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
 * \file rmg-jentry.c
 */

#include "rmg-jentry.h"

static void
action_entry_free (gpointer _entry)
{
  g_assert (_entry);
  g_free (_entry);
}

static void
friend_entry_free (gpointer _entry)
{
  RmgFEntry *entry = (RmgFEntry *)_entry;

  g_assert (entry);

  g_free (entry->friend_name);
  g_free (entry->friend_context);
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

  /* the first action will start when rvector is 1 */
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
      g_list_free_full (jentry->friends, friend_entry_free);

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
rmg_jentry_set_checkstart (RmgJEntry *jentry, gboolean check_start)
{
  g_assert (jentry);
  jentry->check_start = check_start;
}

void
rmg_jentry_add_action (RmgJEntry *jentry,
                       RmgActionType type,
                       glong trigger_level_min,
                       glong trigger_level_max,
                       gboolean reset_after)
{
  RmgAEntry *action = g_new0 (RmgAEntry, 1);

  g_assert (jentry);

  action->hash = (gulong)g_rand_int (jentry->hash_generator);

  action->type = type;
  action->trigger_level_min = trigger_level_min;
  action->trigger_level_max = trigger_level_max;
  action->reset_after = reset_after;

  jentry->actions = g_list_append (jentry->actions, RMG_AENTRY_TO_PTR (action));
}

void
rmg_jentry_add_friend (RmgJEntry *jentry,
                       const gchar *friend_name,
                       const gchar *friend_context,
                       RmgFriendType type,
                       RmgFriendActionType action,
                       glong argument,
                       glong delay)
{
  RmgFEntry *friend = g_new0 (RmgFEntry, 1);

  g_assert (jentry);
  g_assert (friend_name);
  g_assert (friend_context);

  friend->hash = (gulong)g_rand_int (jentry->hash_generator);
  friend->friend_name = g_strdup (friend_name);
  friend->friend_context = g_strdup (friend_context);
  friend->type = type;
  friend->action = action;
  friend->delay = delay;
  friend->argument = argument;

  jentry->friends = g_list_append (jentry->friends, RMG_FENTRY_TO_PTR (friend));
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

gboolean
rmg_jentry_get_checkstart (RmgJEntry *jentry)
{
  g_assert (jentry);
  return jentry->check_start;
}

const GList *
rmg_jentry_get_actions (RmgJEntry *jentry)
{
  g_assert (jentry);
  return jentry->actions;
}

const GList *
rmg_jentry_get_friends (RmgJEntry *jentry)
{
  g_assert (jentry);
  return jentry->friends;
}
