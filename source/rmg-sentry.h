/* rmg-sentry.h
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
 * @struct RmgSEntry
 * @brief The RmgSEntry opaque data structure
 */
typedef struct _RmgSEntry {
  gulong hash;
  gchar *name;
  gchar *private_data;
  gchar *public_data;
  glong rvector;
  gboolean relaxing;
  glong timeout;
  const gchar *parser_current_element;
  GList *actions;
  grefcount rc;     /**< Reference counter variable  */
} RmgSEntry;

#define RMG_SENTRY_TO_PTR(e) ((gpointer)(RmgSEntry *)(e))
#define RMG_AENTRY_TO_PTR(a) ((gpointer)(RmgAEntry *)(a))

/*
 * @brief Create a new sentry object
 * @return On success return a new RmgSEntry object otherwise return NULL
 */
RmgSEntry *rmg_sentry_new (gulong version);

/**
 * @brief Aquire sentry object
 * @param sentry Pointer to the sentry object
 */
RmgSEntry *rmg_sentry_ref (RmgSEntry *sentry);

/**
 * @brief Release sentry object
 * @param sentry Pointer to the sentry object
 */
void rmg_sentry_unref (RmgSEntry *sentry);

/**
 * @brief Setter
 */
void rmg_sentry_set_name (RmgSEntry *sentry, const gchar *name);

/**
 * @brief Setter
 */
void rmg_sentry_set_private_data_path (RmgSEntry *sentry, const gchar *dpath);

/**
 * @brief Setter
 */
void rmg_sentry_set_public_data_path (RmgSEntry *sentry, const gchar *dpath);

/**
 * @brief Setter
 */
void rmg_sentry_set_rvector (RmgSEntry *sentry, glong rvector);

/**
 * @brief Setter
 */
void rmg_sentry_set_relaxing (RmgSEntry *sentry, gboolean relaxing);

/**
 * @brief Setter
 */
void rmg_sentry_set_timeout (RmgSEntry *sentry, glong timeout);

/**
 * @brief Setter
 */
void rmg_sentry_add_action (RmgSEntry *sentry, RmgActionType type, glong trigger_level_min, glong trigger_level_max);

/**
 * @brief Getter
 */
gulong rmg_sentry_get_hash (RmgSEntry *sentry);

/**
 * @brief Getter
 */
const gchar *rmg_sentry_get_name (RmgSEntry *sentry);

/**
 * @brief Getter
 */
const gchar *rmg_sentry_get_private_data_path (RmgSEntry *sentry);

/**
 * @brief Getter
 */
const gchar *rmg_sentry_get_public_data_path (RmgSEntry *sentry);

/**
 * @brief Getter
 */
glong rmg_sentry_get_rvector (RmgSEntry *sentry);

/**
 * @brief Getter
 */
gboolean rmg_sentry_get_relaxing (RmgSEntry *sentry);

/**
 * @brief Getter
 */
glong rmg_sentry_get_timeout (RmgSEntry *sentry);

/**
 * @brief Getter
 */
const GList *rmg_sentry_get_actions (RmgSEntry *sentry);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgSEntry, rmg_sentry_unref);

G_END_DECLS
