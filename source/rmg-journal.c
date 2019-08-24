/* rmg-journal.c
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

#include "rmg-journal.h"
#include "rmg-jentry.h"
#include "rmg-defaults.h"
#include "rmg-utils.h"

/**
 * @enum Journal query type
 */
typedef enum _JournalQueryType {
  QUERY_CREATE_TABLES,
  QUERY_GET_HASH,
  QUERY_ADD_SERVICE,
  QUERY_REMOVE_SERVICE,
  QUERY_ADD_ACTION,
  QUERY_GET_PRIVATE_DATA,
  QUERY_GET_PUBLIC_DATA,
  QUERY_GET_RELAXING,
  QUERY_SET_RELAXING,
  QUERY_GET_TIMEOUT,
  QUERY_GET_RVECTOR,
  QUERY_SET_RVECTOR,
  QUERY_GET_ACTION,
} JournalQueryType;

/**
 * @enum Journal query data object
 */
typedef struct _JournalQueryData {
  JournalQueryType type;
  gpointer response;
} JournalQueryData;

/**
 * @struct Add action object helper
 */
typedef struct _JournalAddAction {
  RmgJournal *journal;
  RmgJEntry  *service;
} JournalAddAction;

const gchar *rmg_table_services = "Services";
const gchar *rmg_table_actions = "Actions";

/**
 * @brief SQlite3 callback
 */
static int sqlite_callback (void *data, int argc, char **argv, char **colname);

/**
 * @brief Parser markup callback for element start
 */
static void parser_start_element (GMarkupParseContext *context,
                                  const gchar         *element_name,
                                  const gchar        **attribute_names,
                                  const gchar        **attribute_values,
                                  gpointer user_data,
                                  GError             **error);

/**
 * @brief Parser markup callback for element text data
 */
static void parser_text_data (GMarkupParseContext *context,
                              const gchar         *text,
                              gsize text_len,
                              gpointer user_data,
                              GError             **error);

/**
 * @brief Markup parser definition
 */
static GMarkupParser markup_parser =
{
  parser_start_element,
  NULL,
  parser_text_data,
  NULL,
  NULL
};

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
            *((gulong *)(querydata->response)) = g_ascii_strtoull (argv[i], NULL, 10);
        }
      break;

    case QUERY_ADD_SERVICE:
      break;

    case QUERY_REMOVE_SERVICE:
      break;

    case QUERY_ADD_ACTION:
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

    case QUERY_GET_RELAXING:
      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "RELAXING") == 0)
            *((gboolean *)(querydata->response)) = (gboolean)g_ascii_strtoll (argv[i], NULL, 10);
        }
      break;

    case QUERY_SET_RELAXING:
      break;

    case QUERY_GET_TIMEOUT:
      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "TIMEOUT") == 0)
            *((glong *)(querydata->response)) = g_ascii_strtoll (argv[i], NULL, 10);
        }
      break;

    case QUERY_GET_RVECTOR:
      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "RVECTOR") == 0)
            *((glong *)(querydata->response)) = g_ascii_strtoll (argv[i], NULL, 10);
        }
      break;

    case QUERY_SET_RVECTOR:
      break;

    case QUERY_GET_ACTION:
      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "TYPE") == 0)
            *((RmgActionType *)(querydata->response)) = (RmgActionType)g_ascii_strtoll (argv[i], NULL, 10);
        }
      break;

    default:
      break;
    }

  return(0);
}

