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
 * \file rmg-message.c
 */

#include "rmg-message.h"

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>

#define RMG_MESSAGE_IOVEC_MAX_ARRAY (16)
const char *rmg_notavailable_str = "NotAvailable";

RmgMessage *
rmg_message_new (RmgMessageType type, uint16_t session)
{
  RmgMessage *msg = g_new0 (RmgMessage, 1);

  g_assert (msg);

  g_ref_count_init (&msg->rc);
  msg->hdr.type = type;
  msg->hdr.hsh = RMG_MESSAGE_START_HASH;
  msg->hdr.session = session;
  msg->hdr.version = RMG_MESSAGE_PROTOCOL_VERSION;

  return msg;
}

RmgMessage *
rmg_message_ref (RmgMessage *msg)
{
  g_assert (msg);
  g_ref_count_inc (&msg->rc);
  return msg;
}

void
rmg_message_unref (RmgMessage *msg)
{
  g_assert (msg);

  if (g_ref_count_dec (&msg->rc) == TRUE)
    {
      g_free (msg->data.service_name);
      g_free (msg->data.context_name);
      g_free (msg);
    }
}

gboolean
rmg_message_is_valid (RmgMessage *msg)
{
  g_assert (msg);

  if (msg->hdr.hsh != RMG_MESSAGE_START_HASH || msg->hdr.version != RMG_MESSAGE_PROTOCOL_VERSION)
    return FALSE;

  return TRUE;
}

RmgMessageType
rmg_message_get_type (RmgMessage *msg)
{
  g_assert (msg);
  return msg->hdr.type;
}

void
rmg_message_set_action_response (RmgMessage *msg, uint64_t action_response)
{
  g_assert (msg);
  msg->data.action_response = action_response;
}

uint64_t
rmg_message_get_action_response (RmgMessage *msg)
{
  g_assert (msg);
  return msg->data.action_response;
}

void
rmg_message_set_instance_status (RmgMessage *msg, uint64_t instance_status)
{
  g_assert (msg);
  msg->data.instance_status = instance_status;
}

uint64_t
rmg_message_get_instance_status (RmgMessage *msg)
{
  g_assert (msg);
  return msg->data.instance_status;
}

void
rmg_message_set_process_name (RmgMessage *msg, const gchar *process_name)
{
  g_assert (msg);
  g_assert (process_name);
  msg->data.process_name = g_strdup (process_name);
}

const gchar *
rmg_message_get_process_name (RmgMessage *msg)
{
  g_assert (msg);
  return msg->data.process_name;
}

void
rmg_message_set_service_name (RmgMessage *msg, const gchar *service_name)
{
  g_assert (msg);
  g_assert (service_name);
  msg->data.service_name = g_strdup (service_name);
}

const gchar *
rmg_message_get_service_name (RmgMessage *msg)
{
  g_assert (msg);
  return msg->data.service_name;
}

void
rmg_message_set_context_name (RmgMessage *msg, const gchar *context_name)
{
  g_assert (msg);
  g_assert (context_name);
  msg->data.context_name = g_strdup (context_name);
}

const gchar *
rmg_message_get_context_name (RmgMessage *msg)
{
  g_assert (msg);
  return msg->data.context_name;
}

