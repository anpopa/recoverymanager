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
 * \file rmg-journal.h
 */

#pragma once

#include "rmg-options.h"
#include "rmg-types.h"

#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

typedef void (*RmgJournalCallback)(gpointer _journal, gpointer _service_name);

/**
 * @struct RmgJournal
 * @brief The RmgJournal opaque data structure
 */
typedef struct _RmgJournal {
    RmgOptions *options;
    sqlite3 *database; /**< The sqlite3 database object */
    grefcount rc;      /**< Reference counter variable  */
} RmgJournal;

/**
 * @brief Create a new journal object
 * @param options A pointer to the RmgOptions object created by the main
 * application
 * @param error The GError object or NULL
 * @return On success return a new RmgJournal object otherwise return NULL
 */
RmgJournal *rmg_journal_new(RmgOptions *options, GError **error);

/**
 * @brief Aquire journal object
 * @param journal Pointer to the journal object
 * @return The referenced journal object
 */
RmgJournal *rmg_journal_ref(RmgJournal *journal);

/**
 * @brief Release an journal object
 * @param journal Pointer to the journal object
 */
void rmg_journal_unref(RmgJournal *journal);

/**
 * @brief Reload units from filesystem
 * @param journal Pointer to the journal object
 * @param error The GError object or NULL
 * @return On success return RMG_STATUS_OK
 */
RmgStatus rmg_journal_reload_units(RmgJournal *journal, GError **error);

/**
 * @brief Get service hash if exist
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 * @return If found return the current version in DB and 0 if not found
 */
gulong rmg_journal_get_hash(RmgJournal *journal, const gchar *service_name, GError **error);

/**
 * @brief Add new service entry in database
 * @param journal Pointer to the journal object
 * @param hash The entry unique hash value
 * @param service_name The service name to lookup
 * @param private_data Private data directory path
 * @param public_data Public data directory path
 * @param timeput Relaxation timeout
 * @param error The GError object or NULL
 * @return On success return RMG_STATUS_OK
 */
RmgStatus rmg_journal_add_service(RmgJournal *journal,
                                  gulong hash,
                                  const gchar *service_name,
                                  const gchar *private_data,
                                  const gchar *public_data,
                                  gboolean check_start,
                                  glong timeout,
                                  GError **error);
/**
 * @brief Add new action entry in database
 * @param journal Pointer to the journal object
 * @param hash The entry unique hash value
 * @param service_name The service name to lookup
 * @param trigger_level_min The rvector min trigger level
 * @param trigger_level_max The rvector max trigger level
 * @param error The GError object or NULL
 * @return On success return RMG_STATUS_OK
 */
RmgStatus rmg_journal_add_action(RmgJournal *journal,
                                 gulong hash,
                                 const gchar *service_name,
                                 RmgActionType action_type,
                                 glong trigger_level_min,
                                 glong trigger_level_max,
                                 gboolean reset_after,
                                 GError **error);

/**
 * @brief Add new friend entry in database
 * @param journal Pointer to the journal object
 * @param hash The entry unique hash value
 * @param service_name The service name to lookup
 * @param friend_name The friend name
 * @param friend_context The friend context name
 * @param friend_type The friend type
 * @param friend_action The friend action type
 * @param friend_argument The friend action argument
 * @param friend_delay The friend action delay
 * @param error The GError object or NULL
 * @return On success return RMG_STATUS_OK
 */
RmgStatus rmg_journal_add_friend(RmgJournal *journal,
                                 gulong hash,
                                 const gchar *service_name,
                                 const gchar *friend_name,
                                 const gchar *friend_context,
                                 RmgFriendType friend_type,
                                 RmgFriendActionType friend_action,
                                 glong friend_argument,
                                 glong friend_delay,
                                 GError **error);
/**
 * @brief Get check start value
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 * @return Check start value
 */
gboolean rmg_journal_get_checkstart(RmgJournal *journal, const gchar *service_name, GError **error);
/**
 * @brief Get private data path for service
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 * @return A new allocated string with requested data
 */
gchar *
rmg_journal_get_private_data_path(RmgJournal *journal, const gchar *service_name, GError **error);
/**
 * @brief Get public data path for service
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 * @return A new allocated string with requested data
 */
gchar *
rmg_journal_get_public_data_path(RmgJournal *journal, const gchar *service_name, GError **error);
/**
 * @brief Get timeout value
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 * @return Timeout value
 */
glong rmg_journal_get_relaxing_timeout(RmgJournal *journal,
                                       const gchar *service_name,
                                       GError **error);
/**
 * @brief Call funtion for each relaxing service
 * @param journal Pointer to the journal object
 * @param callback GSourceFunc callback
 * @param error The GError object or NULL
 */
void rmg_journal_call_foreach_relaxing(RmgJournal *journal,
                                       RmgJournalCallback callback,
                                       GError **error);
/**
 * @brief Call funtion for each service with check start flag set
 * @param journal Pointer to the journal object
 * @param callback GSourceFunc callback
 * @param error The GError object or NULL
 */
void rmg_journal_call_foreach_checkstart(RmgJournal *journal,
                                         RmgJournalCallback callback,
                                         GError **error);
/**
 * @brief Get rvector value
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 * @return Gradiant value
 */
glong rmg_journal_get_rvector(RmgJournal *journal, const gchar *service_name, GError **error);

/**
 * @brief Set rvector value
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 * @return On success return RMG_STATUS_OK
 */
RmgStatus rmg_journal_set_rvector(RmgJournal *journal,
                                  const gchar *service_name,
                                  glong rvector,
                                  GError **error);
/**
 * @brief Get current action for service
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 * @return The current action according with service state
 */
RmgActionType
rmg_journal_get_service_action(RmgJournal *journal, const gchar *service_name, GError **error);
/**
 * @brief Get reset after flag for current action for service
 * @param journal Pointer to the journal object
 * @param service_name The service name to lookup
 * @param error The GError object or NULL
 * @return True if the reset after flag is set for current action
 */
gboolean rmg_journal_get_service_action_reset_after(RmgJournal *journal,
                                                    const gchar *service_name,
                                                    GError **error);

/**
 * @brief Get services for friend
 * @param journal Pointer to the journal object
 * @param friend_name The friend name to lookup
 * @param friend_context The friend context to lookup
 * @param friend_type The friend type to lookup
 * @param error The GError object or NULL
 * @return A GList with new allocated structures RmgFriendResponseEntry
 */
GList *rmg_journal_get_services_for_friend(RmgJournal *journal,
                                           const gchar *friend_name,
                                           const gchar *friend_context,
                                           RmgFriendType friend_type,
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
RmgStatus
rmg_journal_remove_service(RmgJournal *journal, const gchar *service_name, GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(RmgJournal, rmg_journal_unref);

G_END_DECLS
