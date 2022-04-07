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
 * \file rmg-journal.c
 */

#include "rmg-journal.h"
#include "rmg-defaults.h"
#include "rmg-jentry.h"
#include "rmg-utils.h"

/**
 * @enum Journal query type
 */
typedef enum _JournalQueryType
{
  QUERY_CREATE_TABLES,
  QUERY_GET_HASH,
  QUERY_ADD_SERVICE,
  QUERY_REMOVE_SERVICE,
  QUERY_ADD_ACTION,
  QUERY_ADD_FRIEND,
  QUERY_GET_PRIVATE_DATA,
  QUERY_GET_PUBLIC_DATA,
  QUERY_GET_TIMEOUT,
  QUERY_GET_CHECK_START,
  QUERY_GET_RVECTOR,
  QUERY_SET_RVECTOR,
  QUERY_GET_ACTION,
  QUERY_GET_ACTION_RESET_AFTER,
  QUERY_GET_SERVICES_FOR_FRIEND,
  QUERY_CALL_RELAXING,
  QUERY_CALL_CHECK_START,
} JournalQueryType;

/**
 * @enum Journal query data object
 */
typedef struct _JournalQueryData
{
  JournalQueryType type;
  gpointer response;
  RmgJournal *journal;
  RmgJournalCallback callback;
} JournalQueryData;

/**
 * @struct Add action object helper
 */
typedef struct _JournalAddAction
{
  RmgJournal *journal;
  RmgJEntry *service;
} JournalAddAction;

/**
 * @struct Add action object helper
 */
typedef struct _JournalAddFriend
{
  RmgJournal *journal;
  RmgJEntry *service;
} JournalAddFriend;

const gchar *rmg_table_services = "Services";
const gchar *rmg_table_actions = "Actions";
const gchar *rmg_table_friends = "Friends";

/**
 * @brief SQlite3 callback
 */
static int sqlite_callback (void *data, int argc, char **argv, char **colname);

/**
 * @brief Parser markup callback for element start
 */
static void parser_start_element (GMarkupParseContext *context, const gchar *element_name,
                                  const gchar **attribute_names, const gchar **attribute_values,
                                  gpointer user_data, GError **error);

/**
 * @brief Parser markup callback for element text data
 */
static void parser_text_data (GMarkupParseContext *context, const gchar *text, gsize text_len,
                              gpointer user_data, GError **error);

/**
 * @brief Markup parser definition
 */
static GMarkupParser markup_parser = { parser_start_element, NULL, parser_text_data, NULL, NULL };

