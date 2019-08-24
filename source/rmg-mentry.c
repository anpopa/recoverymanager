/* rmg-mentry.c
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

#include "rmg-mentry.h"

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

extern const gchar *sd_dbus_name;
extern const gchar *sd_dbus_object_path;
extern const gchar *sd_dbus_interface_unit;
extern const gchar *sd_dbus_interface_manager;

static void
on_properties_changed (GDBusProxy          *proxy,
                       GVariant            *changed_properties,
                       const gchar* const  *invalidated_properties,
                       gpointer user_data)
{
  RmgMEntry *mentry = (RmgMEntry *)user_data;

  /* Note that we are guaranteed that changed_properties and
   * invalidated_properties are never NULL
   */

  RMG_UNUSED (proxy);
  RMG_UNUSED (invalidated_properties);

  if (g_variant_n_children (changed_properties) > 0)
    {
      g_autofree gchar *new_active_state = NULL;
      g_autofree gchar *new_sub_state = NULL;
      GVariantIter *iter;
      const gchar *key;
      GVariant *value;

      g_variant_get (changed_properties,
                     "a{sv}",
                     &iter);

      while (g_variant_iter_loop (iter, "{&sv}", &key, &value))
        {
          if (g_strcmp0 (key, "ActiveState") == 0)
            new_active_state = g_variant_print (value, TRUE);
          else if (g_strcmp0 (key, "SubState") == 0)
            new_sub_state = g_variant_print (value, TRUE);
        }

      if (mentry->active_state != rmg_mentry_active_state_from (new_active_state)
          || mentry->active_substate != rmg_mentry_active_substate_from (new_sub_state))
        {
          mentry->active_state = rmg_mentry_active_state_from (new_active_state);
          mentry->active_substate = rmg_mentry_active_substate_from (new_sub_state);

          g_info ("Service '%s' state change to ActiveState='%s' SubState='%s'",
                  mentry->service_name,
                  new_active_state,
                  new_sub_state);
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
rmg_mentry_new (const gchar *service_name, const gchar *object_path);
{
  RmgMEntry *mentry = g_new0 (RmgMEntry, 1);

  g_ref_count_init (&mentry->rc);

  mentry->service_name = g_strdup (service_name);
  mentry->object_path = g_strdup (object_path);

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

      g_free (mentry);
    }
}

RmgStatus
rmg_mentry_build_proxy (RmgMEntry *mentry, gpointer _dispatcher)
{
  g_autoptr (GError) error = NULL;

  g_assert (mentry);
  g_assert (_dispatcher);

  mentry->dispatcher = _dispatcher;
  mentry->proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                 G_DBUS_PROXY_FLAGS_NONE,
                                                 NULL, /* GDBusInterfaceInfo */
                                                 sd_dbus_name,
                                                 mentry->object_path,
                                                 sd_dbus_interface_unit,
                                                 NULL, /* GCancellable */
                                                 &error);
  if (error != NULL)
    {
      g_warning ("Fail to build proxy for mentry='%s'. Error %s",
                 mentry->service_name,
                 error->message);

      return RMG_STATUS_ERROR;
    }

  g_signal_connect (mentry->proxy,
                    "g-properties-changed",
                    G_CALLBACK (on_properties_changed),
                    mentry);

  return RMG_STATUS_OK;
}
