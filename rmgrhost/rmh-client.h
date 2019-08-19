/* rmh-client.h
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
#include "rmg-message.h"
#include "rmg-journal.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @struct RmhClient
 * @brief The RmhClient opaque data structure
 */
typedef struct _RmhClient {
  GSource source;  /**< Event loop source */
  gpointer tag;     /**< Unix server socket tag  */
  grefcount rc;     /**< Reference counter variable  */
  gint sockfd;      /**< Module file descriptor (client fd) */
  guint64 id;       /**< Client instance id */

  RmgJournal *journal; /**< Own a reference to the journal object */
} RmhClient;

/*
 * @brief Create a new client object
 * @param clientfd Socket file descriptor accepted by the server
 * @param journal A pointer to the RmgJournal object created by the main
 * application
 * @return On success return a new RmhClient object otherwise return NULL
 */
RmhClient *rmh_client_new (gint clientfd, RmgJournal *journal);

/**
 * @brief Aquire client object
 * @param client Pointer to the client object
 * @return The referenced client object
 */
RmhClient *rmh_client_ref (RmhClient *client);

/**
 * @brief Release client object
 * @param client Pointer to the client object
 */
void rmh_client_unref (RmhClient *client);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmhClient, rmh_client_unref);

G_END_DECLS
