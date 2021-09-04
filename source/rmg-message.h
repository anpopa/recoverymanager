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
 * \file rmg-message.h
 */
#pragma once

#include "rmg-types.h"

#include <glib.h>

G_BEGIN_DECLS

#define RMG_MESSAGE_PROTOCOL_VERSION (0x0001)
#define RMG_MESSAGE_START_HASH (0xECDE)
#define RMG_MESSAGE_MAX_NAME_LEN (128)

/**
 * @brief The message types
 */
typedef enum _RmgMessageType {
    RMG_MESSAGE_UNKNOWN,
    RMG_MESSAGE_REQUEST_CONTEXT_RESTART,
    RMG_MESSAGE_REQUEST_PLATFORM_RESTART,
    RMG_MESSAGE_REQUEST_FACTORY_RESET,
    RMG_MESSAGE_ACTION_RESPONSE,
    RMG_MESSAGE_REPLICA_DESCRIPTOR,
    RMG_MESSAGE_INFORM_PROCESS_CRASH,
    RMG_MESSAGE_INFORM_CLIENT_SERVICE_FAILED,
    RMG_MESSAGE_INFORM_PRIMARY_SERVICE_FAILED,
    RMG_MESSAGE_INSTANCE_STATUS
} RmgMessageType;

/**
 * @brief The RmgMessageHdr opaque data structure
 */
typedef struct _RmgMessageHdr {
    uint16_t hsh;
    uint16_t session;
    uint32_t version;
    RmgMessageType type;
    uint16_t size_of_arg1;
    uint16_t size_of_arg2;
    uint16_t size_of_arg3;
    uint16_t size_of_arg4;
} RmgMessageHdr;

/**
 * @brief The RmgMessageData opaque data structure
 */
typedef struct _RmgMessageData {
    uint64_t action_response;
    uint64_t instance_status;
    gchar *process_name;
    gchar *service_name;
    gchar *context_name;
} RmgMessageData;

/**
 * @brief The RmgMessage opaque data structure
 */
typedef struct _RmgMessage {
    RmgMessageHdr hdr;
    RmgMessageData data;
    grefcount rc;
} RmgMessage;

/*
 * @brief Create a new message object
 * @param type The message type
 * @param session Unique session identifier
 * @return The new allocated message object
 */
RmgMessage *rmg_message_new(RmgMessageType type, uint16_t session);

/*
 * @brief Aquire a message object
 * @param msg The message object
 * @return The referenced message object
 */
RmgMessage *rmg_message_ref(RmgMessage *msg);

/*
 * @brief Release message
 * @param msg The message object
 */
void rmg_message_unref(RmgMessage *msg);

/*
 * @brief Validate if the message object is consistent
 * @param msg The message object
 */
gboolean rmg_message_is_valid(RmgMessage *msg);

/*
 * @brief Get message type
 * @param msg The message object
 */
RmgMessageType rmg_message_get_type(RmgMessage *msg);

/*
 * @brief Set action response
 * @param msg The message object
 * @param action_response The action responce
 */
void rmg_message_set_action_response(RmgMessage *msg, uint64_t action_response);

/*
 * @brief Get action response
 * @param msg The message object
 * @return The action response
 */
uint64_t rmg_message_get_action_response(RmgMessage *msg);

/*
 * @brief Set instance status
 * @param msg The message object
 * @param instance_status The instance status
 */
void rmg_message_set_instance_status(RmgMessage *msg, uint64_t instance_status);

/*
 * @brief Get instance status
 * @param msg The message object
 * @return The instance status
 */
uint64_t rmg_message_get_instance_status(RmgMessage *msg);

/*
 * @brief Set process name
 * @param msg The message object
 * @param process_name The process name
 */
void rmg_message_set_process_name(RmgMessage *msg, const gchar *process_name);

/*
 * @brief Get process name
 * @param msg The message object
 * @return The process name
 */
const gchar *rmg_message_get_process_name(RmgMessage *msg);

/*
 * @brief Set service name
 * @param msg The message object
 * @param service_name The service name
 */
void rmg_message_set_service_name(RmgMessage *msg, const gchar *service_name);

/*
 * @brief Get service name
 * @param msg The message object
 * @return The service name
 */
const gchar *rmg_message_get_service_name(RmgMessage *msg);

/*
 * @brief Set context name
 * @param msg The message object
 * @param context_name The context name
 */
void rmg_message_set_context_name(RmgMessage *msg, const gchar *context_name);

/*
 * @brief Get context name
 * @param msg The message object
 * @return The context name
 */
const gchar *rmg_message_get_context_name(RmgMessage *msg);

/*
 * @brief Read data into message object
 * If message read has payload the data has to be released by the caller
 * @param msg The message object
 * @param fd File descriptor to read from
 * @return RMG_STATUS_OK on success, RMG_STATUS_ERROR otherwise
 */
RmgStatus rmg_message_read(gint fd, RmgMessage *msg);

/*
 * @brief Write data into message object
 * @param msg The message object
 * @param fd File descriptor to write to
 * @return RMG_STATUS_OK on success, RMG_STATUS_ERROR otherwise
 */
RmgStatus rmg_message_write(gint fd, RmgMessage *msg);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(RmgMessage, rmg_message_unref);

G_END_DECLS
