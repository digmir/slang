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
#include "mem.h"
#include "datadump.h"
#include "map_int.h"
#include "list.h"

#include <string.h>

int load_dmp( RUNTIME* runtime )
{
    char    dumpdatafile[ MAX_NAME_LEN * 2 ];
    int     i;
    UINT64  rootpos;
    
    rootpos = 0;
    
    if ( runtime->fData )
    {
        free_dmp( runtime );
    }
    
    strcpy( dumpdatafile , runtime->basepath );
    strcat( dumpdatafile , DATA_DUMP_DATANAME );
    
    runtime->fData = fileopen( dumpdatafile , "rb+" );
    
    if ( runtime->fData == NULL )
    {
        runtime->fData = fileopen( dumpdatafile , "wb+" );
        
        if ( runtime->fData == NULL )
        {
            log_error( "load_dmp(%s) ERROR" , dumpdatafile );
            return 0;
        }
        
        fileclose( runtime->fData );
        runtime->fData = fileopen( dumpdatafile , "rb+" );
        
        if ( runtime->fData == NULL )
        {
            log_error( "load_dmp ERROR2" );
            return 0;
        }
    }
    
    //log_info( "dump size=%d" , filesize( runtime->fData ) );
    
    if ( filesize( runtime->fData ) > INT64_SIZE )
    {
        if ( fileread( runtime->fData , &rootpos , INT64_SIZE ) != INT64_SIZE )
        {
            free_dmp( runtime );
            return 0;
        }
    }
    else
    {
        rootpos = 0;
        filewrite( runtime->fData , &rootpos , INT64_SIZE );
    }
    
    runtime->loadvar = map_int_init( 0 );
    
    if ( rootpos > 0 )
    {
        runtime->root = read_data( runtime , rootpos );
    }
    
    if ( runtime->root == NULL )
    {
        runtime->root = new_table_value( runtime );
    }
    
    return 1;
}

static int tbl_read_data_callback(
    char*       key         ,
    long long*  data_ptr    ,
    RUNTIME*    runtime
);

RT_VALUE* read_data( RUNTIME* runtime , UINT64 pos )
{
    RT_VALUE*   value;
    int         i;
    int         count;
    int         len;
    char        key[MAX_KEY_LEN];
    UINT64      itempos;
    UINT64      keydumpsize;
    
    value = map_int_query( runtime->loadvar , pos );
    
    if ( ( value != NULL ) && ( value->type != RVT_TBL_ITEM_UNLOAD ) )
    {
        return value;
    }
    
    if ( fileseek( runtime->fData , ( long )pos , FILESEEK_SET ) == -1 )
    {
        log_error( "fileseek Error %d" , pos );
        return NULL;
    }
    
    if ( value == NULL )
    {
        value = memalloc_zero( sizeof( RT_VALUE ) );
    }
    
    value->fpos = pos;
    count = fileread( runtime->fData , &value->type , INT_SIZE );
    
    if ( count != INT_SIZE )
    {
        count = filetell( runtime->fData );
        memfree( value );
        log_error( "fileread Error" );
        return NULL;
    }
    
    if ( fileread( runtime->fData , &value->ref , INT_SIZE ) != INT_SIZE )
    {
        memfree( value );
        log_error( "fileread Error" );
        return NULL;
    }
    
    if ( value->type == RVT_STRING )
    {
        if ( fileread( runtime->fData , &value->size , INT_SIZE ) != INT_SIZE )
        {
            memfree( value );
            log_error( "fileread Error" );
            return NULL;
        }
        
        value->data = memalloc_zero( value->size + 1 );
        
        if ( fileread(
            runtime->fData  ,
            value->data     ,
            value->size
        ) != value->size )
        {
            memfree( value );
            log_error( "fileread Error" );
            return NULL;
        }
    }
    else if ( value->type == RVT_TABLE )
    {
        if ( fileread( runtime->fData , &count , INT_SIZE ) != INT_SIZE )
        {
            memfree( value );
            log_error( "fileread Error" );
            return NULL;
        }
        
        if ( count > 0 )
        {
            if ( fileread(
                runtime->fData  ,
                &keydumpsize    ,
                INT64_SIZE
            ) != INT64_SIZE )
            {
                memfree( value );
                log_error( "fileread Error" );
                return NULL;
            }
            
            value->data = dmap_init( count );
            
            for ( i = 0; i < count; i++ )
            {
                if ( fileread( runtime->fData , &len , INT_SIZE ) != INT_SIZE )
                {
                    dmap_release( value->data , NULL , NULL );
                    memfree( value );
                    log_error( "fileread Error" );
                    return NULL;
                }
                
                if ( len >= MAX_KEY_LEN )
                {
                    dmap_release( value->data , NULL , NULL );
                    memfree( value );
                    log_error( "fileread Error" );
                    return NULL;
                }
                
                key[len] = 0;
                
                if ( fileread( runtime->fData , key , len ) != len )
                {
                    dmap_release( value->data , NULL , NULL );
                    memfree( value );
                    log_error( "fileread Error" );
                    return NULL;
                }
                
                if ( fileread(
                    runtime->fData  ,
                    &itempos        ,
                    INT64_SIZE
                ) != INT64_SIZE )
                {
                    dmap_release( value->data , NULL , NULL );
                    memfree( value );
                    log_error( "fileread Error" );
                    return NULL;
                }
                
                dmap_insert64( value->data , key , itempos );
            }
            
            if ( dmap_foreach2(
                value->data ,
                ( DMAP_CALLBACK2 )tbl_read_data_callback ,
                runtime
            ) == 0 )
            {
                dmap_release( value->data , NULL , NULL );
                memfree( value );
                log_error( "table read Error" );
                return NULL;
            }
        }
        else
        {
            value->data = dmap_init( 3 );
        }
    }
    else
    {
        memfree( value );
        return NULL;
    }
    
    if ( value )
    {
        mem_ext_ref( runtime , value->ref );
        map_int_insert( runtime->loadvar , pos , value );
    }
    
    return value;
}