static int
sqlite_callback (void *data, int argc, char **argv, char **colname)
{
  JournalQueryData *querydata = (JournalQueryData *)data;

  switch (querydata->type)
    {
    case QUERY_CREATE_TABLES:
      break;

    case QUERY_GET_HASH:
      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "HASH") == 0)
            *((gulong *)(querydata->response)) = (gulong)g_ascii_strtoull (argv[i], NULL, 10);
        }
      break;

    case QUERY_ADD_SERVICE:
      break;

    case QUERY_REMOVE_SERVICE:
      break;

    case QUERY_ADD_ACTION:
      break;

    case QUERY_ADD_FRIEND:
      break;

    case QUERY_GET_PRIVATE_DATA:
      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "PRIVDATA") == 0)
            querydata->response = (gpointer)g_strdup (argv[i]);
        }
      break;

    case QUERY_GET_PUBLIC_DATA:
      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "PUBLDATA") == 0)
            querydata->response = (gpointer)g_strdup (argv[i]);
        }
      break;

    case QUERY_GET_TIMEOUT:
      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "TIMEOUT") == 0)
            *((glong *)(querydata->response)) = (glong)g_ascii_strtoll (argv[i], NULL, 10);
        }
      break;

    case QUERY_GET_CHECK_START:
      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "CHKSTART") == 0)
            *((gboolean *)(querydata->response)) = (gboolean)g_ascii_strtoll (argv[i], NULL, 10);
        }
      break;

    case QUERY_GET_RVECTOR:
      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "RVECTOR") == 0)
            *((glong *)(querydata->response)) = (glong)g_ascii_strtoll (argv[i], NULL, 10);
        }
      break;

    case QUERY_SET_RVECTOR:
      break;

    case QUERY_GET_ACTION:
      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "TYPE") == 0)
            {
              *((RmgActionType *)(querydata->response))
                  = (RmgActionType)g_ascii_strtoll (argv[i], NULL, 10);
            }
        }
      break;

    case QUERY_GET_ACTION_RESET_AFTER:
      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "RESET") == 0)
            *((gboolean *)(querydata->response)) = (gboolean)g_ascii_strtoll (argv[i], NULL, 10);
        }
      break;

    case QUERY_GET_SERVICES_FOR_FRIEND:
      {
        RmgFriendResponseEntry *friend_response = g_new0 (RmgFriendResponseEntry, 1);
        GList **services = (GList **)(querydata->response);

        for (gint i = 0; i < argc; i++)
          {
            if (g_strcmp0 (colname[i], "SERVICE") == 0)
              friend_response->service_name = g_strdup (argv[i]);
            else if (g_strcmp0 (colname[i], "ACTION") == 0)
              friend_response->action = (RmgFriendActionType)g_ascii_strtoll (argv[i], NULL, 10);
            else if (g_strcmp0 (colname[i], "ARGUMENT") == 0)
              friend_response->argument = (glong)g_ascii_strtoll (argv[i], NULL, 10);
            else if (g_strcmp0 (colname[i], "DELAY") == 0)
              friend_response->delay = (glong)g_ascii_strtoll (argv[i], NULL, 10);
          }

        *services = g_list_append (*services, friend_response);
      }
      break;

    case QUERY_CALL_RELAXING: /* fallthrough */
    case QUERY_CALL_CHECK_START:
      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "NAME") == 0)
            {
              if (querydata->callback != NULL)
                querydata->callback (querydata->journal, (gpointer)argv[i]);
            }
        }
      break;

    default:
      break;
    }

  return (0);
}

RmgJournal *
rmg_journal_new (RmgOptions *options, GError **error)
{
  RmgJournal *journal = NULL;
  g_autofree gchar *opt_dbdir = NULL;
  g_autofree gchar *dbfile = NULL;
  gchar *query_error = NULL;

  JournalQueryData data
      = { .type = QUERY_CREATE_TABLES, .journal = journal, .callback = NULL, .response = NULL };

  journal = g_new0 (RmgJournal, 1);

  g_assert (journal);

  g_ref_count_init (&journal->rc);
  journal->options = rmg_options_ref (options);

  opt_dbdir = rmg_options_string_for (options, KEY_DATABASE_DIR);
  dbfile = g_build_filename (opt_dbdir, RMG_DATABASE_FILE_NAME, NULL);

  if (sqlite3_open (dbfile, &journal->database))
    {
      g_warning ("Cannot open journal database at path %s", dbfile);
      g_set_error (error, g_quark_from_static_string ("JournalNew"), 1, "Database open failed");
    }
  else
    {
      g_autofree gchar *services_sql = NULL;

      services_sql = g_strdup_printf ("CREATE TABLE IF NOT EXISTS %s        "
                                      "(HASH      UNSIGNED INTEGER PRIMARY KEY NOT NULL, "
                                      " NAME      TEXT            NOT NULL, "
                                      " PRIVDATA  TEXT            NOT NULL, "
                                      " PUBLDATA  TEXT            NOT NULL, "
                                      " RVECTOR   NUMERIC         NOT NULL, "
                                      " CHKSTART  NUMERIC         NOT NULL, "
                                      " TIMEOUT   NUMERIC         NOT NULL);",
                                      rmg_table_services);

      if (sqlite3_exec (journal->database, services_sql, sqlite_callback, &data, &query_error)
          != SQLITE_OK)
        {
          g_warning ("Fail to create services table. SQL error %s", query_error);
          g_set_error (error, g_quark_from_static_string ("JournalNew"), 1,
                       "Create services table fail");
        }
      else
        {
          g_autofree gchar *actions_sql = NULL;
          g_autofree gchar *friends_sql = NULL;

          actions_sql = g_strdup_printf ("CREATE TABLE IF NOT EXISTS %s        "
                                         "(HASH     UNSIGNED INTEGER PRIMARY KEY NOT NULL, "
                                         " SERVICE  TEXT       NOT   NULL, "
                                         " TYPE     NUMERIC    NOT   NULL, "
                                         " TLMIN    NUMERIC    NOT   NULL, "
                                         " TLMAX    NUMERIC    NOT   NULL, "
                                         " RESET    NUMERIC    NOT   NULL);",
                                         rmg_table_actions);

          if (sqlite3_exec (journal->database, actions_sql, sqlite_callback, &data, &query_error)
              != SQLITE_OK)
            {
              g_warning ("Fail to create actions table. SQL error %s", query_error);
              g_set_error (error, g_quark_from_static_string ("JournalNew"), 1,
                           "Create actions table fail");
            }

          friends_sql = g_strdup_printf ("CREATE TABLE IF NOT EXISTS %s        "
                                         "(HASH     UNSIGNED INTEGER PRIMARY KEY NOT NULL, "
                                         " SERVICE  TEXT       NOT   NULL, "
                                         " FRIEND   TEXT       NOT   NULL, "
                                         " CONTEXT  TEXT       NOT   NULL, "
                                         " TYPE     NUMERIC    NOT   NULL, "
                                         " ACTION   NUMERIC    NOT   NULL, "
                                         " ARGUMENT NUMERIC    NOT   NULL, "
                                         " DELAY    NUMERIC    NOT   NULL);",
                                         rmg_table_friends);

          if (sqlite3_exec (journal->database, friends_sql, sqlite_callback, &data, &query_error)
              != SQLITE_OK)
            {
              g_warning ("Fail to create friends table. SQL error %s", query_error);
              g_set_error (error, g_quark_from_static_string ("JournalNew"), 1,
                           "Create friends table fail");
            }
        }
    }

  return journal;
}

