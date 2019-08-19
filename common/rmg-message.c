/* rmg-message.c
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

#include "rmg-message.h"

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void
rmg_message_init (RmgMessage *m,
                  RmgMessageType type,
                  uint16_t session)
{
  g_assert (m);

  memset (m, 0, sizeof(RmgMessage));
  m->hdr.type = type;
  m->hdr.hsh = RMG_MESSAGE_START_HASH;
  m->hdr.session = session;
}

void
rmg_message_set_data (RmgMessage *m,
                      void *data,
                      uint32_t size)
{
  g_assert (m);
  g_assert (data);

  m->hdr.data_size = size;
  m->data = data;
}

void
rmg_message_free_data (RmgMessage *m)
{
  g_assert (m);

  if (m->data != NULL)
    free (m->data);
}

bool
rmg_message_is_valid (RmgMessage *m)
{
  if (m == NULL)
    return false;

  if (m->hdr.hsh != RMG_MESSAGE_START_HASH)
    return false;

  return true;
}

RmgMessageType
rmg_message_get_type (RmgMessage *m)
{
  g_assert (m);
  return m->hdr.type;
}

RmgStatus
rmg_message_set_version (RmgMessage *m,
                         const gchar *version)
{
  g_assert (m);
  snprintf ((gchar*)m->hdr.version, RMG_VERSION_STRING_LEN, "%s", version);
  return RMG_STATUS_OK;
}

RmgStatus
rmg_message_read (gint fd,
                  RmgMessage *m)
{
  gssize sz;

  g_assert (m);

  sz = read (fd, &m->hdr, sizeof(RmgMessageHdr));
  if (sz != sizeof(RmgMessageHdr) || !rmg_message_is_valid (m))
    return RMG_STATUS_ERROR;

  m->data = calloc (1, m->hdr.data_size);
  if (m->data == NULL)
    return RMG_STATUS_ERROR;

  sz = read (fd, m->data, m->hdr.data_size);

  return sz == m->hdr.data_size ? RMG_STATUS_OK : RMG_STATUS_ERROR;
}

RmgStatus
rmg_message_write (gint fd,
                   RmgMessage *m)
{
  gssize sz;

  g_assert (m);

  if (!rmg_message_is_valid (m))
    return RMG_STATUS_ERROR;

  sz = write (fd, &m->hdr, sizeof(RmgMessageHdr));
  if (sz != sizeof(RmgMessageHdr))
    return RMG_STATUS_ERROR;

  sz = write (fd, m->data, m->hdr.data_size);

  return sz == m->hdr.data_size ? RMG_STATUS_OK : RMG_STATUS_ERROR;
}
