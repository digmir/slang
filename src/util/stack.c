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
#include "stack.h"
#include "memalloc.h"

STACK* create_stack( int max_count )
{
    STACK*  stack_ptr;
    
    stack_ptr               = memalloc_zero( sizeof( STACK ) );
    stack_ptr->max_count    = max_count;
    stack_ptr->value_ptr    = memalloc_zero(
        ( max_count + 1 ) * sizeof( void* )
    );
    
    stack_ptr->pos          = 0;
    return stack_ptr;
}

void release_stack( STACK* stack_ptr )
{
    memfree( stack_ptr->value_ptr );
    memfree( stack_ptr );
}

BOOL push( STACK* stack_ptr , void* value )
{
    if ( stack_ptr->pos >= stack_ptr->max_count )
    {
        return FALSE;
    }
    
    stack_ptr->value_ptr[ stack_ptr->pos ] = value;
    stack_ptr->pos++;
    return TRUE;
}

void* pop( STACK* stack_ptr )
{
    if ( stack_ptr->pos == 0 )
    {
        return NULL;
    }
    
    stack_ptr->pos--;
    return stack_ptr->value_ptr[ stack_ptr->pos ];
}

BOOL isempty( STACK* stack_ptr )
{
    if ( stack_ptr->pos == 0 )
    {
        return TRUE;
    }
    
    return FALSE;
}
