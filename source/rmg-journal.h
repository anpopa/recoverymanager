/* rmg-journal.h
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

#pragma once

#include "rmg-types.h"
#include "rmg-options.h"

#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

/**
 * @struct RmgJournal
 * @brief The RmgJournal opaque data structure
 */
typedef struct _RmgJournal {
  sqlite3 *database; /**< The sqlite3 database object */
  grefcount rc;     /**< Reference counter variable  */
} RmgJournal;

/**
 * @brief Create a new journal object
 * @param options A pointer to the RmgOptions object created by the main
 * application
 * @param error The GError object or NULL
 * @return On success return a new RmgJournal object otherwise return NULL
 */
RmgJournal *rmg_journal_new (RmgOptions *options, GError **error);

/**
 * @brief Aquire journal object
 * @param journal Pointer to the journal object
 * @return The referenced journal object
 */
RmgJournal *rmg_journal_ref (RmgJournal *journal);

/**
 * @brief Release an journal object
 * @param journal Pointer to the journal object
 */
void rmg_journal_unref (RmgJournal *journal);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgJournal, rmg_journal_unref);

G_END_DECLS
