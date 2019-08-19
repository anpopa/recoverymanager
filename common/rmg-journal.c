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

const gchar *rmg_journal_table_name = "CrashTable";

/**
 * @brief SQlite3 callback
 */
static int sqlite_callback (void *data, int argc, char **argv, char **colname);

static int
sqlite_callback (void *data, int argc, char **argv, char **colname)
{
  JournalQueryData *querydata = (JournalQueryData *)data;

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
  g_autofree gchar *sql = NULL;
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

  opt_dbdir = rmg_options_string_for (options, KEY_DATABASE_DIR);
  dbfile = g_build_filename (opt_dbdir, MG_DATABASE_FILE_NAME, NULL);

  if (sqlite3_open (dbfile, &journal->database))
    {
      g_warning ("Cannot open journal database at path %s", dbfile);
      g_set_error (error, g_quark_from_static_string ("JournalNew"), 1, "Database open failed");
    }
  else
    {
      sql = g_strdup_printf ("CREATE TABLE IF NOT EXISTS %s       "
                             "(ID INT PRIMARY KEY     NOT   NULL, "
                             "PROCNAME        TEXT    NOT   NULL, "
                             "CRASHID         TEXT    NOT   NULL, "
                             "VECTORID        TEXT    NOT   NULL, "
                             "CONTEXTID       TEXT    NOT   NULL, "
                             "FILEPATH        TEXT    NOT   NULL, "
                             "FILESIZE        INT     NOT   NULL, "
                             "PID             INT     NOT   NULL, "
                             "SIGNAL          INT     NOT   NULL, "
                             "TIMESTAMP       INT     NOT   NULL, "
                             "OSVERSION       TEXT    NOT   NULL, "
                             "TSTATE          BOOL    NOT   NULL, "
                             "RSTATE          BOOL    NOT   NULL);",
                             rmg_journal_table_name);

      if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
        {
          g_warning ("Fail to create crash table. SQL error %s", query_error);
          g_set_error (error, g_quark_from_static_string ("JournalNew"), 1, "Create crash table fail");
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
    g_free (journal);
}
