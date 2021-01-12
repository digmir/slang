/**
 * Copyright (c) 2020, digmir <dev@digmir.com>
 * 
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * */

#ifndef __UTIL_MAP_INT_H_INCLUDED__
#define __UTIL_MAP_INT_H_INCLUDED__

#ifndef NULL
    #define NULL                0
#endif

#define MAP_VT                  long long

typedef struct _MAP_INT_NODE    MAP_INT_NODE;
typedef struct _MAP_INT_NODE*   HMAP_INT_NODE;

struct _MAP_INT_NODE
{
    MAP_VT          key;
    void*           data;
    MAP_INT_NODE*   next;
};

typedef struct _MAP_INT_ROOT    MAP_INT_ROOT;
typedef struct _MAP_INT_ROOT*   HMAP_INT;

struct _MAP_INT_ROOT
{
    unsigned long   hashsize;
    unsigned long   count;
    HMAP_INT_NODE*  node;
};

unsigned long gethash_int( MAP_VT key );

HMAP_INT map_int_init( unsigned long hashsize );

int map_int_insert( HMAP_INT hmap , MAP_VT key , void* data );

void* map_int_query( HMAP_INT hmap , MAP_VT key );

void map_int_release( HMAP_INT hmap );

unsigned long map_int_getcount( HMAP_INT hmap );

#endif
