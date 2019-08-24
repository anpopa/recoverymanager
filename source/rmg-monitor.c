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

const gchar *active_state_names[] = {
    "unknown",
    "active",
    "reloading",
    "inactive",
    "failed",
    "activating",
    "deactivating",
    NULL
};

const gchar *active_substate_names[] = {
    "unknown",
    "running",
    "dead",
    "inactive",
    "stop-sigterm",
    NULL
};

const gchar *sd_dbus_name = "org.freedesktop.systemd1";
const gchar *sd_dbus_object_path = "/org/freedesktop/systemd1";
const gchar *sd_dbus_interface_unit = "org.freedesktop.systemd1.Unit";
const gchar *sd_dbus_interface_manager = "org.freedesktop.systemd1.Manager";

ServiceActiveState
rmg_monitor_active_state_from (const gchar *state_name)
{
  gint i = 0;

  while (active_state_names[i] != NULL)
    {
      if (g_strcmp0 (active_state_names[i], state_name) == 0)
        return (ServiceActiveState)i;

      i++:
    }

  return SERVICE_STATE_UNKNOWN;
}

ServiceActiveSubstate
rmg_monitor_active_substate_from (const gchar *substate_name)
{
  gint i = 0;

  while (active_substate_names[i] != NULL)
    {
      if (g_strcmp0 (active_substate_names[i], substate_name) == 0)
        return (ServiceActiveSubstate)i;

      i++:
    }

  return SERVICE_SUBSTATE_UNKNOWN;
}

RmgMonitor *
rmg_monitor_new (RmgDispatcher *dispatcher, GError **error)
{
  RmgMonitor *monitor = g_new0 (RmgMonitor, 1);

  g_assert (monitor);
  g_assert (dispatcher);

  g_ref_count_init (&monitor->rc);

  /* TODO: Do initialization */
  RMG_UNUSED (error);

  /* no error is set and no error should be set after this point */
  monitor->dispatcher = rmg_dispatcher_ref (dispatcher);

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

      g_source_unref (RMG_EVENT_SOURCE (monitor));
    }
}