RmgJournal *
rmg_journal_ref (RmgJournal *journal)
{
  g_assert (journal);
  g_ref_count_inc (&journal->rc);
  return journal;
}

void
rmg_journal_unref (RmgJournal *journal)
{
  g_assert (journal);

  if (g_ref_count_dec (&journal->rc) == TRUE)
    {
      if (journal->options)
        rmg_options_unref (journal->options);

      g_free (journal);
    }
}

static void
parser_start_element (GMarkupParseContext *context, const gchar *element_name,
                      const gchar **attribute_names, const gchar **attribute_values,
                      gpointer user_data, GError **error)
{
  RmgJEntry *entry = (RmgJEntry *)user_data;

  entry->parser_current_element = element_name;

  RMG_UNUSED (context);
  RMG_UNUSED (error);

  if (g_strcmp0 (element_name, "action") == 0)
    {
      RmgActionType action_type = ACTION_INVALID;
      gboolean reset_after = FALSE;
      glong retry = 1;

      for (gint i = 0; attribute_names[i] != NULL; i++)
        {
          if (g_strcmp0 (attribute_names[i], "type") == 0)
            {
              if (attribute_values[i] != NULL)
                action_type = rmg_utils_action_type_from (attribute_values[i]);
            }
          else if (g_strcmp0 (attribute_names[i], "retry") == 0)
            {
              if (attribute_values[i] != NULL)
                retry = (glong)g_ascii_strtoll (attribute_values[i], NULL, 10);
            }
          else if (g_strcmp0 (attribute_names[i], "reset") == 0)
            {
              if (g_strcmp0 (attribute_values[i], "true") == 0)
                reset_after = TRUE;
            }
        }

      if (retry < 1 || action_type == ACTION_INVALID)
        g_warning ("Invalid action settings");
      else
        {
          glong g = rmg_jentry_get_rvector (entry) + retry;

          rmg_jentry_add_action (entry, action_type, rmg_jentry_get_rvector (entry), g,
                                 reset_after);
          rmg_jentry_set_rvector (entry, g);
        }
    }
  else if (g_strcmp0 (element_name, "friend") == 0)
    {
      RmgFEntryParserHelper *friend = &entry->parser_current_friend;

      memset (friend, 0, sizeof (entry->parser_current_friend));

      for (gint i = 0; attribute_names[i] != NULL; i++)
        {
          if (g_strcmp0 (attribute_names[i], "type") == 0)
            {
              if (attribute_values[i] != NULL)
                friend->type = rmg_utils_friend_type_from (attribute_values[i]);
            }
          else if (g_strcmp0 (attribute_names[i], "action") == 0)
            {
              if (attribute_values[i] != NULL)
                friend->action = rmg_utils_friend_action_type_from (attribute_values[i]);
            }
          else if (g_strcmp0 (attribute_names[i], "delay") == 0)
            {
              if (attribute_values[i] != NULL)
                friend->delay = (glong)g_ascii_strtoll (attribute_values[i], NULL, 10);
            }
          else if (g_strcmp0 (attribute_names[i], "arg") == 0)
            {
              if (attribute_values[i] != NULL)
                friend->argument = (glong)g_ascii_strtoll (attribute_values[i], NULL, 10);
            }
          else if (g_strcmp0 (attribute_names[i], "context") == 0)
            {
              if (attribute_values[i] != NULL)
                friend->friend_context = g_strdup (attribute_values[i]);
            }
        }
    }
  else if (g_strcmp0 (element_name, "service") == 0)
    {
      for (gint i = 0; attribute_names[i] != NULL; i++)
        {
          if (g_strcmp0 (attribute_names[i], "relaxtime") == 0)
            {
              glong relaxtime = (glong)g_ascii_strtoll (attribute_values[i], NULL, 10);
              rmg_jentry_set_timeout (entry, (relaxtime > 0 ? relaxtime : 5));
            }
          else if (g_strcmp0 (attribute_names[i], "checkstart") == 0)
            {
              gboolean check_start = FALSE;

              if (g_strcmp0 (attribute_values[i], "true") == 0)
                check_start = TRUE;

              rmg_jentry_set_checkstart (entry, check_start);
            }
        }
    }
}

