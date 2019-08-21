/* rmg-server.h
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

#include "rmg-types.h"
#include "rmg-options.h"
#include "rmg-journal.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @struct RmgServer
 * @brief The RmgServer opaque data structure
 */
typedef struct _RmgServer {
  GSource source;  /**< Event loop source */
  grefcount rc;     /**< Reference counter variable  */
  gpointer tag;     /**< Unix server socket tag  */
  gint sockfd;      /**< Module file descriptor (server listen fd) */
  RmgOptions *options; /**< Own reference to global options */
  gpointer *dispatcher; /**< A pointer to dispatcher. Not owned and optional */
} RmgServer;

/*
 * @brief Create a new server object
 *
 * @param options A pointer to the RmgOptions object created by the main
 * application
 * @param transfer A pointer to the RmgTransfer object created by the main
 * application
 * @param journal A pointer to the RmgJournal object created by the main
 * application
 *
 * @return On success return a new RmgServer object otherwise return NULL
 */
RmgServer *rmg_server_new (RmgOptions *options, gpointer dispatcher, GError **error);

/**
 * @brief Aquire server object
 * @param server Pointer to the server object
 * @return The referenced server object
 */
RmgServer *rmg_server_ref (RmgServer *server);

/**
 * @brief Start the server an listen for clients
 * @param server Pointer to the server object
 * @return If server starts listening the function return RMG_STATUS_OK
 */
RmgStatus rmg_server_bind_and_listen (RmgServer *server);

/**
 * @brief Release server object
 * @param server Pointer to the server object
 */
void rmg_server_unref (RmgServer *server);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgServer, rmg_server_unref);

G_END_DECLS
