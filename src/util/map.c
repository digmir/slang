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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "logger.h"
#include "map.h"
#include "memalloc.h"

unsigned long gethash( const char* key )
{
    unsigned long   hash;
    int             i;
    
    hash    = 0;
    i       = MAX_KEY_LEN;
    
    while ( (* key ) && ( i > 0 ) )
    {
        hash = ( hash << i ) + hash + toupper( *key++ );
        i--;
    }
    
    return hash;
}

HDMAP dmap_init( int hashsize )
{
    HDMAP   hmap;
    
    if ( hashsize == 0 )
    {
        hashsize = 127773;
    }
    
    hmap = memalloc_zero( sizeof( DMAP_ROOT ) );
    hmap->hashsize = hashsize;
    hmap->node = memalloc_zero( sizeof( HDMAP_NODE ) * hashsize );
    return hmap;
}

char* dmap_insert( HDMAP hmap , char* key , void* data )
{
    return dmap_insert64( hmap , key , ( long long )data );
}

char* dmap_insert64( HDMAP hmap , char* key , long long data )
{
    unsigned long   hashcode;
    HDMAP_NODE      node;
    
    if ( hmap == NULL )
    {
        return NULL;
    }
    
    if ( hmap->nodecount > ( hmap->hashsize * 10 ) )
    {
        dmap_rebuild( hmap );
    }
    
    hashcode = gethash( key ) % hmap->hashsize;
    
    node = hmap->node[hashcode];
    
    if ( node != NULL )
    {
        while ( 1 )
        {
            if ( strncmp( node->key , key , MAX_KEY_LEN ) == 0 )
            {
                node->data = data;
                return node->key;
            }
            
            if ( node->next == NULL )
            {
                node->next = memalloc_zero( sizeof( DMAP_NODE ) );
                node = node->next;
                break;
            }
            else
            {
                node = node->next;
            }
        }
    }
    else
    {
        node = memalloc_zero( sizeof( DMAP_NODE ) );
        hmap->node[hashcode] = node;
    }
    
    strncpy( node->key , key , MAX_KEY_LEN );
    node->data = data;
    hmap->nodecount++;
    
    return node->key;
}

void* dmap_query( HDMAP hmap , char* key )
{
    return ( void* )dmap_query64( hmap , key );
}

long long dmap_query64( HDMAP hmap , char* key )
{
    unsigned long   hashcode;
    HDMAP_NODE      node;
    
    if ( hmap == NULL )
    {
        return ( long long )NULL;
    }
    
    hashcode = gethash( key ) % hmap->hashsize;
    
    node = hmap->node[hashcode];
    
    while ( node != NULL )
    {
        if ( strncmp( key , node->key , MAX_KEY_LEN ) == 0 )
        {
            return node->data;
        }
        
        node = node->next;
    }
    
    return ( long long )NULL;
}

void* dmap_getanddel( HDMAP hmap , char* key )
{
    void*           ret;
    unsigned long   hashcode;
    HDMAP_NODE      parentnode;
    HDMAP_NODE      node;
    
    if ( hmap == NULL )
    {
        return NULL;
    }
    
    hashcode = gethash( key ) % hmap->hashsize;
    
    node = hmap->node[hashcode];
    
    parentnode = NULL;
    
    while ( node != NULL )
    {
        if ( strncmp( key , node->key , MAX_KEY_LEN ) == 0 )
        {
            ret = ( void* )node->data;
            
            if ( parentnode == NULL )
            {
                hmap->node[hashcode] = node->next;
            }
            else
            {
                parentnode->next = node->next;
            }
            
            hmap->nodecount--;
            memfree( node );
            return ret;
        }
        
        parentnode = node;
        node = node->next;
    }
    
    return NULL;
}

void dmap_erase( HDMAP hmap , char* key )
{
    unsigned long   hashcode;
    HDMAP_NODE      node;
    HDMAP_NODE      parentnode;
    
    if ( hmap == NULL )
    {
        return;
    }
    
    hashcode = gethash( key ) % hmap->hashsize;
    node = hmap->node[hashcode];
    
    if ( node == NULL )
    {
        return;
    }
    
    parentnode = node;
    
    while ( node != NULL )
    {
        if ( strncmp( key , node->key , MAX_KEY_LEN ) == 0 )
        {
            if ( node == parentnode )
            {
                hmap->node[hashcode] = node->next;
            }
            else
            {
                parentnode->next = node->next;
            }
            
            hmap->nodecount--;
            memfree( node );
            
            if ( ( hmap->nodecount * 100 ) < hmap->hashsize )
            {
                dmap_rebuild( hmap );
            }
            
            return;
        }
        
        parentnode = node;
        node = node->next;
    }
    return;
}

void dmap_release( HDMAP hmap , DMAP_CALLBACK callback , void* param )
{
    HDMAP_NODE  node;
    HDMAP_NODE  nodenext;
    int         i;
    
    for ( i = 0; i < hmap->hashsize; i++ )
    {
        node = hmap->node[i];
        
        while ( node != NULL )
        {
            nodenext = node->next;
            
            if ( callback != NULL )
            {
                callback( node->key , ( void* )node->data , param );
            }
            
            memfree( node );
            node = nodenext;
        }
        
        hmap->node[i] = 0;
    }
    
    memfree( hmap->node );
    memfree( hmap );
}

int dmap_foreach( HDMAP hmap , DMAP_CALLBACK callback , void* param )
{
    HDMAP_NODE  node;
    int         i;
    
    if ( callback == NULL )
    {
        return 0;
    }
    
    for ( i = 0; i < hmap->hashsize; i++ )
    {
        node = hmap->node[i];
        
        while ( node != NULL )
        {
            if ( callback( node->key , ( void* )node->data , param ) == 0 )
            {
                return 0;
            }
            
            node = node->next;
        }
    }
    
    return 1;
}

int dmap_foreach2( HDMAP hmap , DMAP_CALLBACK2 callback , void* param )
{
    HDMAP_NODE  node;
    int         i;
    
    if ( callback == NULL )
    {
        return 0;
    }
    
    for ( i = 0; i < hmap->hashsize; i++ )
    {
        node = hmap->node[i];
        
        while ( node != NULL )
        {
            if ( callback( node->key , ( void** )&node->data , param ) == 0 )
            {
                return 0;
            }
            
            node = node->next;
        }
    }
    
    return 1;
}

int dmap_getcount( HDMAP hmap )
{
    return hmap->nodecount;
}

void dmap_rebuild( HDMAP hmap )
{
    int             i;
    HDMAP_NODE      oldnode;
    HDMAP_NODE      node;
    unsigned long   hashcode;
    int             newhashsize;
    HDMAP_NODE*     newnodelist;
    
    newhashsize = hmap->nodecount * 2;
    newnodelist = memalloc_zero( sizeof( HDMAP_NODE ) * newhashsize );
    
    for ( i = 0; i < hmap->hashsize; i++ )
    {
        oldnode = hmap->node[i];
        
        while ( oldnode != NULL )
        {
            hashcode = gethash( oldnode->key ) % newhashsize;
            
            node = newnodelist[hashcode];
            
            if ( node != NULL )
            {
                while ( node->next != NULL )
                {
                    node = node->next;
                }
                
                node->next = oldnode;
            }
            else
            {
                newnodelist[hashcode] = oldnode;
            }
            
            node = oldnode;
            oldnode = oldnode->next;
            node->next = NULL;
        }
    }
    
    memfree( hmap->node );
    hmap->node = newnodelist;
    hmap->hashsize = newhashsize;
}