static void
parser_text_data (GMarkupParseContext *context, const gchar *text, gsize text_len,
                  gpointer user_data, GError **error)
{
  RmgJEntry *entry = (RmgJEntry *)user_data;
  g_autofree gchar *buffer = g_new0 (gchar, text_len + 1);

  RMG_UNUSED (context);
  RMG_UNUSED (error);

  memcpy (buffer, text, text_len);

  if (g_strcmp0 (entry->parser_current_element, "service") == 0)
    rmg_jentry_set_name (entry, buffer);
  else if (g_strcmp0 (entry->parser_current_element, "privatedata") == 0)
    rmg_jentry_set_private_data_path (entry, buffer);
  else if (g_strcmp0 (entry->parser_current_element, "publicdata") == 0)
    rmg_jentry_set_public_data_path (entry, buffer);
  else if (g_strcmp0 (entry->parser_current_element, "friend") == 0)
    {
      if (entry->parser_current_friend.friend_context == NULL)
        entry->parser_current_friend.friend_context = g_strdup (g_get_host_name ());

      rmg_jentry_add_friend (entry, buffer, /* friend_name */
                             entry->parser_current_friend.friend_context,
                             entry->parser_current_friend.type, entry->parser_current_friend.action,
                             entry->parser_current_friend.argument,
                             entry->parser_current_friend.delay);

      g_free (entry->parser_current_friend.friend_context);
    }
}

static void
add_action_for_service (gpointer _action, gpointer _helper)
{
  RmgAEntry *action = (RmgAEntry *)_action;
  JournalAddAction *helper = (JournalAddAction *)_helper;

  g_autoptr (GError) error = NULL;

  g_info ("Adding action='%s' for service='%s'", g_action_name[action->type],
          helper->service->name);

  if (rmg_journal_add_action (helper->journal, action->hash, helper->service->name, action->type,
                              action->trigger_level_min, action->trigger_level_max,
                              action->reset_after, &error)
      != RMG_STATUS_OK)
    {
      g_warning ("Fail to add action type %u for service %s. Error %s", action->type,
                 helper->service->name, error->message);
    }
}

static void
add_friend_for_service (gpointer _friend, gpointer _helper)
{
  RmgFEntry *friend = (RmgFEntry *)_friend;
  JournalAddFriend *helper = (JournalAddFriend *)_helper;

  g_autoptr (GError) error = NULL;

  g_info ("Adding friend='%s' in context='%s' for service='%s'", friend->friend_name,
          friend->friend_context, helper->service->name);

  if (rmg_journal_add_friend (helper->journal, friend->hash, helper->service->name,
                              friend->friend_name, friend->friend_context, friend->type,
                              friend->action, friend->argument, friend->delay, &error)
      != RMG_STATUS_OK)
    {
      g_warning ("Fail to add friend %s for service %s. Error %s", friend->friend_name,
                 helper->service->name, error->message);
    }
}