RmgStatus
rmg_message_read (gint fd, RmgMessage *msg)
{
  struct iovec iov[RMG_MESSAGE_IOVEC_MAX_ARRAY] = {};
  gint iov_index = 0;
  gssize sz;

  g_assert (msg);

  /* set message header segments */
  iov[iov_index].iov_base = &msg->hdr.hsh;
  iov[iov_index++].iov_len = sizeof(msg->hdr.hsh);
  iov[iov_index].iov_base = &msg->hdr.session;
  iov[iov_index++].iov_len = sizeof(msg->hdr.session);
  iov[iov_index].iov_base = &msg->hdr.version;
  iov[iov_index++].iov_len = sizeof(msg->hdr.version);
  iov[iov_index].iov_base = &msg->hdr.type;
  iov[iov_index++].iov_len = sizeof(msg->hdr.type);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg1;
  iov[iov_index++].iov_len = sizeof(msg->hdr.size_of_arg1);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg2;
  iov[iov_index++].iov_len = sizeof(msg->hdr.size_of_arg2);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg3;
  iov[iov_index++].iov_len = sizeof(msg->hdr.size_of_arg3);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg4;
  iov[iov_index++].iov_len = sizeof(msg->hdr.size_of_arg4);

  if ((sz = readv (fd, iov, iov_index)) <= 0)
    return RMG_STATUS_ERROR;

  memset (iov, 0, sizeof(iov));
  iov_index = 0;

  /* set message data segments */
  switch (rmg_message_get_type (msg))
    {
    case RMG_MESSAGE_ACTION_RESPONSE:
      /* arg1 */
      iov[iov_index].iov_base = &msg->data.action_response;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;
      break;

    case RMG_MESSAGE_INSTANCE_STATUS:
      /* arg1 */
      iov[iov_index].iov_base = &msg->data.instance_status;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;
      break;

    case RMG_MESSAGE_REQUEST_CONTEXT_RESTART:
    case RMG_MESSAGE_REQUEST_PLATFORM_RESTART:
    case RMG_MESSAGE_REQUEST_FACTORY_RESET:
      /* arg1 */
      msg->data.service_name = g_new0 (gchar, msg->hdr.size_of_arg1);
      iov[iov_index].iov_base = msg->data.service_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;

      /* arg2 */
      msg->data.context_name = g_new0 (gchar, msg->hdr.size_of_arg2);
      iov[iov_index].iov_base = msg->data.context_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg2;
      break;

    case RMG_MESSAGE_INFORM_PROCESS_CRASH:
      /* arg1 */
      msg->data.process_name = g_new0 (gchar, msg->hdr.size_of_arg1);
      iov[iov_index].iov_base = msg->data.process_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;

      /* arg2 */
      msg->data.context_name = g_new0 (gchar, msg->hdr.size_of_arg2);
      iov[iov_index].iov_base = msg->data.context_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg2;
      break;

    case RMG_MESSAGE_INFORM_CLIENT_SERVICE_FAILED:
    case RMG_MESSAGE_INFORM_PRIMARY_SERVICE_FAILED:
      /* arg1 */
      msg->data.service_name = g_new0 (gchar, msg->hdr.size_of_arg1);
      iov[iov_index].iov_base = msg->data.service_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;

      /* arg2 */
      msg->data.context_name = g_new0 (gchar, msg->hdr.size_of_arg2);
      iov[iov_index].iov_base = msg->data.context_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg2;
      break;

    case RMG_MESSAGE_REPLICA_DESCRIPTOR:
      /* arg1 */
      msg->data.context_name = g_new0 (gchar, msg->hdr.size_of_arg1);
      iov[iov_index].iov_base = msg->data.context_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;
      break;

    default:
      break;
    }

  /* read into the message structure */
  if (iov_index > 0)
    if ((sz = readv (fd, iov, iov_index)) <= 0)
      return RMG_STATUS_ERROR;

  return RMG_STATUS_OK;
}

