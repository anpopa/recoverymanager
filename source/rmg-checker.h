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
 * \file rmg-checker.h
 */

#pragma once

#include "rmg-dispatcher.h"
#include "rmg-types.h"

#include <gio/gio.h>
#include <glib.h>

G_BEGIN_DECLS

/**
 * @enum checker_event_data
 * @brief Checker event data type
 */
typedef enum _CheckerEventType { CHECKER_EVENT_CHECK_SERVICES } CheckerEventType;

typedef gboolean (*RmgCheckerCallback)(gpointer _checker, gpointer _event);

/**
 * @struct RmgCheckerEvent
 * @brief The file transfer event
 */
typedef struct _RmgCheckerEvent {
    CheckerEventType type; /**< The event type the element holds */
} RmgCheckerEvent;

/**
 * @struct RmgChecker
 * @brief The RmgChecker opaque data structure
 */
typedef struct _RmgChecker {
    GSource source;              /**< Event loop source */
    GAsyncQueue *queue;          /**< Async queue */
    RmgCheckerCallback callback; /**< Callback function */
    grefcount rc;                /**< Reference counter variable  */
    RmgOptions *options;
    RmgJournal *journal;
    GDBusProxy *proxy;
} RmgChecker;

/*
 * @brief Create a new checker object
 * @return On success return a new RmgChecker object otherwise return NULL
 */
RmgChecker *rmg_checker_new(RmgJournal *journal, RmgOptions *options);

/**
 * @brief Aquire checker object
 * @param checker Pointer to the checker object
 */
RmgChecker *rmg_checker_ref(RmgChecker *checker);

/**
 * @brief Release checker object
 * @param checker Pointer to the checker object
 */
void rmg_checker_unref(RmgChecker *checker);

/**
 * @brief Build DBus proxy
 * @param checker Pointer to the checker object
 */
void rmg_checker_set_proxy(RmgChecker *checker, GDBusProxy *dbus_proxy);

/**
 * @brief Service integrity checker
 * @param checker Pointer to the checker object
 */
void rmg_checker_check_services(RmgChecker *checker);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(RmgChecker, rmg_checker_unref);

G_END_DECLS
