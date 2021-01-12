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

#ifndef __UTIL_MAP_H_INCLUDED__
#define __UTIL_MAP_H_INCLUDED__

#ifndef NULL
    #define NULL            0
#endif

#define MAX_KEY_LEN         128

typedef struct _DMAP_NODE   DMAP_NODE;
typedef struct _DMAP_NODE*  HDMAP_NODE;

struct _DMAP_NODE
{
    char        key[MAX_KEY_LEN];
    long long   data;
    DMAP_NODE*  next;
};

typedef struct _DMAP_ROOT   DMAP_ROOT;
typedef struct _DMAP_ROOT*  HDMAP;

struct _DMAP_ROOT
{
    int         hashsize;
    int         nodecount;
    HDMAP_NODE* node;
};

unsigned long gethash( const char* key );

/* return continue? */
typedef int (* DMAP_CALLBACK )( char* key , void* data , void* param );

/* return continue? */
typedef int (* DMAP_CALLBACK2 )( char* key , void** data , void* param );

HDMAP dmap_init( int hashsize );

char* dmap_insert( HDMAP hmap , char* key , void* data );

char* dmap_insert64( HDMAP hmap , char* key , long long data );

void* dmap_query( HDMAP hmap , char* key );

long long dmap_query64( HDMAP hmap , char* key );

void* dmap_getanddel( HDMAP hmap , char* key );

void dmap_erase( HDMAP hmap , char* key );

void dmap_release( HDMAP hmap , DMAP_CALLBACK callback , void* param );

int dmap_foreach( HDMAP hmap , DMAP_CALLBACK callback , void* param );

int dmap_foreach2( HDMAP hmap , DMAP_CALLBACK2 callback , void* param );

int dmap_getcount( HDMAP hmap );

void dmap_rebuild( HDMAP hmap );

#endif
