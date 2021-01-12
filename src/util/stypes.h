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

#ifndef __UTIL_TYPES_H__INCLUDED__
#define __UTIL_TYPES_H__INCLUDED__

#include <stdint.h>

#define INT2PTR(x)                  ( void* )( intptr_t )x

typedef signed char                 INT8;
typedef signed short                INT16;
typedef signed int                  INT32;
typedef signed long long            INT64;

typedef unsigned char               UINT8;
typedef unsigned short              UINT16;
typedef unsigned int                UINT32;
typedef unsigned long long          UINT64;

typedef INT32                       INT;
typedef UINT32                      UINT;

#ifndef MAX
    #define MAX(x,y) ((x)>(y)?(x):(y))
#endif

#ifndef MIN
    #define MIN(x,y) ((x)<(y)?(x):(y))
#endif

#ifndef BOOL
    #define BOOL    int
    #define FALSE   0
    #define TRUE    1
#endif

#ifndef NULL
    #define NULL    ( ( void* )0 )
#endif

#ifndef NOOP
    #define NOOP
#endif

#endif
