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

#include "slextlib.h"
#include "logger.h"
#include "run.h"
#include "mem.h"
#include "datadump.h"

#include <stdlib.h>
#include <string.h>

#ifdef WINDOWS
#include <windows.h>
#endif
#ifdef linux
#include <dlfcn.h>
#endif

SLINT slext_get_count( RUNTIME* runtime )
{
    return runtime->current.param->count;
}

SLINT slext_get_int( RUNTIME* runtime , SLINT i )
{
    PARAM_ITEM* paramlist;
    
    if ( ( i >= 0 ) && ( runtime->current.param->count > i ) )
    {
        paramlist = &(runtime->current.param->list[i]);
        
        if ( paramlist->value != NULL )
        {
            if ( paramlist->value->type == RVT_STRING )
            {
                if ( paramlist->value->data != NULL )
                {
                    return atoi( paramlist->value->data );
                }
            }
        }
    }
    
    return 0;
}

char* slext_get_ptr( RUNTIME* runtime , SLINT i , SLINT* outsize )
{
    PARAM_ITEM* paramlist;
    
    if ( ( i >= 0 ) && ( runtime->current.param->count > i ) )
    {
        paramlist = &(runtime->current.param->list[i]);
        
        if ( paramlist->value != NULL )
        {
            if ( paramlist->value->type == RVT_STRING )
            {
                if ( paramlist->value->data != NULL )
                {
                    if ( outsize )
                    {
                        *outsize = paramlist->value->size;
                    }
                    
                    return paramlist->value->data;
                }
            }
        }
    }
    
    if ( outsize )
    {
        *outsize = 0;
    }
    
    return NULL;
}

char* slext_get_str( RUNTIME* runtime , SLINT i )
{
    SLINT strsize;
    char* str;
    
    str = slext_get_ptr( runtime , i , &strsize );
    
    if ( ( str == NULL ) || ( strsize == 0 ) )
    {
        return NULL;
    }
    
    str[strsize] = 0;
    return dup_str( str );
}

RT_VALUE* slext_get_tbl( RUNTIME* runtime , SLINT i )
{
    PARAM_ITEM* paramlist;
    
    if ( ( i >= 0 ) && ( runtime->current.param->count > i ) )
    {
        paramlist = &(runtime->current.param->list[i]);
        
        if ( paramlist->value != NULL )
        {
            if ( paramlist->value->type == RVT_TABLE )
            {
                if ( paramlist->value->data != NULL )
                {
                    return paramlist->value;
                }
            }
        }
    }
    
    return NULL;
}

SLINT slext_set_int( RUNTIME* runtime , SLINT i , SLINT value )
{
    if ( ( runtime->current.retvalue_count > i ) && ( i >= 0 ) )
    {
        unref_value( runtime , runtime->current.retvalue[i].value );
        runtime->current.retvalue[i].value = ref_value_int( runtime , value );
        return 1;
    }
    
    return 0;
}

SLINT slext_set_ptr(
    RUNTIME*    runtime ,
    SLINT       i       ,
    char*       str     ,
    SLINT       size    ,
    SLINT       bcopy
) {
    if ( ( runtime->current.retvalue_count > i ) && ( i >= 0 ) )
    {
        unref_value( runtime , runtime->current.retvalue[i].value );
        runtime->current.retvalue[i].value = ref_value_str(
            runtime ,
            str     ,
            size    ,
            bcopy
        );
        return 1;
    }
    else
    {
        if ( ! bcopy )
        {
            memfree( str );
        }
    }
    
    return 0;
}

SLINT slext_set_str( RUNTIME* runtime , SLINT i , char* str )
{
    int strsize = 0;
    
    if ( str != NULL )
    {
        strsize = strlen( str );
    }
    
    return slext_set_ptr( runtime , i , str , strsize , 1 );
}

SLINT slext_set_tbl( RUNTIME* runtime , SLINT i , RT_VALUE* pvalue )
{
    if ( ( runtime->current.retvalue_count > i ) && ( i >= 0 ) )
    {
        unref_value( runtime , runtime->current.retvalue[i].value );
        runtime->current.retvalue[i].value = ref_value( runtime , pvalue );
        return 1;
    }
    
    return 0;
}

