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
  RmgOptions *options;
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

/**
 * @brief Reload units from filesystem
 * @param journal Pointer to the journal object
 * @param error The GError object or NULL
 * @return On success return RMG_STATUS_OK
 */
RmgStatus rmg_journal_reload_units (RmgJournal *journal, GError **error);

/**
 * @brief Get service hash if exist
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 * @return If found return the current version in DB and 0 if not found
 */
gulong rmg_journal_get_hash (RmgJournal *journal,
                             const gchar *service_name,
                             GError **error);

/**
 * @brief Add new service entry in database
 *
 * @param journal Pointer to the journal object
 * @param hash The entry unique hash value
 * @param service_name The service name to lookup
 * @param private_data Private data directory path
 * @param public_data Public data directory path
 * @param timeput Relaxation timeout
 * @param error The GError object or NULL
 *
 * @return On success return RMG_STATUS_OK
 */
RmgStatus rmg_journal_add_service (RmgJournal *journal,
                                   gulong hash,
                                   const gchar *service_name,
                                   const gchar *private_data,
                                   const gchar *public_data,
                                   glong timeout,
                                   GError **error);
/**
 * @brief Add new action entry in database
 *
 * @param journal Pointer to the journal object
 * @param hash The entry unique hash value
 * @param service_name The service name to lookup
 * @param trigger_level_min The rvector min trigger level
 * @param trigger_level_max The rvector max trigger level
 * @param error The GError object or NULL
 *
 * @return On success return RMG_STATUS_OK
 */
RmgStatus rmg_journal_add_action (RmgJournal *journal,
                                  gulong hash,
                                  const gchar *service_name,
                                  RmgActionType action_type,
                                  glong trigger_level_min,
                                  glong trigger_level_max,
                                  GError **error);
/**
 * @brief Get private data path for service
 *
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 *
 * @return A new allocated string with requested data
 */
gchar *rmg_journal_get_private_data_path (RmgJournal *journal,
                                          const gchar *service_name,
                                          GError **error);
/**
 * @brief Get public data path for service
 *
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 *
 * @return A new allocated string with requested data
 */
gchar *rmg_journal_get_public_data_path (RmgJournal *journal,
                                         const gchar *service_name,
                                         GError **error);
/**
 * @brief Get relaxing state
 *
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 *
 * @return Boolean value of relaxing state
 */
gboolean rmg_journal_get_relaxing_state (RmgJournal *journal,
                                         const gchar *service_name,
                                         GError **error);
/**
 * @brief Set relaxing state
 *
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param relaxing_status The new state
 * @param error The GError object or NULL
 *
 * @return On success return RMG_STATUS_OK
 */
RmgStatus rmg_journal_set_relaxing_state (RmgJournal *journal,
                                          const gchar *service_name,
                                          gboolean relaxing_status,
                                          GError **error);
/**
 * @brief Get timeout value
 *
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 *
 * @return Timeout value
 */
glong rmg_journal_get_relaxing_timeout (RmgJournal *journal,
                                        const gchar *service_name,
                                        GError **error);
/**
 * @brief Get rvector value
 *
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 *
 * @return Gradiant value
 */
glong rmg_journal_get_rvector (RmgJournal *journal,
                               const gchar *service_name,
                               GError **error);

/**
 * @brief Set rvector value
 *
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 *
 * @return On success return RMG_STATUS_OK
 */
RmgStatus rmg_journal_set_rvector (RmgJournal *journal,
                                   const gchar *service_name,
                                   glong rvector,
                                   GError **error);
/**
 * @brief Get current action for service
 *
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 *
 * @return The current action according with service state
 */
RmgActionType rmg_journal_get_service_action (RmgJournal *journal,
                                              const gchar *service_name,
                                              GError **error);
/**
 * @brief Remove service
 *
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 *
 * @return On success return RMG_STATUS_OK
 */
RmgStatus rmg_journal_remove_service (RmgJournal *journal,
                                      const gchar *service_name,
                                      GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgJournal, rmg_journal_unref);

G_END_DECLS
