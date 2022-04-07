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
 * \file rmg-monitor.c
 */

#include "rmg-monitor.h"
#include "rmg-devent.h"
#include "rmg-mentry.h"
#include "rmg-relaxtimer.h"

const gchar *sd_dbus_name = "org.freedesktop.systemd1";
const gchar *sd_dbus_object_path = "/org/freedesktop/systemd1";
const gchar *sd_dbus_interface_unit = "org.freedesktop.systemd1.Unit";
const gchar *sd_dbus_interface_manager = "org.freedesktop.systemd1.Manager";

/**
 * @brief Post new event
 * @param monitor A pointer to the monitor object
 * @param type The type of the new event to be posted
 */
static void post_monitor_event (RmgMonitor *monitor, MonitorEventType type);

/**
 * @brief GSource prepare function
 */
static gboolean monitor_source_prepare (GSource *source, gint *timeout);

/**
 * @brief GSource dispatch function
 */
static gboolean monitor_source_dispatch (GSource *source, GSourceFunc callback, gpointer _monitor);

/**
 * @brief GSource callback function
 */
static gboolean monitor_source_callback (gpointer _monitor, gpointer _event);

/**
 * @brief GSource destroy notification callback function
 */
static void monitor_source_destroy_notify (gpointer _monitor);

/**
 * @brief Async queue destroy notification callback function
 */
static void monitor_queue_destroy_notify (gpointer _monitor);

/**
 * @brief Initialization status callback for a new mentry
 */
static void mentry_init_status (gpointer _mentry, gpointer _monitor, RmgStatus status);

/**
 * @brief Build proxy local
 */
static void monitor_build_proxy (RmgMonitor *monitor);

/**
 * @brief Read services local
 */
static void monitor_read_services (RmgMonitor *monitor);

/**
 * @brief Start relax timers
 */
static void monitor_start_relax_timers (RmgMonitor *monitor);

/**
 * @brief Add service if not exist
 */
static void add_service (RmgMonitor *monitor, const gchar *service_name, const gchar *object_path,
                         ServiceActiveState active_state, ServiceActiveSubstate active_substate);

/**
 * @brief Callback for a notify proxy entry
 */
static void call_notify_proxy_entry (gpointer _notify_object, gpointer _dbus_proxy);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs monitor_source_funcs = {
  monitor_source_prepare, NULL, monitor_source_dispatch, NULL, NULL, NULL,
};

static void
post_monitor_event (RmgMonitor *monitor, MonitorEventType type)
{
  RmgMonitorEvent *e = NULL;

  g_assert (monitor);

  e = g_new0 (RmgMonitorEvent, 1);
  e->type = type;

  g_async_queue_push (monitor->queue, e);
}

static gboolean
monitor_source_prepare (GSource *source, gint *timeout)
{
  RmgMonitor *monitor = (RmgMonitor *)source;

  RMG_UNUSED (timeout);

  return (g_async_queue_length (monitor->queue) > 0);
}

