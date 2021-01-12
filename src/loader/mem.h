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

#ifndef __LOADER_MEM_H_INCLUDED__
#define __LOADER_MEM_H_INCLUDED__

#include "memalloc.h"
#include "run.h"
#include "list.h"

void mem_init( RUNTIME* runtime );

void mem_release( RUNTIME* runtime );

void mem_ext_ref( RUNTIME* runtime , int ref );

HDLIST get_delvalue( RUNTIME* runtime );

RT_VALUE* ref_value( RUNTIME* runtime , RT_VALUE* value );

RT_VALUE* ref_value_int( RUNTIME* runtime , int value );

RT_VALUE* ref_value_str( RUNTIME* runtime , char* str , int size , int bcopy );

RT_VALUE* new_table_value( RUNTIME* runtime );

void update_value( RUNTIME* runtime , RT_VALUE* value );

void unref_value( RUNTIME* runtime , RT_VALUE* v );

#endif
