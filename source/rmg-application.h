/* rmg-application.h
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

#pragma once

#include "rmg-defaults.h"
#include "rmg-types.h"
#include "rmg-logging.h"
#include "rmg-options.h"
#include "rmg-journal.h"
#include "rmg-sdnotify.h"
#include "rmg-monitor.h"
#include "rmg-dispatcher.h"
#include "rmg-executor.h"

#include <glib.h>
#include <stdlib.h>

G_BEGIN_DECLS

/**
 * @struct RmgApplication
 * @brief Crashmanager application object referencing main objects
 */
typedef struct _RmgApplication {
  RmgOptions *options;
  RmgJournal *journal;
  RmgSDNotify *sdnotify;
  RmgMonitor *monitor;
  RmgDispatcher *dispatcher;
  RmgExecutor *executor;
  GMainLoop *mainloop;
  grefcount rc;           /**< Reference counter variable  */
} RmgApplication;

/**
 * @brief Create a new RmgApplication object
 * @param config Full path to the configuration file
 * @param error An error object must be provided. If an error occurs during
 * initialization the error is reported and application should not use the
 * returned object. If the error is set the object is invalid and needs to be
 * released.
 */
RmgApplication *rmg_application_new (const gchar *config, GError **error);

/**
 * @brief Aquire RmgApplication object
 * @param app The object to aquire
 * @return The aquiered app object
 */
RmgApplication *rmg_application_ref (RmgApplication *app);

/**
 * @brief Release a RmgApplication object
 * @param app The rmg application object to release
 */
void rmg_application_unref (RmgApplication *app);

/**
 * @brief Execute rmg application
 * @param app The rmg application object
 * @return If run was succesful CDH_OK is returned
 */
RmgStatus rmg_application_execute (RmgApplication *app);

/**
 * @brief Get main event loop reference
 * @param app The rmg application object
 * @return A pointer to the main event loop
 */
GMainLoop *rmg_application_get_mainloop (RmgApplication *app);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgApplication, rmg_application_unref);

G_END_DECLS
