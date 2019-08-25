/* rmg-mentry.h
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
#include <gio/gio.h>

G_BEGIN_DECLS

typedef enum _ServiceActiveState {
  SERVICE_STATE_UNKNOWN,
  SERVICE_STATE_ACTIVE,
  SERVICE_STATE_RELOADING,
  SERVICE_STATE_INACTIVE,
  SERVICE_STATE_FAILED,
  SERVICE_STATE_ACTIVATING,
  SERVICE_STATE_DEACTIVATING
} ServiceActiveState;

typedef enum _ServiceActiveSubstate {
  SERVICE_SUBSTATE_UNKNOWN,
  SERVICE_SUBSTATE_RUNNING,
  SERVICE_SUBSTATE_DEAD,
  SERVICE_SUBSTATE_STOP_SIGTERM
} ServiceActiveSubstate;

/**
 * @struct Service monitor entry
 * @brief Reprezentation of a service with state from systemd
 */
typedef struct _RmgMEntry {
  grefcount rc;     /**< Reference counter variable  */
  GDBusProxy *proxy;
  gpointer dispatcher;
  gchar *service_name;
  gchar *object_path;
  ServiceActiveState active_state;
  ServiceActiveSubstate active_substate;
} RmgMEntry;

#define RMG_MENTRY_TO_PTR(e) ((gpointer)(RmgMEntry *)(e))

/*
 * @brief Create a new mentry object
 * @return On success return a new RmgMEntry object otherwise return NULL
 */
RmgMEntry *rmg_mentry_new (const gchar *service_name,
                           const gchar *object_path,
                           ServiceActiveState active_state,
                           ServiceActiveSubstate active_substate);

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
 * @return Build status
 */
RmgStatus rmg_mentry_build_proxy (RmgMEntry *mentry, gpointer _dispatcher);

G_END_DECLS
