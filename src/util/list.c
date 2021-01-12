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

#include "logger.h"
#include "stypes.h"
#include "memalloc.h"
#include "list.h"

HDLIST dlist_init()
{
    HDLIST  hdlist;
    
    hdlist = memalloc_zero( sizeof( DLIST_ROOT ) );
    return hdlist;
}

void dlist_push( HDLIST hdlist , DLIST_VT pData )
{
    DLIST_NODE* pNode;
    
    if ( hdlist == NULL )
    {
        return;
    }
    
    pNode = memalloc_zero( sizeof( DLIST_NODE ) );
    pNode->pData = pData;
    
    if ( hdlist->pHead == NULL )
    {
        hdlist->pLast = pNode;
        hdlist->pHead = pNode;
    }
    else
    {
        hdlist->pLast->pNext = pNode;
        hdlist->pLast = pNode;
    }
}

DLIST_VT dlist_pop( HDLIST hdlist )
{
    DLIST_VT    pData;
    DLIST_NODE* pNode;
    
    if ( hdlist == NULL )
    {
        return ( DLIST_VT )NULL;
    }
    
    if ( hdlist->pHead == NULL )
    {
        return ( DLIST_VT )NULL;
    }
    
    pNode = hdlist->pHead;
    hdlist->pHead = pNode->pNext;
    
    if ( hdlist->pHead == NULL )
    {
        hdlist->pLast = NULL;
    }
    
    pData = pNode->pData;
    memfree( pNode );
    
    return pData;
}

DLIST_VT dlist_front( HDLIST hdlist )
{
    if ( hdlist == NULL )
    {
        return ( DLIST_VT )NULL;
    }
    
    if ( hdlist->pHead == NULL )
    {
        return ( DLIST_VT )NULL;
    }
    
    return hdlist->pHead->pData;
}

DLIST_VT dlist_last( HDLIST hdlist )
{
    if ( hdlist == NULL )
    {
        return ( DLIST_VT )NULL;
    }
    
    if ( hdlist->pHead == NULL )
    {
        return ( DLIST_VT )NULL;
    }
    
    return hdlist->pLast->pData;
}

void dlist_release( HDLIST hdlist )
{
    DLIST_NODE* pNode;
    DLIST_NODE* pNode2;
    
    if ( hdlist == NULL )
    {
        return;
    }
    
    pNode = hdlist->pHead;
    
    while ( pNode != NULL )
    {
        pNode2 = pNode;
        pNode = pNode->pNext;
        memfree( pNode2 );
    }
    
    memfree( hdlist );
}

void dlist_insertfront( HDLIST hdlist , DLIST_VT pData )
{
    DLIST_NODE* pNode;
    
    if ( hdlist == NULL )
    {
        return;
    }
    
    pNode = memalloc_zero( sizeof( DLIST_NODE ) );
    pNode->pData = pData;
    
    if ( hdlist->pHead == NULL )
    {
        hdlist->pHead = pNode;
        hdlist->pLast = pNode;
    }
    else
    {
        pNode->pNext = hdlist->pHead;
        hdlist->pHead = pNode;
    }
}

int dlist_isempty( HDLIST hdlist )
{
    if ( hdlist == NULL )
    {
        return 1;
    }
    
    if ( hdlist->pHead == NULL )
    {
        return 1;
    }
    
    return 0;
}

int dlist_foreach( HDLIST hdlist , DLIST_CALLBACK callback , void* pParam )
{
    DLIST_NODE* pNode;
    
    if ( callback == NULL )
    {
        return 0;
    }
    
    if ( hdlist == NULL )
    {
        return 0;
    }
    
    pNode = hdlist->pHead;
    
    while ( pNode )
    {
        if ( callback( pNode->pData , pParam ) == 0 )
        {
            return 0;
        }
        
        pNode = pNode->pNext;
    }
    
    return 1;
}
