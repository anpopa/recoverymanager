/* rmg-manager.h
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

#include "rmg-message.h"
#include "rmg-options.h"
#include "rmg-types.h"

#include <glib.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

G_BEGIN_DECLS

#ifndef MANAGER_SELECT_TIMEOUT
#define MANAGER_SELECT_TIMEOUT 3
#endif

/**
 * @struct rmg_manager
 * @brief The coredump handler manager object
 */
typedef struct _RmgManager {
  grefcount rc;                /**< Reference counter variable  */
  gint sfd;                    /**< Server (manager) unix domain file descriptor */
  gboolean connected;          /**< Server connection state */
  struct sockaddr_un saddr;    /**< Server socket addr struct */
  RmgOptions *opts;            /**< Reference to options object */
} RmgManager;

/**
 * @brief Create a new RmgManager object
 * @param opts Pointer to global options object
 * @return A new RmgManager objects
 */
RmgManager *rmg_manager_new (RmgOptions *opts);

/**
 * @brief Aquire RmgManager object
 * @param c Manager object
 */
RmgManager *rmg_manager_ref (RmgManager *c);

/**
 * @brief Release RmgManager object
 * @param c Manager object
 */
void rmg_manager_unref (RmgManager *c);

/**
 * @brief Connect to rmg manager
 * @param c Manager object
 * @return RMG_STATUS_OK on success
 */
RmgStatus rmg_manager_connect (RmgManager *c);

/**
 * @brief Disconnect from rmg manager
 * @param c Manager object
 * @return RMG_STATUS_OK on success
 */
RmgStatus rmg_manager_disconnect (RmgManager *c);

/**
 * @brief Get connection state
 * @param c Manager object
 * @return True if connected
 */
gboolean rmg_manager_connected (RmgManager *c);

/**
 * @brief Send message to rmg manager
 *
 * @param c Manager object
 * @param m Message to send
 *
 * @return True if connected
 */
RmgStatus rmg_manager_send (RmgManager *c, RmgMessage *m);

G_END_DECLS