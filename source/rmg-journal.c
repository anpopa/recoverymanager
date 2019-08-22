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
#include "rmg-sentry.h"
#include "rmg-defaults.h"
#include "rmg-utils.h"

/**
 * @enum Journal query type
 */
typedef enum _JournalQueryType {
  QUERY_CREATE,
} JournalQueryType;

/**
 * @enum Journal query data object
 */
typedef struct _JournalQueryData {
  JournalQueryType type;
  gpointer response;
} JournalQueryData;

const gchar *rmg_table_services = "Services";
const gchar *rmg_table_actions = "Actions";

/**
 * @brief SQlite3 callback
 */
static int sqlite_callback (void *data, int argc, char **argv, char **colname);

static void parser_start_element (GMarkupParseContext *context,
                                  const gchar         *element_name,
                                  const gchar        **attribute_names,
                                  const gchar        **attribute_values,
                                  gpointer user_data,
                                  GError             **error);

static void parser_text_data (GMarkupParseContext *context,
                              const gchar         *text,
                              gsize text_len,
                              gpointer user_data,
                              GError             **error);

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

  RMG_UNUSED (argc);
  RMG_UNUSED (argv);
  RMG_UNUSED (colname);

  switch (querydata->type)
    {
    case QUERY_CREATE:
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
    .type = QUERY_CREATE,
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

      services_sql = g_strdup_printf ("CREATE TABLE IF NOT EXISTS %s      "
                                      "(ID INT PRIMARY KEY    NOT   NULL, "
                                      "NAME            TEXT   NOT   NULL, "
                                      "VERSION         INT    NOT   NULL, "
                                      "PRIVDATA        TEXT   NOT   NULL, "
                                      "PUBLDATA        TEXT   NOT   NULL, "
                                      "GRADIANT        INT    NOT   NULL, "
                                      "RELAXING        BOOL   NOT   NULL, "
                                      "TIMEOUT         INT    NOT   NULL);",
                                      rmg_table_services);

      if (sqlite3_exec (journal->database, services_sql, sqlite_callback, &data, &query_error)
          != SQLITE_OK)
        {
          g_warning ("Fail to create crash table. SQL error %s", query_error);
          g_set_error (error,
                       g_quark_from_static_string ("JournalNew"),
                       1,
                       "Create services table fail");
        }
      else
        {
          g_autofree gchar *actions_sql = NULL;

          actions_sql = g_strdup_printf ("CREATE TABLE IF NOT EXISTS %s      "
                                         "(ID INT PRIMARY KEY    NOT   NULL, "
                                         "SERVICE         INT    NOT   NULL, "
                                         "TYPE            INT    NOT   NULL, "
                                         "LEVEL           INT    NOT   NULL, "
                                         "NEXT            INT    NOT   NULL);",
                                         rmg_table_actions);

          if (sqlite3_exec (journal->database, actions_sql, sqlite_callback, &data, &query_error)
              != SQLITE_OK)
            {
              g_warning ("Fail to create crash table. SQL error %s", query_error);
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
  RmgSEntry *entry = (RmgSEntry *)user_data;

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
          glong g = rmg_sentry_get_gradiant (entry) + retry;
          rmg_sentry_set_gradiant (entry, g);
          rmg_sentry_add_action (entry, action_type, g);
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
  RmgSEntry *entry = (RmgSEntry *)user_data;
  g_autofree gchar *buffer = g_new0 (gchar, text_len + 1);

  RMG_UNUSED (context);
  RMG_UNUSED (error);

  memcpy (buffer, text, text_len);

  if (g_strcmp0 (entry->parser_current_element, "service") == 0)
    rmg_sentry_set_name (entry, buffer);
  else if (g_strcmp0 (entry->parser_current_element, "relaxtime") == 0)
    rmg_sentry_set_timeout (entry, g_ascii_strtoll (buffer, NULL, 10));
  else if (g_strcmp0 (entry->parser_current_element, "privatedata") == 0)
    rmg_sentry_set_private_data_path (entry, buffer);
  else if (g_strcmp0 (entry->parser_current_element, "publicdata") == 0)
    rmg_sentry_set_public_data_path (entry, buffer);
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
      g_autoptr (RmgSEntry) service_entry = NULL;
      g_autoptr (GMarkupParseContext) parser_context = NULL;
      g_autoptr (GError) element_error = NULL;
      g_autofree gchar *fpath = NULL;
      g_autofree gchar *fdata = NULL;

      fpath = g_build_filename (opt_unitsdir, nfile, NULL);
      if (!g_file_get_contents (fpath, &fdata, NULL, NULL))
        {
          g_warning ("Fail to read unit %s", nfile);
          continue;
        }

      service_entry = rmg_sentry_new (rmg_utils_jenkins_hash (edata));
      parser_context = g_markup_parse_context_new (&markup_parser, 0, service_entry, NULL);

      if (!g_markup_parse_context_parse (parser_context, fdata, (gssize)strlen (fdata), &element_error))
        {
          g_warning ("Parser failed for unit %s. Error %s", nfile, element_error->message);
        }
    }

  g_dir_close (gdir);

  return RMG_STATUS_OK;
}

