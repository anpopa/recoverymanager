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
 * \file rmg-options.h
 */

#pragma once

#include "rmg-types.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @enum RmgOptionsKey
 * @brief The option keys
 */
typedef enum _RmgOptionsKey
{
  KEY_RUN_MODE,
  KEY_UNITS_DIR,
  KEY_DATABASE_DIR,
  KEY_PUBLIC_DATA_RESET_CMD,
  KEY_PRIVATE_DATA_RESET_CMD,
  KEY_PLATFORM_RESTART_CMD,
  KEY_FACTORY_RESET_CMD,
  KEY_IPC_SOCK_ADDR,
  KEY_IPC_TIMEOUT_SEC,
  KEY_INTEGRITY_CHECK_SEC
} RmgOptionsKey;

/**
 * @struct RmgOptions
 * @brief The RmgOptions opaque data structure
 */
typedef struct _RmgOptions
{
  GKeyFile *conf;    /**< The GKeyFile object */
  gboolean has_conf; /**< Flag to check if a runtime option object is available */
  grefcount rc;      /**< Reference counter variable  */
} RmgOptions;

/*
 * @brief Create a new options object
 * @return On success return a new RmgOptions object otherwise return NULL
 */
RmgOptions *rmg_options_new (const gchar *conf_path);

/*
 * @brief Aquire options object
 * @return On success return a new RmgOptions object otherwise return NULL
 */
RmgOptions *rmg_options_ref (RmgOptions *opts);

/**
 * @brief Release an options object
 * @param opts Pointer to the options object
 */
void rmg_options_unref (RmgOptions *opts);

/**
 * @brief Get the GKeyFile object
 * @param opts Pointer to the options object
 */
GKeyFile *rmg_options_get_key_file (RmgOptions *opts);

/*
 * @brief Get a configuration value string for key
 * @param opts Pointer to the options object
 * @param key Option key
 * @param error The value status argument to update during call
 * @return On success return a reference to the optional value otherwise return
 * NULL
 */
gchar *rmg_options_string_for (RmgOptions *opts, RmgOptionsKey key);

/*
 * @brief Get a configuration gint64 value for key
 * @param opts Pointer to the options object
 * @param key Option key
 * @param error The value status argument to update during call
 * @return On success return a reference to the optional value otherwise return
 * NULL
 */
gint64 rmg_options_long_for (RmgOptions *opts, RmgOptionsKey key);

G_END_DECLS
