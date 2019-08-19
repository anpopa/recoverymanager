/* rmg-message.h
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

#include "rmg-message-type.h"
#include "rmg-types.h"

#include <glib.h>

G_BEGIN_DECLS

/*
 * @brief Initialize a pre-allocated mesage object
 *
 * @param m The message object
 * @param type The message type
 * @param session Unique session identifier
 */
void rmg_message_init (RmgMessage *m, RmgMessageType type, uint16_t session);

/*
 * @brief Message data setter
 *
 * @param m The message object
 * @param data The message data to set
 * @param size The message data size
 */
void rmg_message_set_data (RmgMessage *m, void *data, uint32_t size);

/*
 * @brief Message data free
 *
 * The function release the data field of the message if set
 *
 * @param m The message object
 */
void rmg_message_free_data (RmgMessage *m);

/*
 * @brief Validate if the message object is consistent
 * @param m The message object
 */
bool rmg_message_is_valid (RmgMessage *m);

/*
 * @brief Get message type
 * @param m The message object
 */
RmgMessageType rmg_message_get_type (RmgMessage *m);

/*
 * @brief Set message version
 *
 * @param m The message object
 * @param version Version string
 *
 * @return RMG_STATUS_OK on success, RMG_STATUS_ERROR otherwise
 */
RmgStatus rmg_message_set_version (RmgMessage *m, const gchar *version);

/*
 * @brief Read data into message object
 *
 * If message read has payload the data has to be released by the caller
 *
 * @param m The message object
 * @param fd File descriptor to read from
 *
 * @return RMG_STATUS_OK on success, RMG_STATUS_ERROR otherwise
 */
RmgStatus rmg_message_read (gint fd, RmgMessage *m);

/*
 * @brief Write data into message object
 *
 * @param m The message object
 * @param fd File descriptor to write to
 *
 * @return RMG_STATUS_OK on success, RMG_STATUS_ERROR otherwise
 */
RmgStatus rmg_message_write (gint fd, RmgMessage *m);

G_END_DECLS
