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
 * \file rmg-mentry.c
 */

#include "rmg-mentry.h"
#include "rmg-dispatcher.h"

const gchar *active_state_names[] = { "unknown", "active",     "reloading",    "inactive",
                                      "failed",  "activating", "deactivating", NULL };

const gchar *active_substate_names[]
    = { "unknown", "running", "dead", "inactive", "stop-sigterm", NULL };

extern const gchar *sd_dbus_name;
extern const gchar *sd_dbus_object_path;
extern const gchar *sd_dbus_interface_unit;
extern const gchar *sd_dbus_interface_manager;

static void
on_properties_changed (GDBusProxy *proxy, GVariant *changed_properties,
                       const gchar *const *invalidated_properties, gpointer user_data)
{
  RmgMEntry *mentry = (RmgMEntry *)user_data;

  /* Note that we are guaranteed that changed_properties and
   * invalidated_properties are never NULL
   */

  RMG_UNUSED (proxy);
  RMG_UNUSED (invalidated_properties);

  if (g_variant_n_children (changed_properties) > 0)
    {
      const gchar *active_state_str = NULL;
      const gchar *active_substate_str = NULL;
      DispatcherEventType dispatcher_event = DEVENT_UNKNOWN;
      ServiceActiveState active_state = SERVICE_STATE_UNKNOWN;
      ServiceActiveSubstate active_substate = SERVICE_SUBSTATE_UNKNOWN;
      GVariantIter *iter;
      const gchar *key;
      GVariant *value;

      g_variant_get (changed_properties, "a{sv}", &iter);

      while (g_variant_iter_loop (iter, "{&sv}", &key, &value))
        {
          if (g_strcmp0 (key, "ActiveState") == 0)
            active_state_str = g_variant_get_string (value, NULL);
          else if (g_strcmp0 (key, "SubState") == 0)
            active_substate_str = g_variant_get_string (value, NULL);
        }

      if ((active_state_str == NULL) || (active_substate_str == NULL))
        {
          g_warning ("Cannot read current active state or substate");
          return;
        }

      active_state = rmg_mentry_active_state_from (active_state_str);
      active_substate = rmg_mentry_active_substate_from (active_substate_str);

      if ((mentry->active_state != active_state) || mentry->active_substate != active_substate)
        {
          g_info ("Service '%s' state change to ActiveState='%s' SubState='%s'",
                  mentry->service_name, active_state_str, active_substate_str);

          if ((mentry->active_state != SERVICE_STATE_FAILED)
              && (active_state == SERVICE_STATE_FAILED))
            {
              dispatcher_event = DEVENT_SERVICE_CRASHED;
            }
          else
            {
              if ((mentry->active_state != SERVICE_STATE_ACTIVE)
                  && (active_state == SERVICE_STATE_ACTIVE))
                {
                  dispatcher_event = DEVENT_SERVICE_RESTARTED;
                }
            }

          mentry->active_state = active_state;
          mentry->active_substate = active_substate;

          if (dispatcher_event != DEVENT_UNKNOWN)
            {
              RmgDEvent *event = rmg_devent_new (dispatcher_event);

              rmg_devent_set_service_name (event, mentry->service_name);
              rmg_devent_set_object_path (event, mentry->object_path);
              rmg_devent_set_manager_proxy (event, mentry->manager_proxy);

              rmg_dispatcher_push_service_event ((RmgDispatcher *)mentry->dispatcher, event);
            }
        }

      g_variant_iter_free (iter);
    }
}

ServiceActiveState
rmg_mentry_active_state_from (const gchar *state_name)
{
  gint i = 0;

  while (active_state_names[i] != NULL)
    {
      if (g_strcmp0 (active_state_names[i], state_name) == 0)
        return (ServiceActiveState)i;

      i++;
    }

  return SERVICE_STATE_UNKNOWN;
}

ServiceActiveSubstate
rmg_mentry_active_substate_from (const gchar *substate_name)
{
  gint i = 0;

  while (active_substate_names[i] != NULL)
    {
      if (g_strcmp0 (active_substate_names[i], substate_name) == 0)
        return (ServiceActiveSubstate)i;

      i++;
    }

  return SERVICE_SUBSTATE_UNKNOWN;
}

const gchar *
rmg_mentry_get_active_state (ServiceActiveState state)
{
  return active_state_names[state];
}

const gchar *
rmg_mentry_get_active_substate (ServiceActiveSubstate state)
{
  return active_substate_names[state];
}

RmgMEntry *
rmg_mentry_new (const gchar *service_name, const gchar *object_path,
                ServiceActiveState active_state, ServiceActiveSubstate active_substate)
{
  RmgMEntry *mentry = g_new0 (RmgMEntry, 1);

  g_ref_count_init (&mentry->rc);

  mentry->service_name = g_strdup (service_name);
  mentry->object_path = g_strdup (object_path);
  mentry->active_state = active_state;
  mentry->active_substate = active_substate;

  return mentry;
}

RmgMEntry *
rmg_mentry_ref (RmgMEntry *mentry)
{
  g_assert (mentry);
  g_ref_count_inc (&mentry->rc);
  return mentry;
}

void
rmg_mentry_unref (RmgMEntry *mentry)
{
  g_assert (mentry);

  if (g_ref_count_dec (&mentry->rc) == TRUE)
    {
      if (mentry->service_name != NULL)
        g_free (mentry->service_name);

      if (mentry->object_path != NULL)
        g_free (mentry->object_path);

      if (mentry->proxy != NULL)
        g_object_unref (mentry->proxy);

      if (mentry->manager_proxy != NULL)
        g_object_unref (mentry->manager_proxy);

      if (mentry->dispatcher != NULL)
        rmg_dispatcher_unref ((RmgDispatcher *)mentry->dispatcher);

      g_free (mentry);
    }
}

void
rmg_mentry_set_manager_proxy (RmgMEntry *mentry, GDBusProxy *manager_proxy)
{
  g_assert (mentry);
  mentry->manager_proxy = g_object_ref (manager_proxy);
}

void
proxy_ready_async_cb (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GError) error = NULL;
  RmgMEntry *mentry = (RmgMEntry *)user_data;
  RmgStatus status = RMG_STATUS_OK;

  RMG_UNUSED (source_object);

  mentry->proxy = g_dbus_proxy_new_finish (res, &error);

  if (error != NULL)
    {
      g_warning ("Fail to build proxy for new service '%s'. Error '%s'", mentry->service_name,
                 error->message);
      status = RMG_STATUS_ERROR;
    }
  else
    {
      g_signal_connect (mentry->proxy, "g-properties-changed", G_CALLBACK (on_properties_changed),
                        mentry);
    }

  mentry->monitor_callback (mentry, mentry->monitor_object, status);
}

void
rmg_mentry_build_proxy_async (RmgMEntry *mentry, gpointer _dispatcher,
                              RmgMEntryAsyncStatus monitor_callback, gpointer _monitor)
{
  g_assert (mentry);
  g_assert (_dispatcher);

  mentry->dispatcher = rmg_dispatcher_ref ((RmgDispatcher *)_dispatcher);
  mentry->monitor_callback = monitor_callback;
  mentry->monitor_object = _monitor;

  g_dbus_proxy_new_for_bus (
      G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, NULL,                /* GDBusInterfaceInfo */
      sd_dbus_name, mentry->object_path, sd_dbus_interface_unit, NULL, /* GCancellable */
      proxy_ready_async_cb, mentry);
}
