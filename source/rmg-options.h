/* rmg-options.h
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

G_BEGIN_DECLS

/**
 * @enum RmgOptionsKey
 * @brief The option keys
 */
typedef enum _RmgOptionsKey {
  KEY_RUN_MODE,
  KEY_UNITS_DIR,
  KEY_DATABASE_DIR,
  KEY_PUBLIC_DATA_RESET_CMD,
  KEY_PRIVATE_DATA_RESET_CMD,
  KEY_PLATFORM_RESTART_CMD,
  KEY_FACTORY_RESET_CMD,
  KEY_IPC_SOCK_ADDR,
  KEY_IPC_TIMEOUT_SEC
} RmgOptionsKey;

/**
 * @struct RmgOptions
 * @brief The RmgOptions opaque data structure
 */
typedef struct _RmgOptions {
  GKeyFile *conf;   /**< The GKeyFile object */
  bool has_conf;      /**< Flag to check if a runtime option object is available */
  grefcount rc;     /**< Reference counter variable  */
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
 *
 * @param opts Pointer to the options object
 * @param key Option key
 * @param error The value status argument to update during call
 *
 * @return On success return a reference to the optional value otherwise return NULL
 */
gchar *rmg_options_string_for (RmgOptions *opts, RmgOptionsKey key);

/*
 * @brief Get a configuration gint64 value for key
 *
 * @param opts Pointer to the options object
 * @param key Option key
 * @param error The value status argument to update during call
 *
 * @return On success return a reference to the optional value otherwise return NULL
 */
gint64 rmg_options_long_for (RmgOptions *opts, RmgOptionsKey key);

G_END_DECLS