RmgStatus
rmg_journal_reload_units (RmgJournal *journal, GError **error)
{
  g_autofree gchar *opt_unitsdir = NULL;
  const gchar *nfile = NULL;
  GDir *gdir = NULL;

  g_assert (journal);

  opt_unitsdir = rmg_options_string_for (journal->options, KEY_UNITS_DIR);

  gdir = g_dir_open (opt_unitsdir, 0, error);
  if (gdir == NULL)
    return RMG_STATUS_ERROR;

  while ((nfile = g_dir_read_name (gdir)) != NULL)
    {
      g_autoptr (RmgJEntry) jentry = NULL;
      g_autoptr (GMarkupParseContext) parser_context = NULL;
      g_autoptr (GError) element_error = NULL;
      g_autofree gchar *fpath = NULL;
      g_autofree gchar *fdata = NULL;
      gulong hash;

      fpath = g_build_filename (opt_unitsdir, nfile, NULL);
      if (!g_file_get_contents (fpath, &fdata, NULL, NULL))
        {
          g_warning ("Fail to read unit %s", nfile);
          continue;
        }

      hash = (gulong)rmg_utils_jenkins_hash (fdata);
      jentry = rmg_jentry_new (hash);
      parser_context = g_markup_parse_context_new (&markup_parser, 0, jentry, NULL);

      if (!g_markup_parse_context_parse (parser_context, fdata, (gssize)strlen (fdata),
                                         &element_error))
        {
          g_warning ("Parser failed for unit %s. Error %s", nfile, element_error->message);
        }
      else
        {
          gulong exist_hash = (gulong)rmg_journal_get_hash (journal, jentry->name, NULL);

          if (hash == exist_hash)
            {
              g_debug ("Service %s parsed and version already in database", jentry->name);
              continue;
            }

          if (rmg_journal_remove_service (journal, jentry->name, &element_error) != RMG_STATUS_OK)
            {
              g_warning ("Fail to remove existent service entry %s", jentry->name);
              continue;
            }

          g_info ("Adding service='%s' as new entry in database", jentry->name);

          if (rmg_journal_add_service (journal, hash, jentry->name, jentry->private_data,
                                       jentry->public_data, jentry->check_start, jentry->timeout,
                                       &element_error)
              == RMG_STATUS_OK)
            {
              JournalAddAction add_action_helper = { .journal = journal, .service = jentry };
              JournalAddFriend add_friend_helper = { .journal = journal, .service = jentry };

              g_list_foreach (jentry->actions, add_action_for_service, &add_action_helper);
              g_list_foreach (jentry->friends, add_friend_for_service, &add_friend_helper);
            }
          else
            {
              g_warning ("Fail to add service entry for unit %s. Error %s", nfile,
                         element_error->message);
            }
        }
    }

  g_dir_close (gdir);

  return RMG_STATUS_OK;
}

RmgStatus
rmg_journal_add_service (RmgJournal *journal, gulong hash, const gchar *service_name,
                         const gchar *private_data, const gchar *public_data, gboolean check_start,
                         glong timeout, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;

  JournalQueryData data
      = { .type = QUERY_ADD_SERVICE, .journal = journal, .callback = NULL, .response = NULL };

  g_assert (journal);
  g_assert (service_name);
  g_assert (private_data);
  g_assert (public_data);

  sql = g_strdup_printf ("INSERT INTO %s                                 "
                         "(HASH,NAME,PRIVDATA,PUBLDATA,RVECTOR,CHKSTART,TIMEOUT)  "
                         "VALUES(%ld, '%s', '%s', '%s', %ld, %ld, %ld);       ",
                         rmg_table_services, (glong)hash, service_name, private_data, public_data,
                         (glong)0, (glong)check_start, timeout);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalAddService"), 1, "SQL query error");
      g_warning ("Fail to add new service entry. SQL error %s", query_error);
      sqlite3_free (query_error);

      return RMG_STATUS_ERROR;
    }

  return RMG_STATUS_OK;
}

