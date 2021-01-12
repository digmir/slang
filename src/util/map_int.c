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
#include "map_int.h"
#include "memalloc.h"

unsigned long gethash_int( MAP_VT key )
{
    return key;
}

HMAP_INT map_int_init( unsigned long hashsize )
{
	HMAP_INT	hmap;
	
	if ( hashsize == 0 )
	{
		hashsize = 127773;
	}
	
	hmap = memalloc_zero( sizeof( MAP_INT_ROOT ) );
	hmap->hashsize = hashsize;
	hmap->node = memalloc_zero( hashsize * sizeof( HMAP_INT_NODE ) );
	return hmap;
}

int map_int_insert( HMAP_INT hmap , MAP_VT key , void* data )
{
	unsigned long	hashcode;
	HMAP_INT_NODE	pnode;
	
	if ( hmap == NULL )
	{
		return 0;
	}
	
	hashcode = gethash_int( key ) % hmap->hashsize;
	
    pnode = hmap->node[ hashcode ];
    
	if ( pnode != NULL )
	{
		while ( 1 )
		{
			if ( pnode->key == key )
			{
				pnode->data = data;
				return 1;
			}
			
			if ( pnode->next == NULL )
            {
                pnode->next = memalloc_zero( sizeof( MAP_INT_NODE ) );
                pnode = pnode->next;
                break;
            }
            else
            {
                pnode = pnode->next;
            }
		}
	}
	else
	{
	    pnode = memalloc_zero( sizeof( MAP_INT_NODE ) );
        hmap->node[hashcode] = pnode;
	}
	
    pnode->key = key;
    pnode->data = data;
	hmap->count++;
	
	return 1;
}

void* map_int_query( HMAP_INT hmap , MAP_VT key )
{
	unsigned long	hashcode;
	HMAP_INT_NODE 	pnode;
	
	if ( hmap == NULL )
	{
		return NULL;
	}
	
	hashcode = gethash_int( key ) % hmap->hashsize;
	
    pnode = hmap->node[hashcode];
    
	while ( pnode != NULL )
	{
		if ( pnode->key == key )
		{
			return pnode->data;
		}
		
		pnode = pnode->next;
	}
	
	return NULL;
}

void map_int_release( HMAP_INT hmap )
{
	HMAP_INT_NODE	pnode;
	HMAP_INT_NODE 	pnodenext;
	int 			i;
	
	for ( i = 0; i < hmap->hashsize; i++ )
	{
        pnode = hmap->node[i];
        
		while ( pnode != NULL )
		{
			pnodenext = pnode->next;
			memfree( pnode );
			pnode = pnodenext;
		}
		
		hmap->node[i] = 0;
	}
	
	memfree( hmap->node );
	memfree( hmap );
}

unsigned long map_int_getcount( HMAP_INT hmap )
{
    return hmap->count;
}
