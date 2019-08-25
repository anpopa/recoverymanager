/* rmg-monitor.c
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

#include "rmg-monitor.h"
#include "rmg-mentry.h"

const gchar *sd_dbus_name = "org.freedesktop.systemd1";
const gchar *sd_dbus_object_path = "/org/freedesktop/systemd1";
const gchar *sd_dbus_interface_unit = "org.freedesktop.systemd1.Unit";
const gchar *sd_dbus_interface_manager = "org.freedesktop.systemd1.Manager";

/**
 * @brief Post new event
 *
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
 * @brief Build proxy local
 */
static void monitor_build_proxy (RmgMonitor *monitor);

/**
 * @brief Read services local
 */
static void monitor_read_services (RmgMonitor *monitor);

/**
 * @brief Add service if not exist
 */
static void add_service (RmgMonitor *monitor, const gchar *service_name, const gchar *object_path);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs monitor_source_funcs =
{
  monitor_source_prepare,
  NULL,
  monitor_source_dispatch,
  NULL,
  NULL,
  NULL,
};

static void
post_monitor_event (RmgMonitor *monitor,
                    MonitorEventType type)
{
  RmgMonitorEvent *e = NULL;

  g_assert (monitor);

  e = g_new0 (RmgMonitorEvent, 1);
  e->type = type;

  g_async_queue_push (monitor->queue, e);
}

static gboolean
monitor_source_prepare (GSource *source,
                        gint *timeout)
{
  RmgMonitor *monitor = (RmgMonitor *)source;

  RMG_UNUSED (timeout);

  return(g_async_queue_length (monitor->queue) > 0);
}

static gboolean
monitor_source_dispatch (GSource *source,
                         GSourceFunc callback,
                         gpointer _monitor)
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
monitor_source_callback (gpointer _monitor,
                         gpointer _event)
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
find_service_by_name (gconstpointer _entry,
                      gconstpointer _service_name)
{
  const RmgMEntry *entry = (const RmgMEntry *)_entry;
  const gchar *service_name = (const gchar *)_service_name;

  if (g_strcmp0 (entry->service_name, service_name) == 0)
    return 0;

  return 1;
}

static void
on_manager_signal (GDBusProxy *proxy,
                   gchar      *sender_name,
                   gchar      *signal_name,
                   GVariant   *parameters,
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
        add_service (monitor, service_name, object_path);
      else
        g_warning ("Fail to read date on UnitNew signal");
    }
}


static void
monitor_build_proxy (RmgMonitor *monitor)
{
  g_autoptr (GError) error = NULL;

  g_assert (monitor);

  monitor->proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                  G_DBUS_PROXY_FLAGS_NONE,
                                                  NULL, /* GDBusInterfaceInfo */
                                                  sd_dbus_name,
                                                  sd_dbus_object_path,
                                                  sd_dbus_interface_manager,
                                                  NULL, /* GCancellable */
                                                  &error);
  if (error != NULL)
    {
      g_warning ("Fail to build proxy for Manager. Error %s",
                 error->message);
    }
  else
    {
      g_signal_connect (monitor->proxy,
                        "g-signal",
                        G_CALLBACK (on_manager_signal),
                        monitor);
    }
}

static void
add_service (RmgMonitor *monitor,
             const gchar *service_name,
             const gchar *object_path)
{
  GList *check_existent = NULL;

  g_assert (monitor);
  g_assert (service_name);
  g_assert (object_path);

  if (!g_str_has_suffix (service_name, ".service"))
    return;

  check_existent = g_list_find_custom (monitor->services,
                                       service_name,
                                       find_service_by_name);

  if (check_existent == NULL)
    {
      RmgMEntry *entry = rmg_mentry_new (service_name, object_path);

      if (rmg_mentry_build_proxy (entry, monitor->dispatcher) != RMG_STATUS_OK)
        {
          g_warning ("Fail to build proxy for new service '%s'", service_name);
          rmg_mentry_unref (entry);
        }
      else
        {
          monitor->services = g_list_prepend (monitor->services, entry);
        }
    }
}

static void
monitor_read_services (RmgMonitor *monitor)
{
  g_autoptr (GError) error = NULL;
  GVariant *service_list;

  g_assert (monitor);

  if (monitor->proxy == NULL)
    {
      g_warning ("Monitor proxy not available for service units read");
      g_return_if_reached ();
    }

  service_list = g_dbus_proxy_call_sync (monitor->proxy,
                                         "ListUnits",
                                         NULL,
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1,
                                         NULL,
                                         &error);
  if (error != NULL)
    {
      g_warning ("Fail to call ListUnits on Manager proxy. Error %s",
                 error->message);
    }
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

      while (g_variant_iter_loop (iter,
                                  "(ssssssouso)",
                                  &unitname,
                                  &description,
                                  &loadstate,
                                  &activestate,
                                  &substate,
                                  &followedby,
                                  &objectpath,
                                  &queuedjobs,
                                  &jobtype,
                                  &jobobjectpath))
        {
          if (unitname != NULL && objectpath != NULL)
            add_service (monitor, unitname, objectpath);
        }
    }
}

RmgMonitor *
rmg_monitor_new (RmgDispatcher *dispatcher)
{
  RmgMonitor *monitor = (RmgMonitor *)g_source_new (&monitor_source_funcs,
                                                    sizeof(RmgMonitor));

  g_assert (monitor);
  g_assert (dispatcher);

  g_ref_count_init (&monitor->rc);

  monitor->dispatcher = rmg_dispatcher_ref (dispatcher);
  monitor->queue = g_async_queue_new_full (monitor_queue_destroy_notify);
  monitor->callback = monitor_source_callback;

  g_source_set_callback (RMG_EVENT_SOURCE (monitor),
                         NULL,
                         monitor,
                         monitor_source_destroy_notify);
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

      g_async_queue_unref (monitor->queue);
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
