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
 * \file rmg-jentry.h
 */

#pragma once

#include "rmg-types.h"

#include <glib.h>
#include <gmodule.h>

G_BEGIN_DECLS

/**
 * @struct RmgAEntry
 * @brief The RmgAEntry data structure
 */
typedef struct _RmgAEntry {
  gulong hash;
  RmgActionType type;
  glong trigger_level_min;
  glong trigger_level_max;
  gboolean reset_after;
} RmgAEntry;

/**
 * @struct RmgFEntry
 * @brief The RmgFEntry data structure
 */
typedef struct _RmgFEntry {
  gulong hash;
  gchar *friend_name;
  gchar *friend_context;
  RmgFriendType type;
  RmgFriendActionType action;
  glong argument;
  glong delay;
} RmgFEntry;

/**
 * @struct RmgFEntryParserHelper
 * @brief The RmgFEntry data structure
 */
typedef struct _RmgFEntryParserHelper {
  gchar *friend_context;  /**< User has to free this string manually  */
  RmgFriendType type;
  RmgFriendActionType action;
  glong argument;
  glong delay;
} RmgFEntryParserHelper;

/**
 * @struct RmgJEntry
 * @brief The RmgJEntry opaque data structure
 */
typedef struct _RmgJEntry {
  gulong hash;
  gchar *name;
  gchar *private_data;
  gchar *public_data;
  glong rvector;
  gboolean relaxing;
  gboolean check_start;
  glong timeout;
  GList *actions;
  GList *friends;

  GRand *hash_generator;
  const gchar *parser_current_element;
  RmgFEntryParserHelper parser_current_friend;

  grefcount rc; /**< Reference counter variable  */
} RmgJEntry;

#define RMG_JENTRY_TO_PTR(e) ((gpointer)(RmgJEntry *)(e))
#define RMG_FENTRY_TO_PTR(e) ((gpointer)(RmgFEntry *)(e))
#define RMG_AENTRY_TO_PTR(a) ((gpointer)(RmgAEntry *)(a))

/*
 * @brief Create a new jentry object
 * @return On success return a new RmgJEntry object otherwise return NULL
 */
RmgJEntry *             rmg_jentry_new                      (gulong version);

/**
 * @brief Aquire jentry object
 * @param jentry Pointer to the jentry object
 */
RmgJEntry *             rmg_jentry_ref                      (RmgJEntry *jentry);

/**
 * @brief Release jentry object
 * @param jentry Pointer to the jentry object
 */
void                    rmg_jentry_unref                    (RmgJEntry *jentry);

/**
 * @brief Setter
 */
void                    rmg_jentry_set_name                 (RmgJEntry *jentry, 
                                                             const gchar *name);

/**
 * @brief Setter
 */
void                    rmg_jentry_set_private_data_path    (RmgJEntry *jentry, 
                                                             const gchar *dpath);

/**
 * @brief Setter
 */
void                    rmg_jentry_set_public_data_path     (RmgJEntry *jentry, 
                                                             const gchar *dpath);

/**
 * @brief Setter
 */
void                    rmg_jentry_set_rvector              (RmgJEntry *jentry, 
                                                             glong rvector);

/**
 * @brief Setter
 */
void                    rmg_jentry_set_relaxing             (RmgJEntry *jentry, 
                                                             gboolean relaxing);

/**
 * @brief Setter
 */
void                    rmg_jentry_set_timeout              (RmgJEntry *jentry, 
                                                             glong timeout);

/**
 * @brief Setter
 */
void                    rmg_jentry_set_checkstart           (RmgJEntry *jentry, 
                                                             gboolean check_start);

/**
 * @brief Setter
 */
void                    rmg_jentry_add_action               (RmgJEntry *jentry,
                                                             RmgActionType type,
                                                             glong trigger_level_min,
                                                             glong trigger_level_max,
                                                             gboolean reset_after);

/**
 * @brief Setter
 */
void                    rmg_jentry_add_friend               (RmgJEntry *jentry,
                                                             const gchar *friend_name,
                                                             const gchar *friend_context,
                                                             RmgFriendType type,
                                                             RmgFriendActionType action,
                                                             glong argument,
                                                             glong delay);

/**
 * @brief Getter
 */
gulong                  rmg_jentry_get_hash                 (RmgJEntry *jentry);

/**
 * @brief Getter
 */
const gchar *           rmg_jentry_get_name                 (RmgJEntry *jentry);

/**
 * @brief Getter
 */
const gchar *           rmg_jentry_get_private_data_path    (RmgJEntry *jentry);

/**
 * @brief Getter
 */
const gchar *           rmg_jentry_get_public_data_path     (RmgJEntry *jentry);

/**
 * @brief Getter
 */
glong                   rmg_jentry_get_rvector              (RmgJEntry *jentry);

/**
 * @brief Getter
 */
gboolean                rmg_jentry_get_relaxing             (RmgJEntry *jentry);

/**
 * @brief Getter
 */
glong                   rmg_jentry_get_timeout              (RmgJEntry *jentry);

/**
 * @brief Getter
 */
gboolean                rmg_jentry_get_checkstart           (RmgJEntry *jentry);

/**
 * @brief Getter
 */
const GList *           rmg_jentry_get_actions              (RmgJEntry *jentry);

/**
 * @brief Getter
 */
const GList *           rmg_jentry_get_friends              (RmgJEntry *jentry);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgJEntry, rmg_jentry_unref);

G_END_DECLS
