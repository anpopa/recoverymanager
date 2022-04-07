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
 * \file rmg-client.h
 */

#pragma once

#include "rmg-journal.h"
#include "rmg-message.h"
#include "rmg-types.h"

#include <glib.h>

G_BEGIN_DECLS

#ifndef CLIENT_SELECT_TIMEOUT
#define CLIENT_SELECT_TIMEOUT 3
#endif

/**
 * @struct RmgClient
 * @brief The RmgClient data structure
 */
typedef struct _RmgClient
{
  GSource source;      /**< Event loop source */
  gpointer tag;        /**< Unix server socket tag  */
  grefcount rc;        /**< Reference counter variable  */
  gint sockfd;         /**< Module file descriptor (client fd) */
  gchar *context_name; /**< Client context name */
  gpointer dispatcher; /**< Optional reference to dispatcher */
  gpointer server;     /**< Optional reference to server */
} RmgClient;

/*
 * @brief Create a new client object
 * @param clientfd Socket file descriptor accepted by the server
 * @param dispatcher A pointer to the RmgDispatcher object created by the main
 * application
 * @return On success return a new RmgClient object otherwise return NULL
 */
RmgClient *rmg_client_new (gint clientfd, gpointer dispatcher);

/**
 * @brief Aquire client object
 * @param client Pointer to the client object
 * @return The referenced client object
 */
RmgClient *rmg_client_ref (RmgClient *client);

/**
 * @brief Release client object
 * @param client Pointer to the client object
 */
void rmg_client_unref (RmgClient *client);

/**
 * @brief Set server reference
 * @param server Pointer to the server object
 * @return The referenced client object
 */
void rmg_client_set_server_ref (RmgClient *client, gpointer server);

/**
 * @brief Send message to client
 * @param client Client object
 * @param msg Message to send
 * @return True if connected
 */
RmgStatus rmg_client_send (RmgClient *client, RmgMessage *msg);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgClient, rmg_client_unref);

G_END_DECLS