RmgJournal *
rmg_journal_new (RmgOptions *options, GError **error)
{
  RmgJournal *journal = NULL;
  g_autofree gchar *opt_dbdir = NULL;
  g_autofree gchar *dbfile = NULL;
  gchar *query_error = NULL;
  JournalQueryData data = {
    .type = QUERY_CREATE_TABLES,
    .response = NULL
  };

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
                                      " RELAXING  BOOL            NOT NULL, "
                                      " TIMEOUT   NUMERIC         NOT NULL);",
                                      rmg_table_services);

      if (sqlite3_exec (journal->database, services_sql, sqlite_callback, &data, &query_error)
          != SQLITE_OK)
        {
          g_warning ("Fail to create services table. SQL error %s", query_error);
          g_set_error (error,
                       g_quark_from_static_string ("JournalNew"),
                       1,
                       "Create services table fail");
        }
      else
        {
          g_autofree gchar *actions_sql = NULL;

          actions_sql = g_strdup_printf ("CREATE TABLE IF NOT EXISTS %s        "
                                         "(HASH     UNSIGNED INTEGER PRIMARY KEY NOT NULL, "
                                         " SERVICE  TEXT                NOT NULL, "
                                         " TYPE     NUMERIC    NOT   NULL, "
                                         " TLMIN    NUMERIC    NOT   NULL, "
                                         " TLMAX    NUMERIC    NOT   NULL);",
                                         rmg_table_actions);

          if (sqlite3_exec (journal->database, actions_sql, sqlite_callback, &data, &query_error)
              != SQLITE_OK)
            {
              g_warning ("Fail to create actions table. SQL error %s", query_error);
              g_set_error (error,
                           g_quark_from_static_string ("JournalNew"),
                           1,
                           "Create actions table fail");
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
parser_start_element (GMarkupParseContext *context,
                      const gchar         *element_name,
                      const gchar        **attribute_names,
                      const gchar        **attribute_values,
                      gpointer user_data,
                      GError             **error)
{
  RmgJEntry *entry = (RmgJEntry *)user_data;

  entry->parser_current_element = element_name;

  RMG_UNUSED (context);
  RMG_UNUSED (error);

  if (g_strcmp0 (element_name, "action") == 0)
    {
      RmgActionType action_type = ACTION_INVALID;
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
                retry = g_ascii_strtoll (attribute_values[i], NULL, 10);
            }
        }

      if (retry < 1 || action_type == ACTION_INVALID)
        {
          g_warning ("Invalid action settings");
        }
      else
        {
          glong g = rmg_jentry_get_rvector (entry) + retry;

          rmg_jentry_add_action (entry, action_type, rmg_jentry_get_rvector (entry), g);
          rmg_jentry_set_rvector (entry, g);
        }
    }
}

static void
parser_text_data (GMarkupParseContext *context,
                  const gchar         *text,
                  gsize text_len,
                  gpointer user_data,
                  GError             **error)
{
  RmgJEntry *entry = (RmgJEntry *)user_data;
  g_autofree gchar *buffer = g_new0 (gchar, text_len + 1);

  RMG_UNUSED (context);
  RMG_UNUSED (error);

  memcpy (buffer, text, text_len);

  if (g_strcmp0 (entry->parser_current_element, "service") == 0)
    rmg_jentry_set_name (entry, buffer);
  else if (g_strcmp0 (entry->parser_current_element, "relaxtime") == 0)
    rmg_jentry_set_timeout (entry, g_ascii_strtoll (buffer, NULL, 10));
  else if (g_strcmp0 (entry->parser_current_element, "privatedata") == 0)
    rmg_jentry_set_private_data_path (entry, buffer);
  else if (g_strcmp0 (entry->parser_current_element, "publicdata") == 0)
    rmg_jentry_set_public_data_path (entry, buffer);
}

