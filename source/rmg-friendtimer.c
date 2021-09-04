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
 * \file rmg-friendtimer.c
 */

#include "rmg-friendtimer.h"
#include "rmg-executor.h"
#include "rmg-utils.h"

/**
 * @struct RmgFriendTimer
 * @brief The RmgFriendTimer data structure
 */
typedef struct _RmgFriendTimer {
    RmgExecutor *executor;
    gchar *service_name;
    RmgFriendActionType action;
    glong argument;
} RmgFriendTimer;

static gboolean friendtimer_callback(gpointer _ftimer)
{
    RmgFriendTimer *ftimer = (RmgFriendTimer *) _ftimer;
    GDBusProxy *sd_manager_proxy = NULL;

    g_autoptr(GVariant) response = NULL;
    g_autoptr(GError) error = NULL;

    g_assert(ftimer);

    sd_manager_proxy = ftimer->executor->sd_manager_proxy;

    g_debug("Friend timer expired for service='%s' action='%s' arg='%ld'",
            ftimer->service_name,
            rmg_utils_friend_action_name(ftimer->action),
            ftimer->argument);

    if ((ftimer->action != FRIEND_ACTION_SIGNAL) && (sd_manager_proxy == NULL)) {
        g_warning("DBUS action needed for friend, but Manager proxy not available");
        return FALSE;
    }

    switch (ftimer->action) {
    case FRIEND_ACTION_START: {
        response = g_dbus_proxy_call_sync(sd_manager_proxy,
                                          "StartUnit",
                                          g_variant_new("(ss)", ftimer->service_name, "replace"),
                                          G_DBUS_CALL_FLAGS_NONE,
                                          -1,
                                          NULL,
                                          &error);

        if (error != NULL)
            g_warning("Fail to call StartUnit on Manager proxy. Error %s", error->message);
        else
            g_info("Request StartUnit for unit='%s' on friend timer callback",
                   ftimer->service_name);
    } break;

    case FRIEND_ACTION_STOP: {
        response = g_dbus_proxy_call_sync(sd_manager_proxy,
                                          "StopUnit",
                                          g_variant_new("(ss)", ftimer->service_name, "replace"),
                                          G_DBUS_CALL_FLAGS_NONE,
                                          -1,
                                          NULL,
                                          &error);

        if (error != NULL)
            g_warning("Fail to call StopUnit on Manager proxy. Error %s", error->message);
        else
            g_info("Request StopUnit for unit='%s' on friend timer callback", ftimer->service_name);
    } break;

    case FRIEND_ACTION_RESTART: {
        response = g_dbus_proxy_call_sync(sd_manager_proxy,
                                          "RestartUnit",
                                          g_variant_new("(ss)", ftimer->service_name, "replace"),
                                          G_DBUS_CALL_FLAGS_NONE,
                                          -1,
                                          NULL,
                                          &error);

        if (error != NULL)
            g_warning("Fail to call RestartUnit on Manager proxy. Error %s", error->message);
        else
            g_info("Request RestartUnit for unit='%s' on friend timer callback",
                   ftimer->service_name);
    } break;

    case FRIEND_ACTION_SIGNAL: {
        response = g_dbus_proxy_call_sync(
            sd_manager_proxy,
            "KillUnit",
            g_variant_new("(ssi)", ftimer->service_name, "main", ftimer->argument),
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error);

        if (error != NULL)
            g_warning("Fail to call RestartUnit on Manager proxy. Error %s", error->message);
        else
            g_info("Request KillUnit for unit='%s' with arg='%ld' on timer callback",
                   ftimer->service_name,
                   ftimer->argument);
    } break;

    default:
        g_warning("Unknown action for friend");
        break;
    }

    return FALSE;
}

void friendtimer_destroy_notify(gpointer _ftimer)
{
    RmgFriendTimer *ftimer = (RmgFriendTimer *) _ftimer;

    g_assert(ftimer);

    rmg_executor_unref((RmgExecutor *) ftimer->executor);
    g_free(ftimer->service_name);
    g_free(ftimer);
}

void rmg_friendtimer_trigger(const gchar *service_name,
                             RmgFriendActionType action,
                             glong argument,
                             gpointer executor,
                             guint timeout)
{
    RmgFriendTimer *ftimer = g_new0(RmgFriendTimer, 1);

    g_assert(service_name);

    ftimer->executor = rmg_executor_ref((RmgExecutor *) executor);
    ftimer->service_name = g_strdup(service_name);
    ftimer->action = action;
    ftimer->argument = argument;

    (void) g_timeout_add_seconds_full(
        G_PRIORITY_DEFAULT, timeout, friendtimer_callback, ftimer, friendtimer_destroy_notify);
}