RUNTIME* slext_tmpruntime_new(
    RUNTIME*    runtime     ,
    SLINT       retcount    ,
    SLINT       paramcount
) {
    RUNTIME* tmpruntime = memalloc_zero( sizeof( RUNTIME ) );
    memcpy( tmpruntime , runtime , sizeof( RUNTIME ) );
    tmpruntime->total_ref = 0;
    
    tmpruntime->current.param = memalloc_zero( sizeof( NODE_PARAM ) );
    
    if ( retcount > 0 )
    {
        tmpruntime->current.param->list  = memalloc_zero(
            retcount * sizeof( PARAM_ITEM )
        );
        tmpruntime->current.param->count = retcount;
    }
    
    if ( paramcount > 0 )
    {
        tmpruntime->current.retvalue = memalloc_zero(
            paramcount * sizeof( PARAM_ITEM )
        );
        tmpruntime->current.retvalue_count = paramcount;
    }
    else
    {
        tmpruntime->current.retvalue = NULL;
        tmpruntime->current.retvalue_count = 0;
    }
    
    return tmpruntime;
}

SLINT slext_tmpruntime_del( RUNTIME* runtime , RUNTIME* tmpruntime )
{
    if ( tmpruntime->current.param->count > 0 )
    {
        free_param_list_value(
            runtime ,
            tmpruntime->current.param->list ,
            tmpruntime->current.param->count
        );
        free_param_list(
            tmpruntime->current.param->list ,
            tmpruntime->current.param->count
        );
    }
    
    if ( tmpruntime->current.retvalue_count > 0 )
    {
        free_param_list_value(
            runtime ,
            tmpruntime->current.retvalue ,
            tmpruntime->current.retvalue_count
        );
        free_param_list(
            tmpruntime->current.retvalue ,
            tmpruntime->current.retvalue_count
        );
    }
    
    runtime->total_ref += tmpruntime->total_ref;
    memfree( tmpruntime->current.param );
    memfree( tmpruntime );
    return 1;
}

SLINT slext_call(
    RUNTIME*    runtime     ,
    char*       func_name   ,
    RUNTIME*    paramrt     ,
    RUNTIME*    retrt
) {
    if ( ( paramrt == NULL ) || ( retrt == NULL ) )
    {
        return 0;
    }
    
    SLINT ret = call_node(
        runtime ,
        func_name ,
        paramrt->current.retvalue ,
        paramrt->current.retvalue_count ,
        retrt->current.param->list ,
        retrt->current.param->count ,
        &runtime->errorno
    );
    return ret;
}

RT_VALUE* slext_tbl_new( SLRUNTIME runtime )
{
    return new_table_value( runtime );
}

SLINT slext_tbl_del( SLRUNTIME runtime , RT_VALUE* table )
{
    unref_value( runtime , table );
    return 1;
}

SLINT slext_tbl_get_int( SLRUNTIME runtime , RT_VALUE* table , char* key )
{
    RT_VALUE* item;
    
    if ( ( table != NULL ) && ( table->type == RVT_TABLE ) )
    {
        item = dmap_query( table->data , key );
        
        if ( item != NULL )
        {
            if ( item->type == RVT_TBL_ITEM_UNLOAD )
            {
                item = read_data( runtime , item->fpos );
            }
            
            if ( ( item->type == RVT_STRING ) && ( item->data != NULL ) )
            {
                return atoi( item->data );
            }
        }
    }
    
    return 0;
}

char* slext_tbl_get_ptr(
    SLRUNTIME   runtime ,
    RT_VALUE*   table   ,
    char*       key     ,
    SLINT*      outsize
) {
    RT_VALUE* item;
    
    if ( ( table != NULL ) && ( table->type == RVT_TABLE ) )
    {
        item = dmap_query( table->data , key );
        
        if ( item != NULL )
        {
            if ( item->type == RVT_TBL_ITEM_UNLOAD )
            {
                item = read_data( runtime , item->fpos );
            }
            
            if ( ( item->type == RVT_STRING ) && ( item->data != NULL ) )
            {
                if ( outsize )
                {
                    *outsize = item->size;
                }
                
                return item->data;
            }
        }
    }
    
    if ( outsize )
    {
        *outsize = 0;
    }
    
    return NULL;
}

RT_VALUE* slext_tbl_get_tbl( SLRUNTIME runtime , RT_VALUE* table , char* key )
{
    RT_VALUE* item;
    
    if ( ( table != NULL ) && ( table->type == RVT_TABLE ) )
    {
        item = dmap_query( table->data , key );
        
        if ( item != NULL )
        {
            if ( item->type == RVT_TBL_ITEM_UNLOAD )
            {
                item = read_data( runtime , item->fpos );
            }
            
            if ( ( item->type == RVT_TABLE ) && ( item->data != NULL ) )
            {
                return item;
            }
        }
    }
    
    return NULL;
}

