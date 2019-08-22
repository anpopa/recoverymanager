/* rmg-types.h
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

#include <glib.h>
#include <elf.h>

G_BEGIN_DECLS

#ifndef RMG_UNUSED
#define RMG_UNUSED(x) (void)(x)
#endif

#define RMG_EVENT_SOURCE(x) (GSource *)(x)

typedef enum _RmgStatus {
  RMG_STATUS_ERROR = -1,
  RMG_STATUS_OK
} RmgStatus;

typedef enum _RmgRunMode {
  RUN_MODE_MASTER,
  RUN_MODE_SLAVE
} RmgRunMode;

/**
 * @enum Action type
 * @brief The type of action top perform sorted from low to high destructive
 *   Actions with an ID higher or equal ACTION_CONTEXT_RESET will be performed
 *   by master instance only.
 */
typedef enum _RmgActionType {
  ACTION_INVALID,
  ACTION_SERVICE_RESET,
  ACTION_PUBLIC_DATA_RESET,
  ACTION_PRIVATE_DATA_RESET,
  ACTION_SERVICE_DISABLE,
  ACTION_CONTEXT_RESET,
  ACTION_PLATFORM_RESET,
  ACTION_FACTORY_RESET,
  ACTION_GURU_MEDITATION /* must be the last entry */
} RmgActionType;

/* Preserve the size and order from RmgActionType */
extern const gchar *g_action_name[];
extern RmgRunMode g_run_mode;

G_END_DECLS
