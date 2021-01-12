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

#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef WINDOWS
    #include <windows.h>
#else
    #include <unistd.h>
    #define MAX_PATH 256
#endif

#include "logger.h"
#include "slang.h"
#include "run.h"
#include "mem.h"

char* getfilepath( char* path , char* filename )
{
    char*   p;
    
    strcpy( filename , path );
    p = strrchr( filename , '/' );
    
    if ( p )
    {
        *p = 0;
    }
    else
    {
        p = strrchr( filename , '\\' );
        
        if ( p )
        {
            *p = 0;
        }
        else
        {
            strcpy( filename , "./" );
        }
    }
    
    return filename;
}

char* getfilename( char* path , char* filename )
{
    char*   p;
    
    p = strrchr( path , '/' );
    
    if ( p )
    {
        strcpy( filename , p + 1 );
    }
    else
    {
        p = strrchr( path , '\\' );
        
        if ( p )
        {
            strcpy( filename , p + 1 );
        }
        else
        {
            strcpy( filename , path );
        }
    }
    
    p = strrchr( filename , '.' );
    
    if ( p )
    {
        *p = 0;
    }
    
    return filename;
}

int get_param_form_commandline(
    RUNTIME*    runtime     ,
    NODE_PARAM* param       ,
    int         offsetarg   ,
    int         argc        ,
    char**      argv
) {
    int         i;
    int         count;
    PARAM_ITEM* item;
    char        name[16];
    
    count = argc - offsetarg;
    item = memalloc_zero( sizeof( PARAM_ITEM ) );
    param->count = 1;
    item[0].value = new_table_value( runtime );
    
    for ( i = 0; i < count; i++ )
    {
        snprintf( name , 16 , "%d" , i + 1 );
        dmap_insert(
            item[0].value->data     ,
            name                    ,
            ref_value_str(
                runtime                         ,
                argv[ i + offsetarg ]           ,
                strlen( argv[ i + offsetarg ] ) ,
                1
            )
        );
    }
    
    param->list = item;
    
    return count;
}

int loader( char* src_file , int argc , char** argv )
{
    char        cur_path[MAX_PATH];
    RUNTIME*    runtime;
    MODULE*     module;
    NODE*       node;
    NODE_PARAM  param;
    char*       p;
    char        module_name[MAX_PATH];
    
    getfilepath( src_file , cur_path );
    
    runtime = new_runtime( cur_path );
    module = NULL;
    
    if ( runtime == NULL )
    {
        log_error( "new runtime %s error" , src_file );
        return 1;
    }
    
    module_name[0] = '.';
    getfilename( src_file , &( module_name[1] ) );
    
    module = load_module( runtime , module_name );
    
    if ( module == NULL )
    {
        log_error( "load module %s error" , module_name );
    }
    else
    {
        node = module->node;
        strcpy( module_name , "." );
        strcat( module_name , module->name );
        strcat( module_name , "." );
        strcat( module_name , node->name);
        
        memset( &param , 0 , sizeof( NODE_PARAM ) );
        
        if ( argc > 3 )
        {
            get_param_form_commandline( runtime , &param , 3 , argc , argv );
        }
        
        runtime->current.module = module;
        
        call_node(
            runtime             ,
            module_name         ,
            param.list          ,
            param.count         ,
            NULL                ,
            0                   ,
            &runtime->errorno
        );
        
        free_param_list_value( runtime , param.list , param.count );
        free_param_list( param.list , param.count );
        
        if ( param.list != NULL )
        {
            memfree( param.list );
        }
    }
    
    release_runtime( runtime );
    
    return 0;
}

int main( int argc , char** argv )
{
    NODE*       node;
    char        src_file[MAX_PATH];
    char        dest_file[MAX_PATH];
    int         mode;
    char*       p;
    
    memset( src_file , 0 , MAX_PATH );
    memset( dest_file , 0 , MAX_PATH );
    node        = NULL;
    mode        = 0;
    
    if ( argc <= 0 )
    {
        log_info( "usage:  [-c] <file>" );
        return 1;
    }
    else if ( ( argc <= 1 ) || ( argc > 3 ) )
    {
        log_info( "usage: %s [-c] <file>" , argv[0] );
        return 1;
    }
    
    if ( argc == 2 )
    {
        mode = 2;
        strncpy( src_file , argv[1] , MIN( MAX_PATH - 1 , strlen( argv[1] ) ) );
    }
    else
    {
        if ( strcmp( argv[1] , "-c" ) == 0 )
        {
            mode = 1;
        }
        else if ( strcmp( argv[1] , "-l" ) == 0 )
        {
            mode = 2;
        }
        else
        {
            log_info( "usage: %s [-c] <file>" , argv[0] );
            return 1;
        }
        
        strncpy( src_file , argv[2] , MIN( MAX_PATH - 1 , strlen( argv[2] ) ) );
    }
    
    if ( mode == 1 )
    {
        strcpy( dest_file , src_file );
        
        p = strrchr( dest_file , '.' );
        
        if ( p )
        {
            strcpy( p , CODE_EXT_FILENAME );
        }
        else
        {
            strcat( dest_file , CODE_EXT_FILENAME );
        }
        
        node = parse_source_file( src_file );
        
        if ( node == NULL )
        {
            log_error( "parse %s error" , src_file );
            return 1;
        }
        
        dump_node( node , dest_file );
        free_node( node );
    }
    else if ( mode == 2 )
    {
        return loader( src_file , argc , argv );
    }
    
    return 1;
}
