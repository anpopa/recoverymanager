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
 * \file rmg-server.h
 */

#pragma once

#include "rmg-journal.h"
#include "rmg-options.h"
#include "rmg-types.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @struct RmgServer
 * @brief The RmgServer opaque data structure
 */
typedef struct _RmgServer
{
  GSource source;      /**< Event loop source */
  grefcount rc;        /**< Reference counter variable  */
  gpointer tag;        /**< Unix server socket tag  */
  gint sockfd;         /**< Module file descriptor (server listen fd) */
  RmgOptions *options; /**< Own reference to global options */
  gpointer dispatcher; /**< A pointer to dispatcher. Not owned and optional */
  GList *clients;      /**< Active clients list */
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
 * @brief Add client reference
 * @param server Pointer to the server object
 * @param client Pointer to the client object
 */
void rmg_server_add_client (RmgServer *server, gpointer client);

/**
 * @brief Rem client reference
 * @param server Pointer to the server object
 * @param client Pointer to the client object
 */
void rmg_server_rem_client (RmgServer *server, gpointer client);

/**
 * @brief Get client reference
 * @param server Pointer to the server object
 * @param client Pointer to the client object
 */
gpointer rmg_server_get_client (RmgServer *server, const gchar *name);

/**
 * @brief Release server object
 * @param server Pointer to the server object
 */
void rmg_server_unref (RmgServer *server);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgServer, rmg_server_unref);

G_END_DECLS
