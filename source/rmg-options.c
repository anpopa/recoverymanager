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
 * \file rmg-options.c
 */

#include "rmg-options.h"
#include "rmg-defaults.h"

#include <glib.h>
#include <stdlib.h>

static gint64 get_long_option(RmgOptions *opts,
                              const gchar *section_name,
                              const gchar *property_name,
                              GError **error);

RmgOptions *rmg_options_new(const gchar *conf_path)
{
    RmgOptions *opts = g_new0(RmgOptions, 1);

    opts->has_conf = FALSE;

    if (conf_path != NULL) {
        g_autoptr(GError) error = NULL;

        opts->conf = g_key_file_new();

        g_assert(opts->conf);

        if (g_key_file_load_from_file(opts->conf, conf_path, G_KEY_FILE_NONE, &error) == TRUE)
            opts->has_conf = TRUE;
        else
            g_debug("Cannot parse configuration file");
    }

    g_ref_count_init(&opts->rc);

    return opts;
}

RmgOptions *rmg_options_ref(RmgOptions *opts)
{
    g_assert(opts);
    g_ref_count_inc(&opts->rc);
    return opts;
}

void rmg_options_unref(RmgOptions *opts)
{
    g_assert(opts);

    if (g_ref_count_dec(&opts->rc) == TRUE) {
        if (opts->conf)
            g_key_file_unref(opts->conf);

        g_free(opts);
    }
}

GKeyFile *rmg_options_get_key_file(RmgOptions *opts)
{
    g_assert(opts);
    return opts->conf;
}

gchar *rmg_options_string_for(RmgOptions *opts, RmgOptionsKey key)
{
    switch (key) {
    case KEY_RUN_MODE:
        if (opts->has_conf) {
            gchar *tmp = g_key_file_get_string(opts->conf, "recoverymanager", "RunMode", NULL);

            if (tmp != NULL)
                return tmp;
        }
        return g_strdup(RMG_RUN_MODE);

    case KEY_DATABASE_DIR:
        if (opts->has_conf) {
            gchar *tmp
                = g_key_file_get_string(opts->conf, "recoverymanager", "DatabaseDirectory", NULL);

            if (tmp != NULL)
                return tmp;
        }
        return g_strdup(RMG_DATABASE_DIR);

    case KEY_UNITS_DIR:
        if (opts->has_conf) {
            gchar *tmp
                = g_key_file_get_string(opts->conf, "recoverymanager", "UnitsDirectory", NULL);

            if (tmp != NULL)
                return tmp;
        }
        return g_strdup(RMG_UNITS_DIR);

    case KEY_PRIVATE_DATA_RESET_CMD:
        if (opts->has_conf) {
            gchar *tmp = g_key_file_get_string(
                opts->conf, "recoverymanager", "PrivateDataResetCommand", NULL);

            if (tmp != NULL)
                return tmp;
        }
        return g_strdup(RMG_PRIVATE_DATA_RESET_CMD);

    case KEY_PUBLIC_DATA_RESET_CMD:
        if (opts->has_conf) {
            gchar *tmp = g_key_file_get_string(
                opts->conf, "recoverymanager", "PublicDataResetCommand", NULL);

            if (tmp != NULL)
                return tmp;
        }
        return g_strdup(RMG_PUBLIC_DATA_RESET_CMD);

    case KEY_PLATFORM_RESTART_CMD:
        if (opts->has_conf) {
            gchar *tmp = g_key_file_get_string(
                opts->conf, "recoverymanager", "PlatformRestartCommand", NULL);

            if (tmp != NULL)
                return tmp;
        }
        return g_strdup(RMG_PLATFORM_RESTART_CMD);

    case KEY_FACTORY_RESET_CMD:
        if (opts->has_conf) {
            gchar *tmp
                = g_key_file_get_string(opts->conf, "recoverymanager", "FactoryResetCommand", NULL);

            if (tmp != NULL)
                return tmp;
        }
        return g_strdup(RMG_FACTORY_RESET_CMD);

    case KEY_IPC_SOCK_ADDR:
        if (opts->has_conf) {
            gchar *tmp
                = g_key_file_get_string(opts->conf, "recoverymanager", "IpcSocketFile", NULL);

            if (tmp != NULL)
                return tmp;
        }
        return g_strdup(RMG_IPC_SOCK_ADDR);

    default:
        break;
    }

    g_error("No default value provided for string key");

    return NULL;
}

static gint64 get_long_option(RmgOptions *opts,
                              const gchar *section_name,
                              const gchar *property_name,
                              GError **error)
{
    g_assert(opts);
    g_assert(section_name);
    g_assert(property_name);

    if (opts->has_conf) {
        g_autofree gchar *tmp
            = g_key_file_get_string(opts->conf, section_name, property_name, NULL);

        if (tmp != NULL) {
            gchar *c = NULL;
            gint64 ret;

            ret = g_ascii_strtoll(tmp, &c, 10);

            if (*c != tmp[0])
                return ret;
        }
    }

    g_set_error_literal(
        error, g_quark_from_string("rmg-options"), 0, "Cannot convert option to long");

    return -1;
}

gint64 rmg_options_long_for(RmgOptions *opts, RmgOptionsKey key)
{
    g_autoptr(GError) error = NULL;
    gint64 value = 0;

    switch (key) {
    case KEY_IPC_TIMEOUT_SEC:
        value = get_long_option(opts, "recoverymanager", "IpcSocketTimeout", &error);
        if (error != NULL)
            value = RMG_IPC_TIMEOUT_SEC;
        break;

    case KEY_INTEGRITY_CHECK_SEC:
        value = get_long_option(opts, "recoverymanager", "IntegrityCheckTimeout", &error);
        if (error != NULL)
            value = RMG_INTEGRITY_CHECK_SEC;
        break;

    default:
        break;
    }

    return value;
}
