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
 * \file rmg-manager.h
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
typedef struct _RmgManager
{
  GSource source;           /**< Event loop source */
  gpointer tag;             /**< Unix server socket tag  */
  grefcount rc;             /**< Reference counter variable  */
  gint sockfd;              /**< Server (manager) unix domain file descriptor */
  gboolean connected;       /**< Server connection state */
  struct sockaddr_un saddr; /**< Server socket addr struct */
  RmgOptions *opts;         /**< Reference to options object */
  gpointer dispatcher;      /**< Optional reference to dispatcher */
} RmgManager;

/**
 * @brief Create a new RmgManager object
 * @param opts Pointer to global options object
 * @return A new RmgManager objects
 */
RmgManager *rmg_manager_new (RmgOptions *opts, gpointer dispatcher);

/**
 * @brief Aquire RmgManager object
 * @param manager Manager object
 */
RmgManager *rmg_manager_ref (RmgManager *manager);

/**
 * @brief Release RmgManager object
 * @param manager Manager object
 */
void rmg_manager_unref (RmgManager *manager);

/**
 * @brief Connect to rmg manager
 * @param manager Manager object
 * @return RMG_STATUS_OK on success
 */
RmgStatus rmg_manager_connect (RmgManager *manager);

/**
 * @brief Disconnect from rmg manager
 * @param manager Manager object
 * @return RMG_STATUS_OK on success
 */
RmgStatus rmg_manager_disconnect (RmgManager *manager);

/**
 * @brief Get connection state
 * @param manager Manager object
 * @return True if connected
 */
gboolean rmg_manager_connected (RmgManager *manager);

/**
 * @brief Send message to rmg manager
 * @param manager Manager object
 * @param m Message to send
 * @return True if connected
 */
RmgStatus rmg_manager_send (RmgManager *manager, RmgMessage *m);

G_END_DECLS