static int tbl_read_data_callback(
    char*       key         ,
    long long*  data_ptr    ,
    RUNTIME*    runtime
) {
    RT_VALUE*   value;
    
    value = map_int_query( runtime->loadvar , *data_ptr );
    
    if ( value )
    {
        *data_ptr = ( long long )( void* )value;
        return 1;
    }
    
    /* change from load all to load on access >*/
    /*value = read_data( runtime , *data_ptr );
    if ( value == NULL )
    {
        log_error( "read_data(%s) ERROR" , key );
    }*/
    /*=*/
    value = memalloc_zero( sizeof( RT_VALUE ) );
    value->fpos = *data_ptr;
    value->type = RVT_TBL_ITEM_UNLOAD;
    map_int_insert( runtime->loadvar , value->fpos , value );
    /*<*/
    
    *data_ptr = ( long long )( void* )value;
    return 1;
}

int tbl_write_data_callback( char* szKey , void* pData , RUNTIME* runtime );

int tbl_write_key_callback( char* szKey , void* pData , RUNTIME* runtime );

UINT64 write_data( RUNTIME* runtime , RT_VALUE* value )
{
    UINT64  pos;
    UINT64  keydumpsize;
    int     count;
    
    if ( value->type == RVT_NULL )
    {
        log_error( "write_data error RVT_NULL" );
        return 0;
    }
    
    if ( value->type == RVT_TBL_ITEM_UNLOAD )
    {
        return value->fpos;
    }
    
    if ( value->type == RVT_STRING )
    {
        if ( value->fpos != 0 )
        {
            return value->fpos;
        }
        
        fileseek( runtime->fData , 0 , FILESEEK_END );
        pos = filetell( runtime->fData );
        
        if ( filewrite( runtime->fData , &value->type , INT_SIZE ) != INT_SIZE )
        {
            log_error( "write_data filewrite error" );
            return 0;
        }
        
        if ( filewrite( runtime->fData , &value->ref , INT_SIZE ) != INT_SIZE )
        {
            log_error( "write_data filewrite error" );
            return 0;
        }
        
        if ( filewrite( runtime->fData , &value->size , INT_SIZE ) != INT_SIZE )
        {
            log_error( "write_data filewrite error" );
            return 0;
        }
        
        if ( filewrite(
            runtime->fData  ,
            value->data     ,
            value->size
        ) != value->size )
        {
            log_error( "write_data filewrite error" );
            return 0;
        }
        
        value->fpos = pos;
        return pos;
    }
    else if ( value->type == RVT_TABLE )
    {
        if ( dmap_foreach(
            value->data ,
            ( DMAP_CALLBACK )tbl_write_data_callback ,
            runtime
        ) == 0 )
        {
            log_error( "write_data filewrite error" );
            return 0;
        }
        
        if ( value->fpos != 0 )
        {
            return value->fpos;
        }
        
        fileseek( runtime->fData , 0 , FILESEEK_END );
        pos = filetell( runtime->fData );
        
        if ( filewrite( runtime->fData , &value->type , INT_SIZE ) != INT_SIZE )
        {
            log_error( "write_data filewrite error" );
            return 0;
        }
        
        if ( filewrite( runtime->fData , &value->ref , INT_SIZE ) != INT_SIZE )
        {
            log_error( "write_data filewrite error" );
            return 0;
        }
        
        count = dmap_getcount( value->data );
        
        if ( filewrite( runtime->fData , &count , INT_SIZE ) != INT_SIZE )
        {
            log_error( "write_data filewrite error" );
            return 0;
        }
        
        keydumpsize = 0;
        
        if ( filewrite(
            runtime->fData  ,
            &keydumpsize    ,
            INT64_SIZE
        ) != INT64_SIZE )
        {
            log_error( "write_data filewrite error" );
            return 0;
        }
        
        if ( dmap_foreach(
            value->data ,
            ( DMAP_CALLBACK )tbl_write_key_callback ,
            runtime
        ) == 0 )
        {
            log_error( "write_data filewrite error" );
            return 0;
        }
        
        value->fpos = pos;
        
        pos = filetell( runtime->fData );
        keydumpsize = pos - value->fpos - ( INT_SIZE * 3 + INT64_SIZE );
        fileseek(
            runtime->fData                  ,
            value->fpos + ( INT_SIZE * 3 )  ,
            FILESEEK_SET
        );
        
        if ( filewrite(
            runtime->fData  ,
            &keydumpsize    ,
            INT64_SIZE
        ) != INT64_SIZE )
        {
            log_error( "write_data filewrite error" );
            return 0;
        }
        
        return value->fpos;
    }
    
    log_error( "write_data filewrite error %d" , value->type );
    return 0;
}

