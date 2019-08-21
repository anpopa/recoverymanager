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

/**
 * @brief Post new event
 *
 * @param monitor A pointer to the monitor object
 * @param type The type of the new event to be posted
 * @param service_name The service name having this event
 */
static void post_monitor_event (RmgMonitor *monitor, MonitorEventType type, const gchar *service_name);

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
                    MonitorEventType type,
                    const gchar *service_name)
{
  RmgMonitorEvent *e = NULL;

  g_assert (monitor);
  g_assert (service_name);

  e = g_new0 (RmgMonitorEvent, 1);

  e->type = type;
  e->service_name = g_strdup (service_name);

  g_async_queue_push (monitor->queue, e);
}

static gboolean
monitor_source_prepare (GSource *source,
                        gint *timeout)
{
  RmgMonitor *monitor = (RmgMonitor *)source;

  CDM_UNUSED (timeout);

  return(g_async_queue_length (monitor->queue) > 0);
}

static gboolean
monitor_source_dispatch (GSource *source,
                         GSourceFunc callback,
                         gpointer _monitor)
{
  RmgMonitor *monitor = (RmgMonitor *)source;
  gpointer event = g_async_queue_try_pop (monitor->queue);

  CDM_UNUSED (callback);
  CDM_UNUSED (_monitor);

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


  /* TODO: Process the event */

  g_free (event->service_name);
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
  CDM_UNUSED (_monitor);
  g_debug ("Monitor queue destroy notification");
}

RmgMonitor *
rmg_monitor_new (RmgDispatcher *dispatcher, GError **error)
{
  RmgMonitor *monitor = (RmgMonitor *)g_source_new (&monitor_source_funcs, sizeof(RmgMonitor));

  g_assert (monitor);
  g_assert (dispatcher);

  g_ref_count_init (&monitor->rc);

  /* TODO: Do initialization */
  RMG_UNUSED (error);

  /* no error is set and no error should be set after this point */
  monitor->dispatcher = rmg_dispatcher_ref (dispatcher);
  monitor->queue = g_async_queue_new_full (monitor_queue_destroy_notify);

  g_source_set_callback (CDM_EVENT_SOURCE (monitor),
                         NULL,
                         monitor,
                         monitor_source_destroy_notify);
  g_source_attach (CDM_EVENT_SOURCE (monitor), NULL);

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
      g_source_unref (CDM_EVENT_SOURCE (monitor));
    }
}

