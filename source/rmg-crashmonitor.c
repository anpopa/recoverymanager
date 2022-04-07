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
 * \file rmg-crashmonitor.c
 */

#include "rmg-crashmonitor.h"

const gchar *cdm_dbus_name = "ro.fxdata.crashmanager";
const gchar *cdm_dbus_object_path = "/ro/fxdata/crashmanager";
const gchar *cdm_dbus_interface_crashes = "ro.fxdata.crashmanager.Crashes";

/**
 * @brief Post new event
 * @param crashmonitor A pointer to the crashmonitor object
 * @param type The type of the new event to be posted
 */
static void post_crashmonitor_event (RmgCrashMonitor *crashmonitor, CrashMonitorEventType type);

/**
 * @brief GSource prepare function
 */
static gboolean crashmonitor_source_prepare (GSource *source, gint *timeout);

/**
 * @brief GSource dispatch function
 */
static gboolean crashmonitor_source_dispatch (GSource *source, GSourceFunc callback,
                                              gpointer _crashmonitor);

/**
 * @brief GSource callback function
 */
static gboolean crashmonitor_source_callback (gpointer _crashmonitor, gpointer _event);

/**
 * @brief GSource destroy notification callback function
 */
static void crashmonitor_source_destroy_notify (gpointer _crashmonitor);

/**
 * @brief Async queue destroy notification callback function
 */
static void crashmonitor_queue_destroy_notify (gpointer _crashmonitor);

/**
 * @brief Build proxy local
 */
static void crashmonitor_build_proxy (RmgCrashMonitor *crashmonitor);

/**
 * @brief Callback for a notify proxy entry
 */
static void call_notify_proxy_entry (gpointer _notify_object, gpointer _dbus_proxy);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs crashmonitor_source_funcs = {
  crashmonitor_source_prepare, NULL, crashmonitor_source_dispatch, NULL, NULL, NULL,
};

static void
post_crashmonitor_event (RmgCrashMonitor *crashmonitor, CrashMonitorEventType type)
{
  RmgCrashMonitorEvent *e = NULL;

  g_assert (crashmonitor);

  e = g_new0 (RmgCrashMonitorEvent, 1);
  e->type = type;

  g_async_queue_push (crashmonitor->queue, e);
}

static gboolean
crashmonitor_source_prepare (GSource *source, gint *timeout)
{
  RmgCrashMonitor *crashmonitor = (RmgCrashMonitor *)source;

  RMG_UNUSED (timeout);

  return (g_async_queue_length (crashmonitor->queue) > 0);
}

