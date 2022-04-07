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
 * \file rmg-checker.c
 */

#include "rmg-checker.h"

/**
 * @brief Post new event
 * @param checker A pointer to the checker object
 * @param type The type of the new event to be posted
 */
static void post_checker_event (RmgChecker *checker, CheckerEventType type);

/**
 * @brief GSource prepare function
 */
static gboolean checker_source_prepare (GSource *source, gint *timeout);

/**
 * @brief GSource dispatch function
 */
static gboolean checker_source_dispatch (GSource *source, GSourceFunc callback, gpointer _checker);

/**
 * @brief GSource callback function
 */
static gboolean checker_source_callback (gpointer _checker, gpointer _event);

/**
 * @brief GSource destroy notification callback function
 */
static void checker_source_destroy_notify (gpointer _checker);

/**
 * @brief Async queue destroy notification callback function
 */
static void checker_queue_destroy_notify (gpointer _checker);

/**
 * @brief Start check services timer
 */
static void checker_start_check_services_timer (RmgChecker *checker);

/**
 * @brief Check services timer callback
 */
static gboolean check_services_timer_callback (gpointer user_data);

/**
 * @brief Check services callback for matching service
 */
static void check_services_callback_for_service (gpointer _journal, gpointer _data);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs checker_source_funcs = {
  checker_source_prepare, NULL, checker_source_dispatch, NULL, NULL, NULL,
};

static void
post_checker_event (RmgChecker *checker, CheckerEventType type)
{
  RmgCheckerEvent *e = NULL;

  g_assert (checker);

  e = g_new0 (RmgCheckerEvent, 1);
  e->type = type;

  g_async_queue_push (checker->queue, e);
}

static gboolean
checker_source_prepare (GSource *source, gint *timeout)
{
  RmgChecker *checker = (RmgChecker *)source;

  RMG_UNUSED (timeout);

  return (g_async_queue_length (checker->queue) > 0);
}

static gboolean
checker_source_dispatch (GSource *source, GSourceFunc callback, gpointer _checker)
{
  RmgChecker *checker = (RmgChecker *)source;
  gpointer event = g_async_queue_try_pop (checker->queue);

  RMG_UNUSED (callback);
  RMG_UNUSED (_checker);

  if (event == NULL)
    return G_SOURCE_CONTINUE;

  return checker->callback (checker, event) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
checker_source_callback (gpointer _checker, gpointer _event)
{
  RmgChecker *checker = (RmgChecker *)_checker;
  RmgCheckerEvent *event = (RmgCheckerEvent *)_event;

  g_assert (checker);
  g_assert (event);

  switch (event->type)
    {
    case CHECKER_EVENT_CHECK_SERVICES:
      checker_start_check_services_timer (checker);
      break;

    default:
      break;
    }

  g_free (event);

  return TRUE;
}

static void
checker_source_destroy_notify (gpointer _checker)
{
  RmgChecker *checker = (RmgChecker *)_checker;

  g_assert (checker);
  g_debug ("Checker destroy notification");

  rmg_checker_unref (checker);
}

static void
checker_queue_destroy_notify (gpointer _checker)
{
  RMG_UNUSED (_checker);
  g_debug ("Checker queue destroy notification");
}

static void
check_services_callback_for_service (gpointer _journal, gpointer _data)
{
  RmgJournal *journal = (RmgJournal *)_journal;
  const char *service_name = (const char *)_data;

  RMG_UNUSED (journal);

  /* TODO: We may want to check if the service is running here and trigger
   *       a restart without increment the rvector to allow crash detaction
   *       mechanism to be triggered in case of failures */
  g_debug ("Service '%s' request integrity check", service_name);
}

static gboolean
check_services_timer_callback (gpointer user_data)
{
  RmgChecker *checker = (RmgChecker *)user_data;

  g_autoptr (GError) error = NULL;

  g_assert (checker);

  rmg_journal_call_foreach_checkstart (checker->journal, check_services_callback_for_service,
                                       &error);
  return false;
}

static void
checker_start_check_services_timer (RmgChecker *checker)
{
  glong timeout;

  g_assert (checker);

  timeout = (glong)rmg_options_long_for (checker->options, KEY_INTEGRITY_CHECK_SEC);
  g_timeout_add_seconds_full (G_PRIORITY_DEFAULT, (guint)timeout, check_services_timer_callback,
                              checker, NULL);
}

RmgChecker *
rmg_checker_new (RmgJournal *journal, RmgOptions *options)
{
  RmgChecker *checker = (RmgChecker *)g_source_new (&checker_source_funcs, sizeof (RmgChecker));

  g_assert (checker);
  g_assert (journal);
  g_assert (options);

  g_ref_count_init (&checker->rc);

  checker->options = rmg_options_ref (options);
  checker->journal = rmg_journal_ref (journal);
  checker->queue = g_async_queue_new_full (checker_queue_destroy_notify);
  checker->callback = checker_source_callback;

  g_source_set_callback (RMG_EVENT_SOURCE (checker), NULL, checker, checker_source_destroy_notify);
  g_source_attach (RMG_EVENT_SOURCE (checker), NULL);

  return checker;
}

RmgChecker *
rmg_checker_ref (RmgChecker *checker)
{
  g_assert (checker);
  g_ref_count_inc (&checker->rc);
  return checker;
}

void
rmg_checker_unref (RmgChecker *checker)
{
  g_assert (checker);

  if (g_ref_count_dec (&checker->rc) == TRUE)
    {
      if (checker->journal != NULL)
        rmg_journal_unref (checker->journal);

      if (checker->options != NULL)
        rmg_options_unref (checker->options);

      if (checker->proxy != NULL)
        g_object_unref (checker->proxy);

      g_async_queue_unref (checker->queue);
      g_source_unref (RMG_EVENT_SOURCE (checker));
    }
}

void
rmg_checker_set_proxy (RmgChecker *checker, GDBusProxy *dbus_proxy)
{
  g_assert (checker);
  g_assert (dbus_proxy);
  g_assert (!checker->proxy);

  g_debug ("Proxy available for checker");
  checker->proxy = g_object_ref (dbus_proxy);
}

void
rmg_checker_check_services (RmgChecker *checker)
{
  post_checker_event (checker, CHECKER_EVENT_CHECK_SERVICES);
}
