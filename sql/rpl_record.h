/* Copyright (c) 2007, 2013, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA */

#ifndef RPL_RECORD_H
#define RPL_RECORD_H

#include <rpl_reporting.h>
#include "my_global.h"                          /* uchar */
#include <unordered_map>

class Relay_log_info;
struct TABLE;
typedef struct st_bitmap MY_BITMAP;

#if !defined(MYSQL_CLIENT)
size_t pack_row(TABLE* table, MY_BITMAP const* cols,
                uchar *row_data, const uchar *data,
                std::unordered_map<int, uint64_t> *field_sizes = nullptr);
#include "sql_base.h"
#endif

#if !defined(MYSQL_CLIENT) && defined(HAVE_REPLICATION)
int unpack_row(Relay_log_info const *rli,
               TABLE *table, uint const colcnt,
               uchar const *const row_data, MY_BITMAP const *cols,
               uchar const **const curr_row_end, ulong *const master_reclength,
               uchar const *const row_end, std::string* row_query= nullptr);

// Fill table's record[0] with default values.
int prepare_record(TABLE *const table, const MY_BITMAP *cols, const bool check);
#endif

#endif