SLINT slext_tbl_set_int(
    SLRUNTIME   runtime ,
    RT_VALUE*   table   ,
    char*       key     ,
    SLINT       value
) {
    RT_VALUE* item;
    
    if ( ( table != NULL ) && ( table->type == RVT_TABLE ) )
    {
        item = dmap_getanddel( table->data , key );
        
        if ( item != NULL )
        {
            unref_value( runtime , item );
        }
        
        dmap_insert( table->data , key , ref_value_int( runtime , value ) );
        return 1;
    }
    
    return 0;
}

SLINT slext_tbl_set_ptr(
    SLRUNTIME   runtime ,
    RT_VALUE*   table   ,
    char*       key     ,
    char*       str     ,
    SLINT       size    ,
    SLINT       bcopy
) {
    RT_VALUE* item;
    
    if ( ( table != NULL ) && ( table->type == RVT_TABLE ) )
    {
        item = dmap_getanddel( table->data , key );
        
        if ( item != NULL )
        {
            unref_value( runtime , item );
        }
        
        dmap_insert(
            table->data ,
            key ,
            ref_value_str( runtime , str , size , bcopy )
        );
        return 1;
    }
    
    return 0;
}

SLINT slext_tbl_set_tbl(
    SLRUNTIME   runtime ,
    RT_VALUE*   table   ,
    char*       key     ,
    RT_VALUE*   pvalue
) {
    RT_VALUE* item;
    
    if ( ( table != NULL ) && ( table->type == RVT_TABLE ) )
    {
        item = dmap_getanddel( table->data , key );
        
        if ( item != NULL )
        {
            unref_value( runtime , item );
        }
        
        dmap_insert( table->data , key , ref_value( runtime , pvalue ) );
        return 1;
    }
    
    return 0;
}

SLINT slext_tbl_set_val(
    SLRUNTIME   runtime ,
    RT_VALUE*   table   ,
    char*       key     ,
    RT_VALUE*   pvalue
) {
    RT_VALUE* item;
    
    if ( ( table != NULL ) && ( table->type == RVT_TABLE ) )
    {
        item = dmap_getanddel( table->data , key );
        
        if ( item != NULL )
        {
            unref_value( runtime , item );
        }
        
        dmap_insert( table->data , key , ref_value( runtime , pvalue ) );
        return 1;
    }
    
    return 0;
}

typedef struct _SLEXT_TBL_FOREACH_CALLBACK_PARAM
{
    SLRUNTIME       runtime;
    TABLE_CALLBACK  callback;
    void*           param;
}
SLEXT_TBL_FOREACH_CALLBACK_PARAM;

SLINT slext_tbl_foreach_callback(
    char*   key     ,
    void*   data    ,
    void*   param
) {
    SLEXT_TBL_FOREACH_CALLBACK_PARAM* pcallback_param =
                                ( SLEXT_TBL_FOREACH_CALLBACK_PARAM* )param;
    RT_VALUE* svalue = ( RT_VALUE* )data;
    
    if ( svalue != NULL )
    {
        if ( svalue->type == RVT_TBL_ITEM_UNLOAD )
        {
            svalue = read_data( pcallback_param->runtime , svalue->fpos );
        }
    }
    
    return pcallback_param->callback( key , svalue , pcallback_param->param );
}

SLINT slext_tbl_foreach(
    SLRUNTIME       runtime     ,
    RT_VALUE*       table       ,
    TABLE_CALLBACK  callback    ,
    void* param
) {
    SLEXT_TBL_FOREACH_CALLBACK_PARAM callback_param = { 0 };
    
    if ( ( table != NULL )
        && ( table->type == RVT_TABLE )
        && ( callback != NULL ) )
    {
        callback_param.runtime = runtime;
        callback_param.callback = callback;
        callback_param.param = param;
        dmap_foreach(
            table->data ,
            slext_tbl_foreach_callback ,
            &callback_param
        );
        return 1;
    }
    
    return 0;
}

SLINT slext_val_get_type( RT_VALUE* svalue )
{
    if ( ( svalue != NULL ) && ( svalue->data != NULL ) )
    {
        if ( svalue->type == RVT_STRING )
        {
            return SLVALUE_TYPE_STR;
        }
        else if ( svalue->type == RVT_TABLE )
        {
            return SLVALUE_TYPE_TBL;
        }
    }
    
    return SLVALUE_TYPE_NUL;
}

