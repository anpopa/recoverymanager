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
 * \file rmg-relaxtimer.h
 */

#pragma once

#include "rmg-journal.h"
#include "rmg-types.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @struct RmgRelaxTimer
 * @brief The RmgRelaxTimer opaque data structure
 */
typedef struct _RmgRelaxTimer {
  RmgJournal *journal;
  gchar *service_name;
  glong rvector;
  glong timeout;
  grefcount rc;
} RmgRelaxTimer;

/*
 * @brief Create a new relaxtimer object
 * @return On success return a new RmgRelaxTimer object otherwise return NULL
 */
guint                   rmg_relaxtimer_trigger              (RmgJournal *journal, 
                                                             const gchar *service_name,
                                                             GError **error);

G_END_DECLS