RmgStatus
rmg_message_write (gint fd, RmgMessage *msg)
{
  struct iovec iov[RMG_MESSAGE_IOVEC_MAX_ARRAY] = {};
  gint iov_index = 0;
  gssize sz;

  g_assert (msg);

  /* set arguments size */
  switch (rmg_message_get_type (msg))
    {
    case RMG_MESSAGE_ACTION_RESPONSE:
      msg->hdr.size_of_arg1 = sizeof(msg->data.action_response);
      break;

    case RMG_MESSAGE_INSTANCE_STATUS:
      msg->hdr.size_of_arg1 = sizeof(msg->data.instance_status);
      break;

    case RMG_MESSAGE_REQUEST_CONTEXT_RESTART:
    case RMG_MESSAGE_REQUEST_PLATFORM_RESTART:
    case RMG_MESSAGE_REQUEST_FACTORY_RESET:
      msg->hdr.size_of_arg1 = (uint16_t)(sizeof(gchar) * strnlen (msg->data.service_name,
                                                                  RMG_MESSAGE_MAX_NAME_LEN
                                                                  + 1));
      msg->hdr.size_of_arg2 = (uint16_t)(sizeof(gchar) * strnlen (msg->data.context_name,
                                                                  RMG_MESSAGE_MAX_NAME_LEN
                                                                  + 1));
      break;

    case RMG_MESSAGE_INFORM_PROCESS_CRASH:
      msg->hdr.size_of_arg1 = (uint16_t)(sizeof(gchar) * strnlen (msg->data.process_name,
                                                                  RMG_MESSAGE_MAX_NAME_LEN
                                                                  + 1));
      msg->hdr.size_of_arg2 = (uint16_t)(sizeof(gchar) * strnlen (msg->data.context_name,
                                                                  RMG_MESSAGE_MAX_NAME_LEN
                                                                  + 1));
      break;

    case RMG_MESSAGE_INFORM_CLIENT_SERVICE_FAILED:
    case RMG_MESSAGE_INFORM_PRIMARY_SERVICE_FAILED:
      msg->hdr.size_of_arg1 = (uint16_t)(sizeof(gchar) * strnlen (msg->data.service_name,
                                                                  RMG_MESSAGE_MAX_NAME_LEN
                                                                  + 1));
      msg->hdr.size_of_arg2 = (uint16_t)(sizeof(gchar) * strnlen (msg->data.context_name,
                                                                  RMG_MESSAGE_MAX_NAME_LEN
                                                                  + 1));
      break;

    case RMG_MESSAGE_REPLICA_DESCRIPTOR:
      msg->hdr.size_of_arg1 = (uint16_t)(sizeof(gchar) * strnlen (msg->data.context_name,
                                                                  RMG_MESSAGE_MAX_NAME_LEN
                                                                  + 1));
      break;

    default:
      break;
    }

  /* set message header segments */
  iov[iov_index].iov_base = &msg->hdr.hsh;
  iov[iov_index++].iov_len = sizeof(msg->hdr.hsh);
  iov[iov_index].iov_base = &msg->hdr.session;
  iov[iov_index++].iov_len = sizeof(msg->hdr.session);
  iov[iov_index].iov_base = &msg->hdr.version;
  iov[iov_index++].iov_len = sizeof(msg->hdr.version);
  iov[iov_index].iov_base = &msg->hdr.type;
  iov[iov_index++].iov_len = sizeof(msg->hdr.type);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg1;
  iov[iov_index++].iov_len = sizeof(msg->hdr.size_of_arg1);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg2;
  iov[iov_index++].iov_len = sizeof(msg->hdr.size_of_arg2);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg3;
  iov[iov_index++].iov_len = sizeof(msg->hdr.size_of_arg3);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg4;
  iov[iov_index++].iov_len = sizeof(msg->hdr.size_of_arg4);

  if ((sz = writev (fd, iov, iov_index)) <= 0)
    return RMG_STATUS_ERROR;

  memset (iov, 0, sizeof(iov));
  iov_index = 0;

  /* set message data segments */
  switch (rmg_message_get_type (msg))
    {
    case RMG_MESSAGE_ACTION_RESPONSE:
      /* arg1 */
      iov[iov_index].iov_base = &msg->data.action_response;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;
      break;

    case RMG_MESSAGE_INSTANCE_STATUS:
      /* arg1 */
      iov[iov_index].iov_base = &msg->data.instance_status;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;
      break;

    case RMG_MESSAGE_REQUEST_CONTEXT_RESTART:
    case RMG_MESSAGE_REQUEST_PLATFORM_RESTART:
    case RMG_MESSAGE_REQUEST_FACTORY_RESET:
      /* arg1 */
      iov[iov_index].iov_base = msg->data.service_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;

      /* arg2 */
      iov[iov_index].iov_base = msg->data.context_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg2;
      break;

    case RMG_MESSAGE_INFORM_PROCESS_CRASH:
      /* arg1 */
      iov[iov_index].iov_base = msg->data.process_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;

      /* arg2 */
      iov[iov_index].iov_base = msg->data.context_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg2;
      break;

    case RMG_MESSAGE_INFORM_CLIENT_SERVICE_FAILED:
    case RMG_MESSAGE_INFORM_PRIMARY_SERVICE_FAILED:
      /* arg1 */
      iov[iov_index].iov_base = msg->data.service_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;

      /* arg2 */
      iov[iov_index].iov_base = msg->data.context_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg2;
      break;

    case RMG_MESSAGE_REPLICA_DESCRIPTOR:
      /* arg1 */
      iov[iov_index].iov_base = msg->data.context_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;
      break;

    default:
      break;
    }

  /* write into the message structure */
  if (iov_index > 0)
    if ((sz = writev (fd, iov, iov_index)) <= 0)
      return RMG_STATUS_ERROR;

  return RMG_STATUS_OK;
}
