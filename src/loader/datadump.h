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

#ifndef __LOADER_DATADUMP_H_INCLUDED__
#define __LOADER_DATADUMP_H_INCLUDED__

#include "slang.h"
#include "run.h"

typedef unsigned long long  UINT64;

int dmp_init( RUNTIME* runtime );

void dmp_release( RUNTIME* runtime );

RT_VALUE* read_data( RUNTIME* runtime , UINT64 pos );

RT_VALUE* dmp_get_rtvalue(
    RUNTIME*    runtime     ,
    char*       modname     ,
    char*       nodename    ,
    char*       varname
);

int dmp_set_rtvalue(
    RUNTIME*    runtime     ,
    char*       modname     ,
    char*       nodename    ,
    char*       varname     ,
    RT_VALUE*   var
);

void free_dmp( RUNTIME* runtime );

#endif