RmgStatus
rmg_journal_add_action (RmgJournal *journal, gulong hash, const gchar *service_name,
                        RmgActionType action_type, glong trigger_level_min, glong trigger_level_max,
                        gboolean reset_after, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;

  JournalQueryData data
      = { .type = QUERY_ADD_ACTION, .journal = journal, .callback = NULL, .response = NULL };

  g_assert (journal);
  g_assert (service_name);

  sql = g_strdup_printf ("INSERT INTO %s                    "
                         "(HASH,SERVICE,TYPE,TLMIN,TLMAX,RESET)   "
                         "VALUES(%ld, '%s', %u, %ld, %ld, %d); ",
                         rmg_table_actions, (glong)hash, service_name, action_type,
                         trigger_level_min, trigger_level_max, reset_after);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalAddAction"), 1, "SQL query error");
      g_warning ("Fail to add new action entry. SQL error %s", query_error);
      sqlite3_free (query_error);

      return RMG_STATUS_ERROR;
    }

  return RMG_STATUS_OK;
}

RmgStatus
rmg_journal_add_friend (RmgJournal *journal, gulong hash, const gchar *service_name,
                        const gchar *friend_name, const gchar *friend_context,
                        RmgFriendType friend_type, RmgFriendActionType friend_action,
                        glong friend_argument, glong friend_delay, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;

  JournalQueryData data
      = { .type = QUERY_ADD_FRIEND, .journal = journal, .callback = NULL, .response = NULL };

  g_assert (journal);
  g_assert (service_name);
  g_assert (friend_name);
  g_assert (friend_context);

  sql = g_strdup_printf ("INSERT INTO %s                    "
                         "(HASH,SERVICE,FRIEND,CONTEXT,TYPE,ACTION,ARGUMENT,DELAY)   "
                         "VALUES(%ld, '%s', '%s', '%s', %u, %u, %ld, %ld); ",
                         rmg_table_friends, (glong)hash, service_name, friend_name, friend_context,
                         friend_type, friend_action, friend_argument, friend_delay);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalAddFriend"), 1, "SQL query error");
      g_warning ("Fail to add new friend entry. SQL error %s", query_error);
      sqlite3_free (query_error);

      return RMG_STATUS_ERROR;
    }

  return RMG_STATUS_OK;
}

