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
 * \file rmg-application.h
 */

#pragma once

#include "rmg-checker.h"
#include "rmg-defaults.h"
#include "rmg-dispatcher.h"
#include "rmg-executor.h"
#include "rmg-journal.h"
#include "rmg-logging.h"
#include "rmg-monitor.h"
#include "rmg-options.h"
#include "rmg-sdnotify.h"
#include "rmg-crashmonitor.h"
#include "rmg-types.h"

#include <glib.h>
#include <stdlib.h>

G_BEGIN_DECLS

/**
 * @struct RmgApplication
 * @brief Crashmanager application object
 */
typedef struct _RmgApplication {
  RmgOptions *options;
  RmgJournal *journal;
  RmgSDNotify *sdnotify;
  RmgMonitor *monitor;
  RmgChecker *checker;
  RmgDispatcher *dispatcher;
  RmgExecutor *executor;
  RmgCrashMonitor *crashmonitor;
  GMainLoop *mainloop;
  grefcount rc;
} RmgApplication;

/**
 * @brief Create a new RmgApplication object
 * @param config Full path to the configuration file
 * @param error An error object must be provided. If an error occurs during
 * initialization the error is reported and application should not use the
 * returned object. If the error is set the object is invalid and needs to be
 * released.
 */
RmgApplication *        rmg_application_new                 (const gchar *config,
                                                             GError **error);

/**
 * @brief Aquire RmgApplication object
 * @param app The object to aquire
 * @return The aquiered app object
 */
RmgApplication *        rmg_application_ref                 (RmgApplication *app);

/**
 * @brief Release RmgApplication object
 * @param app The rmg application object to release
 */
void                    rmg_application_unref               (RmgApplication *app);

/**
 * @brief Execute RmgApplication
 * @param app The RmgApplicationn object
 * @return If run was succesful RMG_STATUS_OK is returned
 */
RmgStatus               rmg_application_execute             (RmgApplication *app);

/**
 * @brief Get main event loop reference
 * @param app The RmgApplicationn object
 * @return A pointer to the main event loop
 */
GMainLoop *             rmg_application_get_mainloop        (RmgApplication *app);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgApplication, rmg_application_unref);

G_END_DECLS
