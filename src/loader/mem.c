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

#include "logger.h"
#include "slang.h"
#include "datadump.h"
#include "mem.h"

void mem_init( RUNTIME* runtime )
{
    runtime->total_ref = 0;
    runtime->hdelvalue = dlist_init();
}

void mem_release( RUNTIME* runtime )
{
    if ( runtime->total_ref != 0 )
    {
        log_info( "ref = %d" , runtime->total_ref );
    }
    
#ifdef LOG_DEL
    UINT64  pos;
    
    if ( ! dlist_isempty( runtime->hdelvalue ) )
    {
        char delfile[ MAX_NAME_LEN * 2 ];
        
        strcpy( delfile , "del.dat" );
        FILE_DESC* pf = fileopen( delfile , "ab" );
        
        if ( ! pf )
        {
            pf = fileopen( delfile , "wb" );
        }
        
        if ( pf )
        {
            while ( ! dlist_isempty( runtime->hdelvalue ) )
            {
                pos = dlist_pop( runtime->hdelvalue );
                filewrite( pf , &pos , INT64_SIZE );
            }
            
            fileclose( pf );
        }
    }
#endif
    
    dlist_release( runtime->hdelvalue );
    runtime->hdelvalue = NULL;
}

void mem_ext_ref( RUNTIME* runtime , int ref )
{
    runtime->total_ref += ref;
}

HDLIST get_delvalue( RUNTIME* runtime )
{
    return runtime->hdelvalue;
}

RT_VALUE* ref_value( RUNTIME* runtime , RT_VALUE* value )
{
    if ( value )
    {
        if ( value->type == RVT_TBL_ITEM_UNLOAD )
        {
            value = read_data( runtime , value->fpos );
            
            if ( value == NULL )
            {
                return NULL;
            }
        }
        
        value->ref++;
        runtime->total_ref++;
    }
    
    return value;
}

RT_VALUE* ref_value_int( RUNTIME* runtime , int value )
{
    RT_VALUE*   ret;
    
    ret = memalloc_zero( sizeof( RT_VALUE ) );
    ret->data = memalloc_zero( 16 );
    snprintf( ret->data , 16 , "%d" , value );
    ret->size = strlen( ret->data );
    ret->type = RVT_STRING;
    ret->ref = 1;
    runtime->total_ref++;
    return ret;
}

RT_VALUE* ref_value_str( RUNTIME* runtime , char* str , int size , int bcopy )
{
    RT_VALUE*   ret;
    
    ret = memalloc_zero( sizeof( RT_VALUE ) );
    
    if ( ( str == NULL ) || ( size == 0 ) )
    {
        ret->type = RVT_NULL;
        ret->size = 0;
        ret->data = NULL;
    }
    else
    {
        if ( bcopy )
        {
            ret->data = memalloc_zero( size + 1 );
            memcpy( ret->data , str , size );
        }
        else
        {
            ret->data = str;
        }
        
        ret->size = size;
        ret->type = RVT_STRING;
    }
    
    ret->ref = 1;
    runtime->total_ref++;
    return ret;
}

int unref_table_callback( char* name , void* data , RUNTIME* runtime )
{
    if ( data != NULL )
    {
        unref_value( runtime , data );
    }
    
    return 1;
}

RT_VALUE* new_table_value( RUNTIME* runtime )
{
    RT_VALUE*   ret;
    
    ret = memalloc_zero( sizeof( RT_VALUE ) );
    ret->type = RVT_TABLE;
    ret->data = dmap_init( 3 );
    ret->ref = 1;
    runtime->total_ref++;
    return ret;
}

void update_value( RUNTIME* runtime , RT_VALUE* value )
{
    if ( value )
    {
#ifdef LOG_DEL
        if ( ( value->fpos > 0 ) && runtime->hdelvalue )
        {
            dlist_push( runtime->hdelvalue , value->fpos );
        }
#endif
        value->fpos = 0;
    }
}

void unref_value( RUNTIME* runtime , RT_VALUE* value )
{
    if ( value != NULL )
    {
        if ( value->type == RVT_TBL_ITEM_UNLOAD )
        {
            map_int_query( runtime->loadvar , 0 );
            memfree( value );
        }
        else if ( value->ref > 0 )
        {
            value->ref--;
            runtime->total_ref--;
            
            if ( value->ref <= 0 )
            {
#ifdef LOG_DEL
                if ( ( value->fpos > 0 ) && runtime->hdelvalue )
                {
                    dlist_push( runtime->hdelvalue , value->fpos );
                }
#endif
                if ( value->type == RVT_STRING )
                {
                    memfree( value->data );
                }
                else if ( value->type == RVT_TABLE )
                {
                    dmap_release(
                        value->data ,
                        ( DMAP_CALLBACK )unref_table_callback ,
                        runtime
                    );
                }
                
                memfree( value );
            }
        }
    }
}
