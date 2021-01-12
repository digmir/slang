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

#ifndef __UTIL_LIST_H_INCLUDED__
#define __UTIL_LIST_H_INCLUDED__

#define DLIST_VT            long long

typedef struct _DLIST_NODE  DLIST_NODE;

struct _DLIST_NODE
{
    DLIST_VT    pData;
    DLIST_NODE* pNext;
};

typedef struct _DLIST_ROOT  DLIST_ROOT;
typedef struct _DLIST_ROOT* HDLIST;

struct _DLIST_ROOT
{
    DLIST_NODE* pHead;
    DLIST_NODE* pLast;
};

/* return continue? */
typedef int (* DLIST_CALLBACK )( DLIST_VT pData , void* pParam );

HDLIST dlist_init();

void dlist_push( HDLIST hdlist , DLIST_VT pData );

DLIST_VT dlist_pop( HDLIST hdlist );

DLIST_VT dlist_front( HDLIST hdlist );

DLIST_VT dlist_last( HDLIST hdlist );

void dlist_release( HDLIST hdlist );

void dlist_insertfront( HDLIST hdlist , DLIST_VT pData );

int dlist_isempty( HDLIST hdlist );

int dlist_foreach( HDLIST hdlist , DLIST_CALLBACK callback , void* pParam );

#endif
