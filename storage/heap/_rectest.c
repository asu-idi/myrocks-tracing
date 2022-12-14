/* Copyright (c) 2000-2002, 2005-2007 MySQL AB
   Use is subject to license terms

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA */

/* Test if a record has changed since last read */
/* In heap this is only used when debugging */

#include "heapdef.h"

int hp_rectest(register HP_INFO *info, const uchar *old)
{
  DBUG_ENTER("hp_rectest");

  if (memcmp(info->current_ptr,old,(size_t) info->s->reclength))
  {
    DBUG_RETURN((my_errno=HA_ERR_RECORD_CHANGED)); /* Record have changed */
  }
  DBUG_RETURN(0);
} /* _heap_rectest */
