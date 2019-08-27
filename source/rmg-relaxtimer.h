/* rmg-relaxtimer.h
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
#include "rmg-journal.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @struct RmgRelaxTimer
 * @brief The RmgRelaxTimer opaque data structure
 */
typedef struct _RmgRelaxTimer {
  RmgJournal *journal;
  gchar *service_name;
  glong *rvector_armed;
  grefcount rc;     /**< Reference counter variable  */
} RmgRelaxTimer;

/*
 * @brief Create a new relaxtimer object
 * @return On success return a new RmgRelaxTimer object otherwise return NULL
 */
RmgRelaxTimer *rmg_relaxtimer_new (void);

/**
 * @brief Aquire relaxtimer object
 * @param relaxtimer Pointer to the relaxtimer object
 * @return The referenced relaxtimer object
 */
RmgRelaxTimer *rmg_relaxtimer_ref (RmgRelaxTimer *relaxtimer);

/**
 * @brief Release relaxtimer object
 * @param relaxtimer Pointer to the relaxtimer object
 */
void rmg_relaxtimer_unref (RmgRelaxTimer *relaxtimer);

/**
 * @brief Release relaxtimer object
 * @param relaxtimer Pointer to the relaxtimer object
 */
void rmg_relaxtimer_trigger_from_journal (RmgRelaxTimer *relaxtimer, const gchar *service_name, RmgJournal *journal);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RmgRelaxTimer, rmg_relaxtimer_unref);

G_END_DECLS
