/* rmg-devent.c
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

#include "rmg-devent.h"

RmgDEvent *
rmg_devent_new (DispatcherEventType type)
{
  RmgDEvent *event = g_new0 (RmgDEvent, 1);

  event->type = type;
  g_ref_count_init (&event->rc);

  return event;
}

RmgDEvent *
rmg_devent_ref (RmgDEvent *event)
{
  g_assert (event);
  g_ref_count_inc (&event->rc);
  return event;
}

void
rmg_devent_unref (RmgDEvent *event)
{
  g_assert (event);

  if (g_ref_count_dec (&event->rc) == TRUE)
    {
      if (event->service_name != NULL)
        g_free (event->service_name);

      if (event->object_path != NULL)
        g_free (event->object_path);

      if (event->context_name != NULL)
        g_free (event->context_name);

      if (event->manager_proxy != NULL)
        g_object_unref (event->manager_proxy);

      g_free (event);
    }
}

void
rmg_devent_set_type (RmgDEvent *event, DispatcherEventType type)
{
  g_assert (event);
  event->type = type;
}

void
rmg_devent_set_service_name (RmgDEvent *event,
                             const gchar *service_name)
{
  g_assert (event);
  event->service_name = g_strdup (service_name);
}

void
rmg_devent_set_object_path (RmgDEvent *event,
                            const gchar *object_path)
{
  g_assert (event);
  event->object_path = g_strdup (object_path);
}

void
rmg_devent_set_context_name (RmgDEvent *event,
                             const gchar *context_name)
{
  g_assert (event);
  event->context_name = g_strdup (context_name);
}

void
rmg_devent_set_manager_proxy (RmgDEvent *event,
                              GDBusProxy *manager_proxy)
{
  g_assert (event);
  event->manager_proxy = g_object_ref (manager_proxy);
}
