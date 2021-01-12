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
#include <stdarg.h>
#include <string.h>
#include "logger.h"

const char* LOG_ERROR   = "";
const char* LOG_INFO    = "";

#define LOG_SRCFILE

void _logger(
    const char*     srcfile     ,
    int             nline       ,
    const char*     prefix      ,
    const char*     format_str  ,
    ...
) {
    va_list         valist;
    char*           buf;
    int             buf_size;
    
    va_start( valist , format_str );
    buf_size = vsnprintf( NULL , 0 , format_str , valist );
    va_end( valist );
    
    if ( prefix == NULL )
    {
        prefix = "";
    }
    
    buf = malloc( buf_size + 2 );
    
    if ( buf != NULL )
    {
        va_start( valist , format_str );
        buf_size = vsnprintf( buf , buf_size + 1 , format_str , valist );
        va_end( valist );
        buf[ buf_size ] = 0;
    }
    
#ifdef LOG_SRCFILE
    if ( srcfile == NULL )
    {
        srcfile = "";
    }
    
    printf( "[%-10s:%04d]%s%s\n" , srcfile , nline , prefix , buf );
#else
    printf( "%s%s\n" , prefix , buf );
#endif
    
    if ( buf != NULL )
    {
        free( buf );
    }
}