int tbl_write_data_callback( char* szKey , void* pData , RUNTIME* runtime )
{
    int         count;
    RT_VALUE*   value;
    
    value = ( RT_VALUE* )pData;
    
    if ( ( value == NULL )
        || ( value->type == RVT_NULL )
        || ( value->data == NULL ) )
    {
        return 1;
    }
    
    if ( write_data( runtime , value ) == 0 )
    {
        log_error( "write_data filewrite error" );
        return 0;
    }
    
    return 1;
}

int tbl_write_key_callback( char* szKey , void* pData , RUNTIME* runtime )
{
    int         count;
    RT_VALUE*   value;
    
    value = ( RT_VALUE* )pData;
    
    if ( ( value == NULL )
        || ( value->type == RVT_NULL )
        || ( value->data == NULL ) )
    {
        return 1;
    }
    
    count = strlen( szKey );
    
    if ( filewrite( runtime->fData , &count , INT_SIZE ) != INT_SIZE )
    {
        return 0;
    }
    
    if ( filewrite( runtime->fData , szKey , count ) != count )
    {
        return 0;
    }
    
    if ( filewrite( runtime->fData , &value->fpos , INT64_SIZE ) != INT64_SIZE )
    {
        return 0;
    }
    
    return 1;
}

void free_dmp( RUNTIME* runtime )
{
    if ( runtime->fData )
    {
        fileclose( runtime->fData );
        runtime->fData = NULL;
    }
    
    if ( runtime->loadvar )
    {
        map_int_release( runtime->loadvar );
        runtime->loadvar = NULL;
    }
    
    if ( runtime->root )
    {
        unref_value( runtime , runtime->root );
        runtime->root = NULL;
    }
}