SLINT slext_val_get_int( RT_VALUE* svalue )
{
    if ( ( svalue != NULL ) && ( svalue->data != NULL ) )
    {
        if ( svalue->type == RVT_STRING )
        {
            return atoi( svalue->data );
        }
    }
    
    return 0;
}

char* slext_val_get_ptr( RT_VALUE* svalue , SLINT* outsize )
{
    if ( outsize )
    {
        *outsize = 0;
    }
    
    if ( ( svalue != NULL ) && ( svalue->data != NULL ) )
    {
        if ( svalue->type == RVT_STRING )
        {
            *outsize = svalue->size;
            return svalue->data;
        }
    }
    
    return NULL;
}

RT_VALUE* slext_val_get_tbl( RT_VALUE* svalue )
{
    if ( ( svalue != NULL ) && ( svalue->data != NULL ) )
    {
        if ( svalue->type == RVT_TABLE )
        {
            return svalue;
        }
    }
    
    return NULL;
}

SLINT slext_reg_func( MODULE* module , char* func_name , EXTFUNC func )
{
    if ( ( module == NULL ) || ( func_name == NULL ) || ( func == NULL ) )
    {
        return 0;
    }
    
    NODE* node;
    
    node = module->node;
    
    while ( node )
    {
        if ( node->next == NULL )
        {
            break;
        }
        
        node = node->next;
    }
    
    if ( node == NULL )
    {
        node = memalloc_zero( sizeof( NODE ) );
        module->node = node;
    }
    else
    {
        node->next = memalloc_zero( sizeof( NODE ) );
        node = node->next;
    }
    
    strncpy(
        node->name ,
        func_name ,
        MIN( MAX_WORD_NAME_LEN - 1 , strlen( func_name ) )
    );
    node->extfunc = func;
    node->type = NT_EXTNODE;
    return 1;
}

void slext_free_str( RUNTIME* runtime , char* str )
{
    if ( str )
    {
        memfree( str );
    }
    
    return;
}

EXTLIB_FUNC g_extlib_func = {
    slext_get_count ,
    slext_get_int ,
    slext_get_ptr ,
    slext_get_str ,
    slext_get_tbl ,
    slext_set_int ,
    slext_set_ptr ,
    slext_set_str ,
    slext_set_tbl ,
    
    slext_tmpruntime_new ,
    slext_tmpruntime_del ,
    slext_call ,
    
    slext_tbl_new ,
    slext_tbl_del ,
    slext_tbl_get_int ,
    slext_tbl_get_ptr ,
    slext_tbl_get_tbl ,
    slext_tbl_set_int ,
    slext_tbl_set_ptr ,
    slext_tbl_set_tbl ,
    slext_tbl_set_val ,
    slext_tbl_foreach ,
    
    slext_val_get_type ,
    slext_val_get_int ,
    slext_val_get_ptr ,
    slext_val_get_tbl ,
    
    slext_reg_func ,
    slext_free_str
};

SLINT slext_load_module( MODULE* module , char* file_path )
{
    REG_EXTLIB reg_extlib = NULL;
    
#ifdef WINDOWS
    module->handle = LoadLibrary( file_path );
    
    if ( module->handle != NULL )
    {
        reg_extlib = ( REG_EXTLIB )GetProcAddress(
            module->handle ,
            "reg_extlib"
        );
    }
#endif
#ifdef linux
    module->handle = dlopen( file_path , RTLD_NOW );
    
    if ( module->handle )
    {
        reg_extlib = ( REG_EXTLIB )dlsym( module->handle , "reg_extlib" );
    }
#endif
    
    if ( reg_extlib != NULL )
    {
        if ( reg_extlib( module , g_extlib_func ) )
        {
            module->modtype = MT_EXTMOD;
            return 1;
        }
    }
    
    return 0;
}

SLINT slext_unload_module( MODULE* module )
{
    UNREG_EXTLIB unreg_extlib = NULL;
    
    if ( ( module != NULL )
        && ( module->modtype == MT_EXTMOD )
        && ( module->handle != NULL ) )
    {
#ifdef WINDOWS
        unreg_extlib = ( UNREG_EXTLIB )GetProcAddress(
            module->handle ,
            "unreg_extlib"
        );
#endif
#ifdef linux
        unreg_extlib = ( UNREG_EXTLIB )dlsym( module->handle , "unreg_extlib" );
#endif
        if ( unreg_extlib != NULL )
        {
            if ( unreg_extlib( module ) )
            {
            }
        }
#ifdef WINDOWS
        FreeLibrary( module->handle );
#endif
#ifdef linux
        dlclose( module->handle );
#endif
        module->handle = NULL;
    }
    return 1;
}