gchar *
rmg_journal_get_private_data_path (RmgJournal *journal, const gchar *service_name, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;

  JournalQueryData data
      = { .type = QUERY_GET_PRIVATE_DATA, .journal = journal, .callback = NULL, .response = NULL };

  g_assert (journal);
  g_assert (service_name);

  sql = g_strdup_printf ("SELECT PRIVDATA FROM %s WHERE NAME IS '%s'", rmg_table_services,
                         service_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalGetPrivateData"), 1,
                   "SQL query error");
      g_warning ("Fail to get private data path. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return (gchar *)data.response;
}

gchar *
rmg_journal_get_public_data_path (RmgJournal *journal, const gchar *service_name, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;

  JournalQueryData data
      = { .type = QUERY_GET_PUBLIC_DATA, .journal = journal, .callback = NULL, .response = NULL };

  g_assert (journal);
  g_assert (service_name);

  sql = g_strdup_printf ("SELECT PUBLDATA FROM %s WHERE NAME IS '%s'", rmg_table_services,
                         service_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalGetPublicData"), 1,
                   "SQL query error");
      g_warning ("Fail to get public data path. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return (gchar *)data.response;
}

gboolean
rmg_journal_get_checkstart (RmgJournal *journal, const gchar *service_name, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  gboolean check_start = FALSE;

  JournalQueryData data
      = { .type = QUERY_GET_CHECK_START, .journal = journal, .callback = NULL, .response = NULL };

  g_assert (journal);
  g_assert (service_name);

  data.response = (gpointer)&check_start;

  sql = g_strdup_printf ("SELECT CHKSTART FROM %s WHERE NAME IS '%s'", rmg_table_services,
                         service_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalGetCheckStart"), 1,
                   "SQL query error");
      g_warning ("Fail to get check start flag. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return check_start;
}

glong
rmg_journal_get_relaxing_timeout (RmgJournal *journal, const gchar *service_name, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  glong timeout = 0;

  JournalQueryData data
      = { .type = QUERY_GET_TIMEOUT, .journal = journal, .callback = NULL, .response = NULL };

  g_assert (journal);
  g_assert (service_name);

  data.response = (gpointer)&timeout;

  sql = g_strdup_printf ("SELECT TIMEOUT FROM %s WHERE NAME IS '%s'", rmg_table_services,
                         service_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalGetRelaxingTimeout"), 1,
                   "SQL query error");
      g_warning ("Fail to get relaxing timeout. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return timeout;
}

void
rmg_journal_call_foreach_relaxing (RmgJournal *journal, RmgJournalCallback callback, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;

  JournalQueryData data
      = { .type = QUERY_CALL_RELAXING, .journal = journal, .callback = callback, .response = NULL };

  g_assert (journal);

  sql = g_strdup_printf ("SELECT NAME FROM %s WHERE RVECTOR > 0", rmg_table_services);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalCallRelaxing"), 1, "SQL query error");
      g_warning ("Fail to get relaxing timeout. SQL error %s", query_error);
      sqlite3_free (query_error);
    }
}

void
rmg_journal_call_foreach_checkstart (RmgJournal *journal, RmgJournalCallback callback,
                                     GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;

  JournalQueryData data = {
    .type = QUERY_CALL_CHECK_START, .journal = journal, .callback = callback, .response = NULL
  };

  g_assert (journal);

  sql = g_strdup_printf ("SELECT NAME FROM %s WHERE CHKSTART > 0", rmg_table_services);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalCallCheckStart"), 1,
                   "SQL query error");
      g_warning ("Fail to get call check start. SQL error %s", query_error);
      sqlite3_free (query_error);
    }
}

glong
rmg_journal_get_rvector (RmgJournal *journal, const gchar *service_name, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  glong rvector = 0;

  JournalQueryData data
      = { .type = QUERY_GET_RVECTOR, .journal = journal, .callback = NULL, .response = NULL };

  g_assert (journal);
  g_assert (service_name);

  data.response = (gpointer)&rvector;

  sql = g_strdup_printf ("SELECT RVECTOR FROM %s WHERE NAME IS '%s'", rmg_table_services,
                         service_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalGetGradiant"), 1, "SQL query error");
      g_warning ("Fail to get rvector. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return rvector;
}

RmgStatus
rmg_journal_set_rvector (RmgJournal *journal, const gchar *service_name, glong rvector,
                         GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  RmgStatus status = RMG_STATUS_OK;

  JournalQueryData data
      = { .type = QUERY_SET_RVECTOR, .journal = journal, .callback = NULL, .response = NULL };

  g_assert (journal);
  g_assert (service_name);

  sql = g_strdup_printf ("UPDATE %s SET RVECTOR = %ld WHERE NAME IS '%s'", rmg_table_services,
                         rvector, service_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalSetRelaxingState"), 1,
                   "SQL query error");
      g_warning ("Fail to set rvector. SQL error %s", query_error);
      sqlite3_free (query_error);
      status = RMG_STATUS_ERROR;
    }

  return status;
}

RmgActionType
rmg_journal_get_service_action (RmgJournal *journal, const gchar *service_name, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  RmgActionType action_type = ACTION_INVALID;
  glong rvector = 0;

  JournalQueryData data
      = { .type = QUERY_GET_ACTION, .journal = journal, .callback = NULL, .response = NULL };

  g_assert (journal);
  g_assert (service_name);

  rvector = rmg_journal_get_rvector (journal, service_name, error);
  data.response = (gpointer)&action_type;

  sql = g_strdup_printf ("SELECT TYPE FROM %s WHERE SERVICE IS '%s' "
                         "AND %ld BETWEEN TLMIN AND TLMAX",
                         rmg_table_actions, service_name, rvector);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalGetServiceAction"), 1,
                   "SQL query error");
      g_warning ("Fail to get service action. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return action_type;
}

gboolean
rmg_journal_get_service_action_reset_after (RmgJournal *journal, const gchar *service_name,
                                            GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  gboolean reset_after = FALSE;
  glong rvector = 0;

  JournalQueryData data = {
    .type = QUERY_GET_ACTION_RESET_AFTER, .journal = journal, .callback = NULL, .response = NULL
  };

  g_assert (journal);
  g_assert (service_name);

  rvector = rmg_journal_get_rvector (journal, service_name, error);
  data.response = (gpointer)&reset_after;

  sql = g_strdup_printf ("SELECT RESET FROM %s WHERE SERVICE IS '%s' "
                         "AND %ld BETWEEN TLMIN AND TLMAX",
                         rmg_table_actions, service_name, rvector);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalGetServiceActionResetAfter"), 1,
                   "SQL query error");
      g_warning ("Fail to get service action reset after. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return reset_after;
}

GList *
rmg_journal_get_services_for_friend (RmgJournal *journal, const gchar *friend_name,
                                     const gchar *friend_context, RmgFriendType friend_type,
                                     GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  GList *services = NULL;

  JournalQueryData data = {
    .type = QUERY_GET_SERVICES_FOR_FRIEND, .journal = journal, .callback = NULL, .response = NULL
  };

  g_assert (journal);
  g_assert (friend_name);
  g_assert (friend_context);

  data.response = (gpointer)&services;

  sql = g_strdup_printf ("SELECT * FROM %s WHERE FRIEND IS '%s' "
                         "AND CONTEXT IS '%s' AND TYPE IS %u",
                         rmg_table_friends, friend_name, friend_context, friend_type);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalGetServicesForFriend"), 1,
                   "SQL query error");
      g_warning ("Fail to get services for friend. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return services;
}

RmgStatus
rmg_journal_remove_service (RmgJournal *journal, const gchar *service_name, GError **error)
{
  g_autofree gchar *service_sql = NULL;
  gchar *query_error = NULL;
  RmgStatus status = RMG_STATUS_OK;

  JournalQueryData data
      = { .type = QUERY_REMOVE_SERVICE, .journal = journal, .callback = NULL, .response = NULL };

  g_assert (journal);
  g_assert (service_name);

  service_sql
      = g_strdup_printf ("DELETE FROM %s WHERE NAME IS '%s'", rmg_table_services, service_name);

  if (sqlite3_exec (journal->database, service_sql, sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalRemoveService"), 1,
                   "SQL query error");
      g_warning ("Fail to remove service entry. SQL error %s", query_error);
      sqlite3_free (query_error);
      status = RMG_STATUS_ERROR;
    }
  else
    {
      g_autofree gchar *actions_sql = NULL;
      g_autofree gchar *friends_sql = NULL;

      actions_sql = g_strdup_printf ("DELETE FROM %s WHERE SERVICE IS '%s'", rmg_table_actions,
                                     service_name);

      if (sqlite3_exec (journal->database, actions_sql, sqlite_callback, &data, &query_error)
          != SQLITE_OK)
        {
          g_set_error (error, g_quark_from_static_string ("JournalRemoveService"), 1,
                       "SQL query error");
          g_warning ("Fail to remove actions. SQL error %s", query_error);
          sqlite3_free (query_error);
          status = RMG_STATUS_ERROR;
        }

      friends_sql = g_strdup_printf ("DELETE FROM %s WHERE SERVICE IS '%s'", rmg_table_friends,
                                     service_name);

      if (sqlite3_exec (journal->database, friends_sql, sqlite_callback, &data, &query_error)
          != SQLITE_OK)
        {
          g_set_error (error, g_quark_from_static_string ("JournalRemoveService"), 1,
                       "SQL query error");
          g_warning ("Fail to remove friends. SQL error %s", query_error);
          sqlite3_free (query_error);
          status = RMG_STATUS_ERROR;
        }
    }

  return status;
}

gulong
rmg_journal_get_hash (RmgJournal *journal, const gchar *service_name, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  gulong hash = 0;

  JournalQueryData data
      = { .type = QUERY_GET_HASH, .journal = journal, .callback = NULL, .response = NULL };

  g_assert (journal);
  g_assert (service_name);

  data.response = (gpointer)&hash;

  sql = g_strdup_printf ("SELECT HASH FROM %s WHERE NAME IS '%s'", rmg_table_services,
                         service_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalGetRelaxingTimeout"), 1,
                   "SQL query error");
      g_warning ("Fail to get entry count. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return hash;
}
