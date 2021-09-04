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
 * \file rmg-devent.c
 */

#include "rmg-devent.h"

RmgDEvent *rmg_devent_new(DispatcherEventType type)
{
    RmgDEvent *event = g_new0(RmgDEvent, 1);

    event->type = type;
    g_ref_count_init(&event->rc);

    return event;
}

RmgDEvent *rmg_devent_ref(RmgDEvent *event)
{
    g_assert(event);
    g_ref_count_inc(&event->rc);
    return event;
}

void rmg_devent_unref(RmgDEvent *event)
{
    g_assert(event);

    if (g_ref_count_dec(&event->rc) == TRUE) {
        if (event->service_name != NULL)
            g_free(event->service_name);

        if (event->process_name != NULL)
            g_free(event->process_name);

        if (event->object_path != NULL)
            g_free(event->object_path);

        if (event->context_name != NULL)
            g_free(event->context_name);

        if (event->manager_proxy != NULL)
            g_object_unref(event->manager_proxy);

        g_free(event);
    }
}

void rmg_devent_set_type(RmgDEvent *event, DispatcherEventType type)
{
    g_assert(event);
    event->type = type;
}

void rmg_devent_set_service_name(RmgDEvent *event, const gchar *service_name)
{
    g_assert(event);
    event->service_name = g_strdup(service_name);
}

void rmg_devent_set_process_name(RmgDEvent *event, const gchar *process_name)
{
    g_assert(event);
    event->process_name = g_strdup(process_name);
}

void rmg_devent_set_object_path(RmgDEvent *event, const gchar *object_path)
{
    g_assert(event);
    event->object_path = g_strdup(object_path);
}

void rmg_devent_set_context_name(RmgDEvent *event, const gchar *context_name)
{
    g_assert(event);
    event->context_name = g_strdup(context_name);
}

void rmg_devent_set_manager_proxy(RmgDEvent *event, GDBusProxy *manager_proxy)
{
    g_assert(event);
    event->manager_proxy = g_object_ref(manager_proxy);
}
