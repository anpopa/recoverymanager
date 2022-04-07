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
 * \file rmg-mentry.h
 */

#pragma once

#include "rmg-types.h"

#include <gio/gio.h>
#include <glib.h>

G_BEGIN_DECLS

typedef enum _ServiceActiveState
{
  SERVICE_STATE_UNKNOWN,
  SERVICE_STATE_ACTIVE,
  SERVICE_STATE_RELOADING,
  SERVICE_STATE_INACTIVE,
  SERVICE_STATE_FAILED,
  SERVICE_STATE_ACTIVATING,
  SERVICE_STATE_DEACTIVATING
} ServiceActiveState;

typedef enum _ServiceActiveSubstate
{
  SERVICE_SUBSTATE_UNKNOWN,
  SERVICE_SUBSTATE_RUNNING,
  SERVICE_SUBSTATE_DEAD,
  SERVICE_SUBSTATE_STOP_SIGTERM
} ServiceActiveSubstate;

typedef void (*RmgMEntryAsyncStatus) (gpointer _mentry, gpointer _monitor, RmgStatus status);

/**
 * @struct Service monitor entry
 * @brief Reprezentation of a service with state from systemd
 */
typedef struct _RmgMEntry
{
  grefcount rc;
  GDBusProxy *proxy;
  GDBusProxy *manager_proxy;
  gpointer dispatcher;
  gchar *service_name;
  gchar *object_path;
  ServiceActiveState active_state;
  ServiceActiveSubstate active_substate;
  RmgMEntryAsyncStatus monitor_callback;
  gpointer monitor_object;
} RmgMEntry;

#define RMG_MENTRY_TO_PTR(e) ((gpointer)(RmgMEntry *)(e))

/*
 * @brief Create a new mentry object
 * @return On success return a new RmgMEntry object otherwise return NULL
 */
RmgMEntry *rmg_mentry_new (const gchar *service_name, const gchar *object_path,
                           ServiceActiveState active_state, ServiceActiveSubstate active_substate);

/**
 * @brief Aquire mentry object
 * @param mentry Pointer to the mentry object
 */
RmgMEntry *rmg_mentry_ref (RmgMEntry *mentry);

/**
 * @brief Release mentry object
 * @param mentry Pointer to the mentry object
 */
void rmg_mentry_unref (RmgMEntry *mentry);

/**
 * @brief Set a reference to manager proxy to pass to dispatcher event
 */
void rmg_mentry_set_manager_proxy (RmgMEntry *mentry, GDBusProxy *manager_proxy);

/*
 * @brief Get service active state from string
 * @return Service Active State
 */
ServiceActiveState rmg_mentry_active_state_from (const gchar *state_name);

/*
 * @brief Get service active substate from string
 * @return Service Active Subtate
 */
ServiceActiveSubstate rmg_mentry_active_substate_from (const gchar *substate_name);

/*
 * @brief Get service active state from string
 * @return Service Active State
 */
const gchar *rmg_mentry_get_active_state (ServiceActiveState state);

/*
 * @brief Get service active substate from string
 * @return Service Active Subtate
 */
const gchar *rmg_mentry_get_active_substate (ServiceActiveSubstate state);

/*
 * @brief Build mentry proxy and dispatch events
 */
void rmg_mentry_build_proxy_async (RmgMEntry *mentry, gpointer _dispatcher,
                                   RmgMEntryAsyncStatus monitor_callback, gpointer monitor_data);

G_END_DECLS
