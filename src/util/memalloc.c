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
#include <string.h>
#include "stypes.h"
#include "memalloc.h"

void* memalloc( long  size )
{
    return malloc( size );
}

void* memalloc_zero( long  size )
{
    void* ret;
    
    ret = memalloc( size );
    memset( ret , 0 , size );
    return ret;
}

void* memclone( void* ptr , long size )
{
    void*   ptr2;
    
    ptr2 = memalloc( size );
    memcpy( ptr2 , ptr , size );
    return ptr2;
}

char* dup_str( const char* str )
{
    char*   ret;
    int     n;
    
    n       = strlen( str );
    ret     = memalloc( n + 1 );
    strcpy( ret , str );
    ret[n]  = 0;
    return ret;
}

void memfree( void* ptr )
{
    free( ptr );
}