static gboolean
monitor_source_dispatch (GSource *source, GSourceFunc callback, gpointer _monitor)
{
  RmgMonitor *monitor = (RmgMonitor *)source;
  gpointer event = g_async_queue_try_pop (monitor->queue);

  RMG_UNUSED (callback);
  RMG_UNUSED (_monitor);

  if (event == NULL)
    return G_SOURCE_CONTINUE;

  return monitor->callback (monitor, event) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
monitor_source_callback (gpointer _monitor, gpointer _event)
{
  RmgMonitor *monitor = (RmgMonitor *)_monitor;
  RmgMonitorEvent *event = (RmgMonitorEvent *)_event;

  g_assert (monitor);
  g_assert (event);

  switch (event->type)
    {
    case MONITOR_EVENT_BUILD_PROXY:
      monitor_build_proxy (monitor);
      break;

    case MONITOR_EVENT_READ_SERVICES:
      monitor_read_services (monitor);
      monitor_start_relax_timers (monitor);
      break;

    default:
      break;
    }

  g_free (event);

  return TRUE;
}

static void
monitor_source_destroy_notify (gpointer _monitor)
{
  RmgMonitor *monitor = (RmgMonitor *)_monitor;

  g_assert (monitor);
  g_debug ("Monitor destroy notification");

  rmg_monitor_unref (monitor);
}

static void
monitor_queue_destroy_notify (gpointer _monitor)
{
  RMG_UNUSED (_monitor);
  g_debug ("Monitor queue destroy notification");
}

gint
find_service_by_name (gconstpointer _entry, gconstpointer _service_name)
{
  const RmgMEntry *entry = (const RmgMEntry *)_entry;
  const gchar *service_name = (const gchar *)_service_name;

  if (g_strcmp0 (entry->service_name, service_name) == 0)
    return 0;

  return 1;
}

static void
on_manager_signal (GDBusProxy *proxy, gchar *sender_name, gchar *signal_name, GVariant *parameters,
                   gpointer user_data)
{
  RmgMonitor *monitor = (RmgMonitor *)user_data;

  RMG_UNUSED (proxy);
  RMG_UNUSED (sender_name);

  if (g_strcmp0 (signal_name, "UnitNew") == 0)
    {
      const gchar *service_name = NULL;
      const gchar *object_path = NULL;

      g_variant_get (parameters, "(so)", &service_name, &object_path);

      if ((service_name != NULL) && (object_path != NULL))
        {
          add_service (monitor, service_name, object_path, SERVICE_STATE_INACTIVE,
                       SERVICE_SUBSTATE_DEAD);
        }
      else
        g_warning ("Fail to read date on UnitNew signal");
    }
}

static void
call_notify_proxy_entry (gpointer _notify_object, gpointer _dbus_proxy)

{
  RmgMonitorNotifyProxy *notify_object = (RmgMonitorNotifyProxy *)_notify_object;
  GDBusProxy *proxy = (GDBusProxy *)_dbus_proxy;

  g_assert (notify_object);
  g_assert (proxy);

  if (notify_object->callback != NULL)
    notify_object->callback (proxy, notify_object->data);

  /* we don't free the notify object since will be freed with monitor */
}

static void
monitor_build_proxy (RmgMonitor *monitor)
{
  g_autoptr (GError) error = NULL;

  g_assert (monitor);

  monitor->proxy = g_dbus_proxy_new_for_bus_sync (
      G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, NULL,                   /* GDBusInterfaceInfo */
      sd_dbus_name, sd_dbus_object_path, sd_dbus_interface_manager, NULL, /* GCancellable */
      &error);

  if (error != NULL)
    g_warning ("Fail to build proxy for Manager. Error %s", error->message);
  else
    {
      g_signal_connect (monitor->proxy, "g-signal", G_CALLBACK (on_manager_signal), monitor);

      g_list_foreach (monitor->notify_proxy, call_notify_proxy_entry, monitor->proxy);
    }
}

GDBusProxy *
rmg_monitor_get_manager_proxy (RmgMonitor *monitor)
{
  g_assert (monitor);
  return monitor->proxy;
}

static void
mentry_init_status (gpointer _mentry, gpointer _monitor, RmgStatus status)
{
  RmgMonitor *monitor = (RmgMonitor *)_monitor;
  RmgMEntry *mentry = (RmgMEntry *)_mentry;

  if (status != RMG_STATUS_OK)
    {
      g_warning ("Fail to initialized mentry for service '%s'", mentry->service_name);
      rmg_mentry_unref (mentry);
    }
  else
    {
      g_info ("Monitoring unit='%s' path='%s'", mentry->service_name, mentry->object_path);
      monitor->services = g_list_prepend (monitor->services, mentry);
    }
}

static void
add_service (RmgMonitor *monitor, const gchar *service_name, const gchar *object_path,
             ServiceActiveState active_state, ServiceActiveSubstate active_substate)
{
  GList *check_existent = NULL;

  g_assert (monitor);
  g_assert (service_name);
  g_assert (object_path);

  if (!g_str_has_suffix (service_name, ".service"))
    return;

  check_existent = g_list_find_custom (monitor->services, service_name, find_service_by_name);

  if (check_existent == NULL)
    {
      RmgMEntry *entry = rmg_mentry_new (service_name, object_path, active_state, active_substate);

      rmg_mentry_set_manager_proxy (entry, rmg_monitor_get_manager_proxy (monitor));
      rmg_mentry_build_proxy_async (entry, monitor->dispatcher, mentry_init_status,
                                    (gpointer)monitor);
    }
}

static void
monitor_read_services (RmgMonitor *monitor)
{
  g_autoptr (GVariant) service_list = NULL;
  g_autoptr (GError) error = NULL;

  g_assert (monitor);

  if (monitor->proxy == NULL)
    {
      g_warning ("Monitor proxy not available for service units read");
      return;
    }

  service_list = g_dbus_proxy_call_sync (monitor->proxy, "ListUnits", NULL, G_DBUS_CALL_FLAGS_NONE,
                                         -1, NULL, &error);
  if (error != NULL)
    g_warning ("Fail to call ListUnits on Manager proxy. Error %s", error->message);
  else
    {
      const gchar *unitname = NULL;
      const gchar *description = NULL;
      const gchar *loadstate = NULL;
      const gchar *activestate = NULL;
      const gchar *substate = NULL;
      const gchar *followedby = NULL;
      const gchar *objectpath = NULL;
      const gchar *jobtype = NULL;
      const gchar *jobobjectpath = NULL;
      glong queuedjobs = 0;
      g_autoptr (GVariantIter) iter = NULL;

      g_variant_get (service_list, "(a(ssssssouso))", &iter);

      while (g_variant_iter_loop (iter, "(ssssssouso)", &unitname, &description, &loadstate,
                                  &activestate, &substate, &followedby, &objectpath, &queuedjobs,
                                  &jobtype, &jobobjectpath))
        {
          if (unitname != NULL && objectpath != NULL && activestate != NULL && substate != NULL)
            {
              add_service (monitor, unitname, objectpath,
                           rmg_mentry_active_state_from (activestate),
                           rmg_mentry_active_substate_from (substate));
            }
        }
    }
}

static void
monitor_add_relax_timer (gpointer _journal, gpointer _data)
{
  RmgJournal *journal = (RmgJournal *)_journal;
  const gchar *service_name = (const gchar *)_data;

  g_autoptr (GError) error = NULL;

  rmg_relaxtimer_trigger (journal, service_name, &error);
  if (error != 0)
    {
      g_warning ("Fail to trigger relaxtimer for service %s. Error %s", service_name,
                 error->message);
    }
}

static void
monitor_start_relax_timers (RmgMonitor *monitor)
{
  g_autoptr (GError) error = NULL;

  g_assert (monitor);

  rmg_journal_call_foreach_relaxing (monitor->dispatcher->journal, monitor_add_relax_timer, &error);

  if (error != 0)
    g_warning ("Fail to start relax timers. Error %s", error->message);
}

static void
remove_service_entry (gpointer _entry)
{
  rmg_mentry_unref ((RmgMEntry *)_entry);
}

static void
remove_notify_proxy_entry (gpointer _entry)
{
  g_free (_entry);
}

RmgMonitor *
rmg_monitor_new (RmgDispatcher *dispatcher)
{
  RmgMonitor *monitor = (RmgMonitor *)g_source_new (&monitor_source_funcs, sizeof (RmgMonitor));

  g_assert (monitor);
  g_assert (dispatcher);

  g_ref_count_init (&monitor->rc);

  monitor->dispatcher = rmg_dispatcher_ref (dispatcher);
  monitor->queue = g_async_queue_new_full (monitor_queue_destroy_notify);
  monitor->callback = monitor_source_callback;

  g_source_set_callback (RMG_EVENT_SOURCE (monitor), NULL, monitor, monitor_source_destroy_notify);
  g_source_attach (RMG_EVENT_SOURCE (monitor), NULL);

  return monitor;
}

RmgMonitor *
rmg_monitor_ref (RmgMonitor *monitor)
{
  g_assert (monitor);
  g_ref_count_inc (&monitor->rc);
  return monitor;
}

void
rmg_monitor_unref (RmgMonitor *monitor)
{
  g_assert (monitor);

  if (g_ref_count_dec (&monitor->rc) == TRUE)
    {
      if (monitor->dispatcher != NULL)
        rmg_dispatcher_unref (monitor->dispatcher);

      if (monitor->proxy != NULL)
        g_object_unref (monitor->proxy);

      g_async_queue_unref (monitor->queue);
      g_list_free_full (monitor->services, remove_service_entry);
      g_list_free_full (monitor->notify_proxy, remove_notify_proxy_entry);
      g_source_unref (RMG_EVENT_SOURCE (monitor));
    }
}

void
rmg_monitor_build_proxy (RmgMonitor *monitor)
{
  post_monitor_event (monitor, MONITOR_EVENT_BUILD_PROXY);
}

void
rmg_monitor_read_services (RmgMonitor *monitor)
{
  post_monitor_event (monitor, MONITOR_EVENT_READ_SERVICES);
}

void
rmg_monitor_register_proxy_available_callback (RmgMonitor *monitor,
                                               RmgMonitorProxyAvailableCallback callback,
                                               gpointer data)
{
  RmgMonitorNotifyProxy *notify = NULL;

  g_assert (monitor);
  g_assert (callback);
  g_assert (data);

  notify = g_new0 (RmgMonitorNotifyProxy, 1);

  notify->callback = callback;
  notify->data = data;

  monitor->notify_proxy = g_list_prepend (monitor->notify_proxy, notify);
}