int dump_dmp( RUNTIME* runtime )
{
    UINT64  rootpos;
    
    if ( runtime == NULL )
    {
        return 0;
    }
    
    if ( runtime->module == NULL || runtime->root == NULL )
    {
        return 0;
    }
    
    if ( runtime->fData == NULL )
    {
        if ( ! load_dmp( runtime ) )
        {
            return 0;
        }
    }
    
    rootpos = write_data( runtime , runtime->root );
    
    if ( rootpos > 0 )
    {
        fileseek( runtime->fData , 0 , FILESEEK_SET );
        
        if ( filewrite( runtime->fData , &rootpos , INT64_SIZE ) != INT64_SIZE )
        {
            log_error( "write_data filewrite error" );
            return 0;
        }
    }
    
    free_dmp( runtime );
    return 1;
}

int dmp_init( RUNTIME* runtime )
{
    return load_dmp( runtime );
}

void dmp_release( RUNTIME* runtime )
{
    if ( dump_dmp( runtime ) == 0 )
    {
        log_error( "dump_dmp Error" );
    }
    
    free_dmp( runtime );
}

RT_VALUE* queryitem( RUNTIME* runtime , HDMAP hMap , char* key )
{
    RT_VALUE*   ret;
    
    ret = dmap_query( hMap , key );
    
    if ( ret != NULL )
    {
        if ( ret->type == RVT_TBL_ITEM_UNLOAD )
        {
            ret = read_data( runtime , ret->fpos );
            
            if ( ret == NULL )
            {
                return NULL;
            }
        }
    }
    
    return ret;
}

RT_VALUE* dmp_get_rtvalue(
    RUNTIME*    runtime     ,
    char*       modname     ,
    char*       nodename    ,
    char*       varname
) {
    RT_VALUE*   ret;
    
    if ( runtime->fData == NULL || runtime->root == NULL )
    {
        if ( load_dmp( runtime ) == 0 )
        {
            return NULL;
        }
    }
    
    if ( runtime->fData == NULL || runtime->root == NULL )
    {
        return NULL;
    }
    
    ret = queryitem( runtime , runtime->root->data , modname );
    
    if ( ret != NULL )
    {
        ret = queryitem( runtime , ret->data , nodename );
        
        if ( ret != NULL )
        {
            ret = queryitem( runtime , ret->data , varname );
            return ret;
        }
    }
    
    return NULL;
}

int dmp_set_rtvalue(
    RUNTIME*    runtime     ,
    char*       modname     ,
    char*       nodename    ,
    char*       varname     ,
    RT_VALUE*   var
) {
    RT_VALUE*   rvmod;
    RT_VALUE*   rvfunc;
    RT_VALUE*   rvvar;
    
    if ( ( runtime->fData == NULL ) || ( runtime->root == NULL ) )
    {
        if ( load_dmp( runtime ) == 0 )
        {
            return 0;
        }
    }
    
    if ( runtime->fData == NULL || runtime->root == NULL )
    {
        return 0;
    }
    
    rvmod = queryitem( runtime , runtime->root->data , modname );
    
    if ( rvmod == NULL )
    {
        rvmod = new_table_value( runtime );
        dmap_insert( runtime->root->data , modname , rvmod );
        rvfunc = NULL;
    }
    else
    {
        rvfunc = queryitem( runtime , rvmod->data , nodename );
    }
    
    if ( rvfunc == NULL )
    {
        rvfunc = new_table_value( runtime );
        dmap_insert( rvmod->data , nodename , rvfunc );
    }
    else
    {
        rvvar = queryitem( runtime , rvfunc->data , varname );
        
        if ( rvvar != NULL )
        {
            if ( rvvar == var )
            {
                return 1;
            }
            
            unref_value( runtime , rvvar );
        }
    }
    
    if ( var != NULL )
    {
        dmap_insert( rvfunc->data , varname , var );
    }
    else
    {
        dmap_erase( rvfunc->data , varname );
    }
    
    return 1;
}
