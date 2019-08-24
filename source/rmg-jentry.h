/* rmg-jentry.h
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

#include <glib.h>
#include <gmodule.h>

G_BEGIN_DECLS

/**
 * @struct RmgAEntry
 * @brief The RmgAEntry opaque data structure
 */
typedef struct _RmgAEntry {
  gulong hash;
  RmgActionType type;
  glong trigger_level_min;
  glong trigger_level_max;
} RmgAEntry;

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
  glong timeout;
  GList *actions;

  GRand *hash_generator;
  const gchar *parser_current_element;
  grefcount rc;     /**< Reference counter variable  */
} RmgJEntry;

#define RMG_JENTRY_TO_PTR(e) ((gpointer)(RmgJEntry *)(e))
#define RMG_AENTRY_TO_PTR(a) ((gpointer)(RmgAEntry *)(a))

/*
 * @brief Create a new jentry object
 * @return On success return a new RmgJEntry object otherwise return NULL
 */
RmgJEntry *rmg_jentry_new (gulong version);

/**
 * @brief Aquire jentry object
 * @param jentry Pointer to the jentry object
 */
RmgJEntry *rmg_jentry_ref (RmgJEntry *jentry);

/**
 * @brief Release jentry object
 * @param jentry Pointer to the jentry object
 */
void rmg_jentry_unref (RmgJEntry *jentry);

/**
 * @brief Setter
 */
void rmg_jentry_set_name (RmgJEntry *jentry, const gchar *name);

/**
 * @brief Setter
 */
void rmg_jentry_set_private_data_path (RmgJEntry *jentry, const gchar *dpath);

/**
 * @brief Setter
 */
void rmg_jentry_set_public_data_path (RmgJEntry *jentry, const gchar *dpath);

/**
 * @brief Setter
 */
void rmg_jentry_set_rvector (RmgJEntry *jentry, glong rvector);

/**
 * @brief Setter
 */
void rmg_jentry_set_relaxing (RmgJEntry *jentry, gboolean relaxing);

/**
 * @brief Setter
 */
void rmg_jentry_set_timeout (RmgJEntry *jentry, glong timeout);

/**
 * @brief Setter
 */
void rmg_jentry_add_action (RmgJEntry *jentry, RmgActionType type, glong trigger_level_min, glong trigger_level_max);

/**
 * @brief Getter
 */
gulong rmg_jentry_get_hash (RmgJEntry *jentry);

/**
 * @brief Getter
 */
const gchar *rmg_jentry_get_name (RmgJEntry *jentry);

/**
 * @brief Getter
 */
const gchar *rmg_jentry_get_private_data_path (RmgJEntry *jentry);

/**
 * @brief Getter
 */
const gchar *rmg_jentry_get_public_data_path (RmgJEntry *jentry);

/**
 * @brief Getter
 */
glong rmg_jentry_get_rvector (RmgJEntry *jentry);

/**
 * @brief Getter
 */
gboolean rmg_jentry_get_relaxing (RmgJEntry *jentry);

/**
 * @brief Getter
 */
glong rmg_jentry_get_timeout (RmgJEntry *jentry);

/**
 * @brief Getter
 */
const GList *rmg_jentry_get_actions (RmgJEntry *jentry);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgJEntry, rmg_jentry_unref);

G_END_DECLS
