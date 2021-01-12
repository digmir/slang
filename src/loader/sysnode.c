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
#include "slang.h"
#include "mem.h"
#include "slextlib.h"
#include "sysnode.h"
#include "errorstr.h"

#include <string.h>

#ifdef WINDOWS
    #include <windows.h>
#else
    #include <termios.h>
    #include <stdio.h>
    //#include <libio.h>
    #include <unistd.h>
    #include <fcntl.h>
    
    #define __need_timeval
    #include <sys/time.h>
    
    unsigned int GetTickCount()
    {
        struct timeval tv;
        
        if ( gettimeofday( &tv , NULL ) != 0 )
        {
            return 0;
        }
        
        return ( tv.tv_sec * 1000 ) + ( tv.tv_usec / 1000 );
    }
#endif

int sf_set_return_value( RUNTIME* runtime , int i , RT_VALUE* pvalue )
{
    if ( ( runtime->current.retvalue_count > i ) && ( i >= 0 ) )
    {
        unref_value( runtime , runtime->current.retvalue[i].value );
        runtime->current.retvalue[i].value = ref_value( runtime , pvalue );
        return RET_OK;
    }
    
    return RET_ERROR;
}

int sysnode_getchar( RUNTIME* runtime )
{
    int c;
    
    c = 0;
#ifdef WINDOWS
    if ( _kbhit() )
    {
        c = getch();
    }
    else
    {
        c = 0;
    }
#else
    struct termios oldt, newt;
    int ch;
    int oldf;
    
    tcgetattr( STDIN_FILENO , &oldt );
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO , TCSANOW , &newt );
    oldf = fcntl( STDIN_FILENO , F_GETFL , 0 );
    fcntl( STDIN_FILENO , F_SETFL , oldf | O_NONBLOCK );
    ch = getchar();
    tcsetattr( STDIN_FILENO , TCSANOW , &oldt );
    fcntl( STDIN_FILENO , F_SETFL , oldf );
    
    if ( ch != EOF )
    {
        ungetc( ch , stdin );
        c = getchar();
    }
    else
    {
        c = 0;
    }
#endif
    return g_extlib_func.slext_set_int( runtime , 0 , c );
}

int sysnode_modulename( RUNTIME* runtime )
{
    if ( runtime->current.module )
    {
        g_extlib_func.slext_set_ptr(
            runtime ,
            0 ,
            runtime->current.module->name ,
            strlen( runtime->current.module->name ) ,
            1
        );
    }
    else
    {
        g_extlib_func.slext_set_ptr( runtime , 0 , NULL , 0 , 1 );
    }
    
    return RET_OK;
}

int sysnode_name( RUNTIME* runtime )
{
    char name[MAX_NAME_LEN*2] = {0};
    
    if ( runtime->current.module )
    {
        strcpy( name , runtime->current.module->name );
    }
    
    if ( runtime->current.node )
    {
        strcat( name , "." );
        strcat( name , runtime->current.node->name );
    }
    
    g_extlib_func.slext_set_ptr( runtime , 0 , name , strlen( name ) , 1 );
    return RET_OK;
}

int sysnode_fission( RUNTIME* runtime )
{
    char*   nodename;
    int     nodenamesize;
    NODE*   node;
    NODE*   node2;
    int     i;
    
    nodename = g_extlib_func.slext_get_ptr( runtime , 0 , &nodenamesize );
    
    if ( nodename == NULL )
    {
        log_error( ERROR_MISSNAME );
        return RET_OK;
    }
    
    node = runtime->current.module->node;
    node2 = node;
    
    while ( node )
    {
        if ( strcmp( node->name , nodename ) == 0 )
        {
            log_error( ERROR_OCCUNAME );
            return RET_OK;
        }
        
        node2 = node;
        node = node->next;
    }
    
    node = memalloc_zero( sizeof( NODE ) );
    
    if ( node2 )
    {
        node2->next = node;
    }
    else
    {
        runtime->current.module->node = node;
    }
    
    memcpy( node , runtime->current.node , sizeof( NODE ) );
    node->next = NULL;
    memset( node->name , 0 , MAX_WORD_NAME_LEN );
    strncpy(
        node->name ,
        nodename ,
        MIN( nodenamesize , MAX_WORD_NAME_LEN - 1 )
    );
    node->type = NT_FISSIONNODE;
    node->param = memalloc_zero( sizeof( NODE_PARAM ) );
    node->param->count = runtime->current.node->param->count;
    
    if ( node->param->count > 0 )
    {
        node->param->list = memalloc_zero(
            node->param->count * sizeof( PARAM_ITEM )
        );
        
        for ( i = 0; i < node->param->count; i++ )
        {
            strcpy(
                node->param->list[i].name ,
                runtime->current.node->param->list[i].name
            );
            node->param->list[i].value = ref_value(
                runtime ,
                runtime->current.node->param->list[i].value
            );
        }
    }
    
    if ( runtime->current.module->nodemap )
    {
        dmap_insert( runtime->current.module->nodemap , node->name , node );
    }
    
    return RET_OK;
}

int sysnode_print( RUNTIME* runtime )
{
    int     i;
    int     n;
    char*   tmpstr;
    
    n = g_extlib_func.slext_get_count( runtime );
    
    if ( n > 0 )
    {
        for ( i = 0; i < n; i++ )
        {
            tmpstr = g_extlib_func.slext_get_ptr( runtime , i , NULL );
            
            if ( tmpstr )
            {
                printf( "%s" , tmpstr );
            }
        }
    }
    
    printf( "\n" );
    return RET_OK;
}

typedef int (* SYSLIBNODE )( RUNTIME* runtime );

int call_sys_node( RUNTIME* runtime , char* node_name )
{
    SYSLIBNODE  node;
    
    if ( runtime->sysnode )
    {
        node = ( SYSLIBNODE )dmap_query( runtime->sysnode , node_name );
        
        if ( node )
        {
            return node( runtime );
        }
    }
    
    return RET_NO_NODE;
}

HDMAP init_sysnode_map()
{
    HDMAP hmap;
    
    hmap = dmap_init( 20 );
    
    dmap_insert( hmap , ".modulename" , sysnode_modulename );
    dmap_insert( hmap , ".name" , sysnode_name );
    dmap_insert( hmap , ".fission" , sysnode_fission );
    dmap_insert( hmap , ".getchar" , sysnode_getchar );
    dmap_insert( hmap , ".print" , sysnode_print);
    
    return hmap;
}