static void
add_action_for_service (gpointer _action, gpointer _helper)
{
  RmgAEntry *action = (RmgAEntry *)_action;
  JournalAddAction *helper = (JournalAddAction *)_helper;

  g_autoptr (GError) error = NULL;

  g_info ("Adding action='%s' for service='%s'",
          g_action_name[action->type],
          helper->service->name);

  if (rmg_journal_add_action (helper->journal,
                              action->hash,
                              helper->service->name,
                              action->type,
                              action->trigger_level_min,
                              action->trigger_level_max,
                              &error)
      != RMG_STATUS_OK)
    {
      g_warning ("Fail to add action type %u for service %s. Error %s",
                 action->type,
                 helper->service->name,
                 error->message);
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

      hash = rmg_utils_jenkins_hash (fdata);
      jentry = rmg_jentry_new (hash);
      parser_context = g_markup_parse_context_new (&markup_parser, 0, jentry, NULL);

      if (!g_markup_parse_context_parse (parser_context, fdata, (gssize)strlen (fdata), &element_error))
        {
          g_warning ("Parser failed for unit %s. Error %s", nfile, element_error->message);
        }
      else
        {
          gulong exist_hash = rmg_journal_get_hash (journal, jentry->name, NULL);

          if (hash == exist_hash)
            {
              g_debug ("Service %s parsed and version already in database", jentry->name);
              continue;
            }

          if (rmg_journal_remove_service (journal,
                                          jentry->name,
                                          &element_error)
              != RMG_STATUS_OK)
            {
              g_warning ("Fail to remove existent service entry %s", jentry->name);
              continue;
            }

          g_info ("Adding service='%s' as new entry in database", jentry->name);

          if (rmg_journal_add_service (journal,
                                       hash,
                                       jentry->name,
                                       jentry->private_data,
                                       jentry->public_data,
                                       jentry->timeout,
                                       &element_error)
              == RMG_STATUS_OK)
            {
              JournalAddAction addhelper = {
                .journal = journal,
                .service = jentry
              };

              g_list_foreach (jentry->actions, add_action_for_service, &addhelper);
            }
          else
            {
              g_warning ("Fail to add service entry for unit %s. Error %s",
                         nfile,
                         element_error->message);
            }
        }
    }

  g_dir_close (gdir);

  return RMG_STATUS_OK;
}

RmgStatus
rmg_journal_add_service (RmgJournal *journal,
                         gulong hash,
                         const gchar *service_name,
                         const gchar *private_data,
                         const gchar *public_data,
                         glong timeout,
                         GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  JournalQueryData data = {
    .type = QUERY_ADD_SERVICE,
    .response = NULL
  };

  g_assert (journal);
  g_assert (service_name);
  g_assert (private_data);
  g_assert (public_data);

  sql = g_strdup_printf ("INSERT INTO %s                                          "
                         "(HASH,NAME,PRIVDATA,PUBLDATA,RVECTOR,RELAXING,TIMEOUT)  "
                         "VALUES(%ld, '%s', '%s', '%s', %ld, %d, %ld);           ",
                         rmg_table_services,
                         (glong)hash,
                         service_name,
                         private_data,
                         public_data,
                         (glong)0,
                         0,
                         timeout);

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
rmg_journal_add_action (RmgJournal *journal,
                        gulong hash,
                        const gchar *service_name,
                        RmgActionType action_type,
                        glong trigger_level_min,
                        glong trigger_level_max,
                        GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  JournalQueryData data = {
    .type = QUERY_ADD_ACTION,
    .response = NULL
  };

  g_assert (journal);
  g_assert (service_name);

  sql = g_strdup_printf ("INSERT INTO %s                    "
                         "(HASH,SERVICE,TYPE,TLMIN,TLMAX)   "
                         "VALUES(%ld, '%s', %u, %ld, %ld); ",
                         rmg_table_actions,
                         (glong)hash,
                         service_name,
                         action_type,
                         trigger_level_min,
                         trigger_level_max);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalAddAction"), 1, "SQL query error");
      g_warning ("Fail to add new action entry. SQL error %s", query_error);
      sqlite3_free (query_error);

      return RMG_STATUS_ERROR;
    }

  return RMG_STATUS_OK;
}

