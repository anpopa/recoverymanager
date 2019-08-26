/* rmg-options.c
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

#include "rmg-options.h"
#include "rmg-defaults.h"

#include <glib.h>
#include <stdlib.h>

static gint64 get_long_option (RmgOptions *opts,
                               const gchar *section_name,
                               const gchar *property_name,
                               GError **error);

RmgOptions *
rmg_options_new (const gchar *conf_path)
{
  RmgOptions *opts = g_new0 (RmgOptions, 1);

  opts->has_conf = false;

  if (conf_path != NULL)
    {
      g_autoptr (GError) error = NULL;

      opts->conf = g_key_file_new ();

      g_assert (opts->conf);

      if (g_key_file_load_from_file (opts->conf, conf_path, G_KEY_FILE_NONE, &error) == TRUE)
        {
          opts->has_conf = true;
        }
      else
        {
          g_debug ("Cannot parse configuration file");
        }
    }

  g_ref_count_init (&opts->rc);

  return opts;
}

RmgOptions *
rmg_options_ref (RmgOptions *opts)
{
  g_assert (opts);
  g_ref_count_inc (&opts->rc);
  return opts;
}

void
rmg_options_unref (RmgOptions *opts)
{
  g_assert (opts);

  if (g_ref_count_dec (&opts->rc) == TRUE)
    {
      if (opts->conf)
        g_key_file_unref (opts->conf);

      g_free (opts);
    }
}

GKeyFile *
rmg_options_get_key_file (RmgOptions *opts)
{
  g_assert (opts);
  return opts->conf;
}

gchar *
rmg_options_string_for (RmgOptions *opts,
                        RmgOptionsKey key)
{
  switch (key)
    {
    case KEY_RUN_MODE:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "recoverymanager", "RunMode", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (RMG_RUN_MODE);

    case KEY_DATABASE_DIR:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "recoverymanager", "DatabaseDirectory", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (RMG_DATABASE_DIR);

    case KEY_UNITS_DIR:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "recoverymanager", "UnitsDirectory", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (RMG_UNITS_DIR);

    case KEY_PRIVATE_DATA_RESET_CMD:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "recoverymanager", "PrivateDataResetCommand", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (RMG_PRIVATE_DATA_RESET_CMD);

    case KEY_PUBLIC_DATA_RESET_CMD:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "recoverymanager", "PublicDataResetCommand", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (RMG_PUBLIC_DATA_RESET_CMD);

    case KEY_PLATFORM_RESTART_CMD:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "recoverymanager", "PlatformRestartCommand", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (RMG_PLATFORM_RESTART_CMD);

    case KEY_FACTORY_RESET_CMD:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "recoverymanager", "FactoryResetCommand", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (RMG_FACTORY_RESET_CMD);

    case KEY_IPC_SOCK_ADDR:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "recoverymanager", "IpcSocketFile", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (RMG_IPC_SOCK_ADDR);

    default:
      break;
    }

  g_error ("No default value provided for string key");

  return NULL;
}

static gint64
get_long_option (RmgOptions *opts,
                 const gchar *section_name,
                 const gchar *property_name,
                 GError **error)
{
  g_assert (opts);
  g_assert (section_name);
  g_assert (property_name);

  if (opts->has_conf)
    {
      g_autofree gchar *tmp = g_key_file_get_string (opts->conf, section_name, property_name, NULL);

      if (tmp != NULL)
        {
          gchar *c = NULL;
          gint64 ret;

          ret = g_ascii_strtoll (tmp, &c, 10);

          if (*c != tmp[0])
            return ret;
        }
    }

  g_set_error_literal (error, g_quark_from_string ("rmg-options"), 0, "Cannot convert option to long");

  return -1;
}

gint64
rmg_options_long_for (RmgOptions *opts,
                      RmgOptionsKey key)
{
  g_autoptr (GError) error = NULL;
  gint64 value = 0;

  switch (key)
    {
    case KEY_IPC_TIMEOUT_SEC:
      value = get_long_option (opts, "recoverymanager", "IpcSocketTimeout", &error);
      if (error != NULL)
        value = RMG_IPC_TIMEOUT_SEC;
      break;

    default:
      break;
    }

  return value;
}