static gboolean
crashmonitor_source_dispatch (GSource *source, GSourceFunc callback, gpointer _crashmonitor)
{
  RmgCrashMonitor *crashmonitor = (RmgCrashMonitor *)source;
  gpointer event = g_async_queue_try_pop (crashmonitor->queue);

  RMG_UNUSED (callback);
  RMG_UNUSED (_crashmonitor);

  if (event == NULL)
    return G_SOURCE_CONTINUE;

  return crashmonitor->callback (crashmonitor, event) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
crashmonitor_source_callback (gpointer _crashmonitor, gpointer _event)
{
  RmgCrashMonitor *crashmonitor = (RmgCrashMonitor *)_crashmonitor;
  RmgCrashMonitorEvent *event = (RmgCrashMonitorEvent *)_event;

  g_assert (crashmonitor);
  g_assert (event);

  switch (event->type)
    {
    case CRASHMONITOR_EVENT_BUILD_PROXY:
      crashmonitor_build_proxy (crashmonitor);
      break;

    default:
      break;
    }

  g_free (event);

  return TRUE;
}

static void
crashmonitor_source_destroy_notify (gpointer _crashmonitor)
{
  RmgCrashMonitor *crashmonitor = (RmgCrashMonitor *)_crashmonitor;

  g_assert (crashmonitor);
  g_debug ("CrashMonitor destroy notification");

  rmg_crashmonitor_unref (crashmonitor);
}

static void
crashmonitor_queue_destroy_notify (gpointer _crashmonitor)
{
  RMG_UNUSED (_crashmonitor);
  g_debug ("CrashMonitor queue destroy notification");
}

static void
on_manager_signal (GDBusProxy *proxy, gchar *sender_name, gchar *signal_name, GVariant *parameters,
                   gpointer user_data)
{
  RmgCrashMonitor *crashmonitor = (RmgCrashMonitor *)user_data;

  RMG_UNUSED (proxy);
  RMG_UNUSED (sender_name);

  if (g_strcmp0 (signal_name, "NewCrash") == 0)
    {
      const gchar *proc_name = NULL;
      const gchar *proc_context = NULL;
      const gchar *proc_crashid = NULL;

      g_variant_get (parameters, "(sss)", &proc_name, &proc_context, &proc_crashid);

      if ((proc_name != NULL) && (proc_context != NULL) && (proc_crashid != NULL))
        {
          RmgDEvent *event = rmg_devent_new (DEVENT_INFORM_PROCESS_CRASH);

          rmg_devent_set_process_name (event, proc_name);
          rmg_devent_set_context_name (event, proc_context);

          g_debug ("Dispatch process crash information for %s in context %s, crashid %s", proc_name,
                   proc_context, proc_crashid);
          rmg_dispatcher_push_service_event (crashmonitor->dispatcher, event);
        }
      else
        g_warning ("Fail to read date on NewCrash signal");
    }
}

static void
call_notify_proxy_entry (gpointer _notify_object, gpointer _dbus_proxy)

{
  RmgCrashMonitorNotifyProxy *notify_object = (RmgCrashMonitorNotifyProxy *)_notify_object;
  GDBusProxy *proxy = (GDBusProxy *)_dbus_proxy;

  g_assert (notify_object);
  g_assert (proxy);

  if (notify_object->callback != NULL)
    notify_object->callback (proxy, notify_object->data);

  /* we don't free the notify object since will be freed with crashmonitor */
}

static void
crashmonitor_build_proxy (RmgCrashMonitor *crashmonitor)
{
  g_autoptr (GError) error = NULL;

  g_assert (crashmonitor);

  crashmonitor->proxy = g_dbus_proxy_new_for_bus_sync (
      G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, NULL, /* GDBusInterfaceInfo */
      cdm_dbus_name, cdm_dbus_object_path, cdm_dbus_interface_crashes, NULL, /* GCancellable */
      &error);

  if (error != NULL)
    g_warning ("Fail to build proxy for Manager. Error %s", error->message);
  else
    {
      g_signal_connect (crashmonitor->proxy, "g-signal", G_CALLBACK (on_manager_signal),
                        crashmonitor);

      g_list_foreach (crashmonitor->notify_proxy, call_notify_proxy_entry, crashmonitor->proxy);
    }
}

GDBusProxy *
rmg_crashmonitor_get_manager_proxy (RmgCrashMonitor *crashmonitor)
{
  g_assert (crashmonitor);
  return crashmonitor->proxy;
}

static void
remove_notify_proxy_entry (gpointer _entry)
{
  g_free (_entry);
}

RmgCrashMonitor *
rmg_crashmonitor_new (RmgDispatcher *dispatcher)
{
  RmgCrashMonitor *crashmonitor
      = (RmgCrashMonitor *)g_source_new (&crashmonitor_source_funcs, sizeof (RmgCrashMonitor));

  g_assert (crashmonitor);
  g_assert (dispatcher);

  g_ref_count_init (&crashmonitor->rc);
  crashmonitor->dispatcher = rmg_dispatcher_ref (dispatcher);
  crashmonitor->queue = g_async_queue_new_full (crashmonitor_queue_destroy_notify);
  crashmonitor->callback = crashmonitor_source_callback;

  g_source_set_callback (RMG_EVENT_SOURCE (crashmonitor), NULL, crashmonitor,
                         crashmonitor_source_destroy_notify);
  g_source_attach (RMG_EVENT_SOURCE (crashmonitor), NULL);

  return crashmonitor;
}

RmgCrashMonitor *
rmg_crashmonitor_ref (RmgCrashMonitor *crashmonitor)
{
  g_assert (crashmonitor);
  g_ref_count_inc (&crashmonitor->rc);
  return crashmonitor;
}

void
rmg_crashmonitor_unref (RmgCrashMonitor *crashmonitor)
{
  g_assert (crashmonitor);

  if (g_ref_count_dec (&crashmonitor->rc) == TRUE)
    {
      if (crashmonitor->dispatcher != NULL)
        rmg_dispatcher_unref (crashmonitor->dispatcher);

      if (crashmonitor->proxy != NULL)
        g_object_unref (crashmonitor->proxy);

      g_async_queue_unref (crashmonitor->queue);
      g_list_free_full (crashmonitor->notify_proxy, remove_notify_proxy_entry);
      g_source_unref (RMG_EVENT_SOURCE (crashmonitor));
    }
}

void
rmg_crashmonitor_build_proxy (RmgCrashMonitor *crashmonitor)
{
  post_crashmonitor_event (crashmonitor, CRASHMONITOR_EVENT_BUILD_PROXY);
}

void
rmg_crashmonitor_register_proxy_available_callback (RmgCrashMonitor *crashmonitor,
                                                    RmgCrashMonitorProxyAvailableCb callback,
                                                    gpointer data)
{
  RmgCrashMonitorNotifyProxy *notify = NULL;

  g_assert (crashmonitor);
  g_assert (callback);
  g_assert (data);

  notify = g_new0 (RmgCrashMonitorNotifyProxy, 1);

  notify->callback = callback;
  notify->data = data;

  crashmonitor->notify_proxy = g_list_prepend (crashmonitor->notify_proxy, notify);
}