gchar *
rmg_journal_get_private_data_path (RmgJournal *journal,
                                   const gchar *service_name,
                                   GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  JournalQueryData data = {
    .type = QUERY_GET_PRIVATE_DATA,
    .response = NULL
  };

  g_assert (journal);
  g_assert (service_name);

  sql = g_strdup_printf ("SELECT PRIVDATA FROM %s WHERE NAME IS '%s'",
                         rmg_table_services, service_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_set_error (error,
                   g_quark_from_static_string ("JournalGetPrivateData"),
                   1,
                   "SQL query error");
      g_warning ("Fail to get private data path. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return (gchar *)data.response;
}

gchar *
rmg_journal_get_public_data_path (RmgJournal *journal,
                                  const gchar *service_name,
                                  GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  JournalQueryData data = {
    .type = QUERY_GET_PUBLIC_DATA,
    .response = NULL
  };

  g_assert (journal);
  g_assert (service_name);

  sql = g_strdup_printf ("SELECT PUBLDATA FROM %s WHERE NAME IS '%s'",
                         rmg_table_services, service_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_set_error (error,
                   g_quark_from_static_string ("JournalGetPublicData"),
                   1,
                   "SQL query error");
      g_warning ("Fail to get public data path. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return (gchar *)data.response;
}

gboolean
rmg_journal_get_relaxing_state (RmgJournal *journal,
                                const gchar *service_name,
                                GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  gboolean state = FALSE;
  JournalQueryData data = {
    .type = QUERY_GET_RELAXING,
    .response = NULL
  };

  g_assert (journal);
  g_assert (service_name);

  data.response = (gpointer) & state;

  sql = g_strdup_printf ("SELECT RELAXING FROM %s WHERE NAME IS '%s'",
                         rmg_table_services, service_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_set_error (error,
                   g_quark_from_static_string ("JournalGetRelaxingState"),
                   1,
                   "SQL query error");
      g_warning ("Fail to get relaxing state. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return state;
}

RmgStatus
rmg_journal_set_relaxing_state (RmgJournal *journal,
                                const gchar *service_name,
                                gboolean relaxing_status,
                                GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  RmgStatus status = RMG_STATUS_OK;
  JournalQueryData data = {
    .type = QUERY_GET_RELAXING,
    .response = NULL
  };

  g_assert (journal);
  g_assert (service_name);

  sql = g_strdup_printf ("UPDATE %s SET RELAXING = %d WHERE NAME IS '%s'",
                         rmg_table_services, relaxing_status, service_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_set_error (error,
                   g_quark_from_static_string ("JournalSetRelaxingState"),
                   1,
                   "SQL query error");
      g_warning ("Fail to set relaxing state. SQL error %s", query_error);
      sqlite3_free (query_error);
      status = RMG_STATUS_ERROR;
    }

  return status;
}

glong
rmg_journal_get_relaxing_timeout (RmgJournal *journal,
                                  const gchar *service_name,
                                  GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  glong timeout = 0;
  JournalQueryData data = {
    .type = QUERY_GET_RELAXING,
    .response = NULL
  };

  g_assert (journal);
  g_assert (service_name);

  data.response = (gpointer) & timeout;

  sql = g_strdup_printf ("SELECT TIMEOUT FROM %s WHERE NAME IS '%s'",
                         rmg_table_services, service_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_set_error (error,
                   g_quark_from_static_string ("JournalGetRelaxingTimeout"),
                   1,
                   "SQL query error");
      g_warning ("Fail to get relaxing timeout. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return timeout;
}

glong
rmg_journal_get_rvector (RmgJournal *journal,
                         const gchar *service_name,
                         GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  glong rvector = 0;
  JournalQueryData data = {
    .type = QUERY_GET_RVECTOR,
    .response = NULL
  };

  g_assert (journal);
  g_assert (service_name);

  data.response = (gpointer) & rvector;

  sql = g_strdup_printf ("SELECT RVECTOR FROM %s WHERE NAME IS '%s'",
                         rmg_table_services, service_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_set_error (error,
                   g_quark_from_static_string ("JournalGetGradiant"),
                   1,
                   "SQL query error");
      g_warning ("Fail to get rvector. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return rvector;
}

RmgStatus
rmg_journal_set_rvector (RmgJournal *journal,
                         const gchar *service_name,
                         glong rvector,
                         GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  RmgStatus status = RMG_STATUS_OK;
  JournalQueryData data = {
    .type = QUERY_SET_RVECTOR,
    .response = NULL
  };

  g_assert (journal);
  g_assert (service_name);

  sql = g_strdup_printf ("UPDATE %s SET RVECTOR = %ld WHERE NAME IS '%s'",
                         rmg_table_services, rvector, service_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_set_error (error,
                   g_quark_from_static_string ("JournalSetRelaxingState"),
                   1,
                   "SQL query error");
      g_warning ("Fail to set rvector. SQL error %s", query_error);
      sqlite3_free (query_error);
      status = RMG_STATUS_ERROR;
    }

  return status;
}

RmgActionType
rmg_journal_get_service_action (RmgJournal *journal,
                                const gchar *service_name,
                                GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  RmgActionType action_type = ACTION_INVALID;
  glong rvector = 0;
  JournalQueryData data = {
    .type = QUERY_GET_ACTION,
    .response = NULL
  };

  g_assert (journal);
  g_assert (service_name);

  rvector = rmg_journal_get_rvector (journal, service_name, error);
  data.response = (gpointer) & action_type;

  sql = g_strdup_printf ("SELECT TYPE FROM %s WHERE SERVICE IS '%s' "
                         "AND %ld BETWEEN TLMIN AND TLMAX",
                         rmg_table_actions, service_name, rvector);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_set_error (error,
                   g_quark_from_static_string ("JournalGetServiceAction"),
                   1,
                   "SQL query error");
      g_warning ("Fail to get service action. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return action_type;
}

RmgStatus
rmg_journal_remove_service (RmgJournal *journal,
                            const gchar *service_name,
                            GError **error)
{
  g_autofree gchar *service_sql = NULL;
  gchar *query_error = NULL;
  RmgStatus status = RMG_STATUS_OK;
  JournalQueryData data = {
    .type = QUERY_REMOVE_SERVICE,
    .response = NULL
  };

  g_assert (journal);
  g_assert (service_name);

  service_sql = g_strdup_printf ("DELETE FROM %s WHERE NAME IS '%s'",
                                 rmg_table_services, service_name);

  if (sqlite3_exec (journal->database, service_sql, sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_set_error (error,
                   g_quark_from_static_string ("JournalRemoveService"),
                   1,
                   "SQL query error");
      g_warning ("Fail to remove service entry. SQL error %s", query_error);
      sqlite3_free (query_error);
      status = RMG_STATUS_ERROR;
    }
  else
    {
      g_autofree gchar *actions_sql = NULL;

      service_sql = g_strdup_printf ("DELETE FROM %s WHERE SERVICE IS '%s'",
                                     rmg_table_actions, service_name);

      if (sqlite3_exec (journal->database, service_sql, sqlite_callback, &data, &query_error)
          != SQLITE_OK)
        {
          g_set_error (error,
                       g_quark_from_static_string ("JournalRemoveService"),
                       1,
                       "SQL query error");
          g_warning ("Fail to remove actions. SQL error %s", query_error);
          sqlite3_free (query_error);
          status = RMG_STATUS_ERROR;
        }
    }

  return status;
}

gulong
rmg_journal_get_hash (RmgJournal *journal,
                      const gchar *service_name,
                      GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  gulong hash = 0;
  JournalQueryData data = {
    .type = QUERY_GET_HASH,
    .response = NULL
  };

  g_assert (journal);
  g_assert (service_name);

  data.response = (gpointer) & hash;

  sql = g_strdup_printf ("SELECT HASH FROM %s WHERE NAME IS '%s'",
                         rmg_table_services, service_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_set_error (error,
                   g_quark_from_static_string ("JournalGetRelaxingTimeout"),
                   1,
                   "SQL query error");
      g_warning ("Fail to get entry count. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return hash;
}
