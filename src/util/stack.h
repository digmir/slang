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

#ifndef __UTIL_STACK_H_INCLUDED__
#define __UTIL_STACK_H_INCLUDED__

#include "stypes.h"

typedef struct _STACK   STACK;

struct _STACK
{
    void**      value_ptr;
    int         max_count;
    int         pos;
};

STACK* create_stack( int max_count );

void release_stack( STACK* stack_ptr );

BOOL push( STACK* stack_ptr , void* value );

void* pop( STACK* stack_ptr );

BOOL isempty( STACK* stack_ptr );

#endif
