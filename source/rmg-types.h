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
 * \file rmg-type.h
 */

#pragma once

#include <elf.h>
#include <glib.h>

G_BEGIN_DECLS

#ifndef RMG_UNUSED
#define RMG_UNUSED(x) (void)(x)
#endif

#define RMG_EVENT_SOURCE(x) (GSource *)(x)

typedef enum _RmgStatus { RMG_STATUS_ERROR = -1, RMG_STATUS_OK } RmgStatus;

typedef enum _RmgRunMode { RUN_MODE_PRIMARY, RUN_MODE_REPLICA } RmgRunMode;

/**
 * @enum Action type
 * @brief The type of action top perform sorted from low to high destructive
 *   Actions with an ID higher or equal ACTION_CONTEXT_RESET will be performed
 *   by primary instance only.
 */
typedef enum _RmgActionType {
  ACTION_INVALID,
  ACTION_SERVICE_IGNORE,
  ACTION_SERVICE_RESET,
  ACTION_PUBLIC_DATA_RESET,
  ACTION_PRIVATE_DATA_RESET,
  ACTION_SERVICE_DISABLE,
  ACTION_CONTEXT_RESET,
  ACTION_PLATFORM_RESTART,
  ACTION_FACTORY_RESET,
  ACTION_GURU_MEDITATION /* must be the last entry */
} RmgActionType;

/**
 * @enum Friend type
 * @brief The type of friend
 */
typedef enum _RmgFriendType {
  FRIEND_UNKNOWN,
  FRIEND_PROCESS,
  FRIEND_SERVICE,
  FRIEND_INVALID
} RmgFriendType;

/**
 * @enum Friend action type
 * @brief The type of friend action
 */
typedef enum _RmgFriendActionType {
  FRIEND_ACTION_UNKNOWN,
  FRIEND_ACTION_START,
  FRIEND_ACTION_STOP,
  FRIEND_ACTION_RESTART,
  FRIEND_ACTION_SIGNAL,
  FRIEND_ACTION_INVALID
} RmgFriendActionType;

/**
 * @struct RmgFriendResponseEntry
 * @brief The RmgFriendResponseEntry data structure
 */
typedef struct _RmgFriendResponseEntry {
  gchar *service_name;
  RmgFriendActionType action;
  gint64 argument;
  gint64 delay;
} RmgFriendResponseEntry;

/* Preserve the size and order from RmgActionType */
extern const gchar *g_action_name[];
extern RmgRunMode g_run_mode;

G_END_DECLS
