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
#include "slanglex.h"
#include "memalloc.h"
#include "run.h"
#include "datadump.h"
#include "mem.h"
#include "eval.h"
#include "sysnode.h"
#include "text_conv.h"
#include "slextlib.h"

#include <string.h>
#include <stdlib.h>

#ifdef WINDOWS
    #include <windows.h>
#else
    #include <dlfcn.h>
    #include <iconv.h>
    #include <errno.h>
#endif

RUNTIME* new_runtime( char* basepath )
{
    RUNTIME*    ret;
    int         charset;
    
    charset = CL_UTF8;
    ret = memalloc_zero( sizeof( RUNTIME ) );
    mem_init( ret );
    
    ret->root = NULL;
    ret->module = dmap_init( 10000 );
    ret->sysnode = init_sysnode_map();
    strncpy(
        ret->basepath ,
        basepath ,
        MIN( strlen( basepath ) , MAX_NAME_LEN - 2 )
    );
    ret->src_ext_filename = convert_code(
        CL_UTF8 ,
        SRC_EXT_FILENAME ,
        charset
    );
    ret->bin_ext_filename = convert_code(
        CL_UTF8 ,
        CODE_EXT_FILENAME ,
        charset
    );
    
    SLINT n = strlen( ret->basepath );
    
    if ( ret->basepath[n - 1] != PS )
    {
        ret->basepath[n] = PS;
        ret->basepath[n + 1] = 0;
    }
    
    if ( dmp_init( ret ) == 0 )
    {
        release_runtime( ret );
        return NULL;
    }
    
    return ret;
}

SLINT release_module( RUNTIME* runtime , MODULE* module )
{
    if ( module == NULL )
    {
        return 1;
    }
    
    if ( module->nodemap )
    {
        dmap_release( module->nodemap , NULL , NULL );
    }
    
    if ( module->node != NULL )
    {
        free_node_data( runtime , module->node );
        free_node( module->node );
    }
    
    if ( module->handle )
    {
        slext_unload_module( module );
    }
    
    memfree( module );
    return 1;
}

SLINT release_runtime_callback(
    bchar*      key         ,
    void*       data_ptr    ,
    void*       runtime
) {
    MODULE* module = ( MODULE* )data_ptr;
    
    if ( module )
    {
        release_module( ( RUNTIME* )runtime , module );
    }
    
    return 1;
}

SLINT release_runtime( RUNTIME* runtime )
{
    dmp_release( runtime );
    
    if ( runtime != NULL )
    {
        dmap_release( runtime->module , release_runtime_callback , runtime );
        dmap_release( runtime->sysnode , NULL , NULL );
    }
    
    memfree( runtime->src_ext_filename );
    memfree( runtime->bin_ext_filename );
    
    mem_release( runtime );
    return 1;
}

void get_module_fullname(
    RUNTIME*    runtime     ,
    bchar*      full_name   ,
    bchar*      name
) {
    char* tmp_str;
    
    if ( runtime->current.module != NULL )
    {
        strcpy( full_name , runtime->current.module->name );
        
        bchar* p = strrchr( full_name , '.' );
        
        if ( p )
        {
            *p = 0;
            strcat( full_name , name );
        }
        else
        {
            strcpy( full_name , name );
        }
    }
    else
    {
        strcpy( full_name , name );
    }
}

/* full_name is CL_UTF8 */
int load_module_form_file(
    RUNTIME*    runtime     ,
    MODULE*     module      ,
    bchar*      full_name
) {
    SLINT       i;
    char        file_path[MAX_NAME_LEN * 4];
    char*       tmp_str;
    
    strcpy( file_path , runtime->basepath );
    
    if ( full_name[0] == '.' )
    {
        full_name++;
    }
    
    tmp_str = dup_str( full_name );
    
    for ( i = 0; i < strlen( tmp_str ); i++ )
    {
        if ( tmp_str[i] == '.' )
        {
            tmp_str[i] = PS;
        }
    }
    
    strcat( file_path , tmp_str );
    memfree( tmp_str );
    
    tmp_str = &(file_path[strlen( file_path )]);
    
    strcpy( tmp_str , runtime->bin_ext_filename );
    
    module->modtype = MT_SCMOD;
    module->node = load_node( file_path );
    
    if ( module->node == NULL )
    {
        strcpy( tmp_str , runtime->src_ext_filename );
        module->node = parse_source_file( file_path );
        
        if ( module->node == NULL )
        {
            *tmp_str = 0;
            strcpy( tmp_str , LIB_EXT_FILENAME );
            
            if ( ! slext_load_module( module , file_path ) )
            {
                return 0;
            }
        }
    }
    
    tmp_str = dup_str( full_name );
    strncpy(
        module->name ,
        tmp_str ,
        MIN( strlen( tmp_str ) , MAX_NAME_LEN - 1 )
    );
    return 1;
}

void set_node_map( MODULE* module )
{
    int     nodenum = 0;
    NODE*   node = module->node;
    
    while ( node )
    {
        nodenum++;
        node = node->next;
    }
    
    if ( nodenum == 0 )
    {
        nodenum = 5;
    }
    
    if ( module->nodemap )
    {
        dmap_release( module->nodemap , NULL , NULL );
    }
    
    module->nodemap = dmap_init( nodenum );
    node = module->node;
    
    while ( node )
    {
        dmap_insert( module->nodemap , node->name , node );
        node = node->next;
    }
}

MODULE* load_module( RUNTIME* runtime , bchar* name )
{
    bchar       full_name[MAX_NAME_LEN];
    MODULE*     module = NULL;
    
    memset( full_name , 0 , MAX_NAME_LEN );
    
    module = memalloc_zero( sizeof( MODULE ) );
    
    /* find module from current directory */
    get_module_fullname( runtime , full_name , name );
    
    if ( ! load_module_form_file( runtime , module , full_name ) )
    {
        if ( strcmp( full_name , name ) != 0 )
        {
            /* find module from root */
            if ( ! load_module_form_file( runtime , module , name ) )
            {
                release_module( runtime , module );
                return NULL;
            }
            
            strcpy( full_name , name );
        }
        else
        {
            release_module( runtime , module );
            return NULL;
        }
    }
    
    release_module( runtime , dmap_getanddel( runtime->module , full_name ) );
    set_node_map( module );
    dmap_insert( runtime->module , full_name , module );
    return module;
}

MODULE* load_module_from_node(
    RUNTIME*    runtime ,
    bchar*      name    ,
    NODE*       node
) {
    MODULE* module = NULL;
    bchar full_name[MAX_NAME_LEN];
    
    get_module_fullname( runtime , full_name , name );
    module = dmap_getanddel( runtime->module , full_name );
    
    if ( module )
    {
        release_module( runtime , module );
    }
    
    module = memalloc_zero( sizeof( MODULE ) );
    strncpy(
        module->name ,
        full_name ,
        MIN( strlen( full_name ) , MAX_NAME_LEN - 1 )
    );
    module->node = node;
    
    if ( module->node == NULL )
    {
        release_module( runtime , module );
        return NULL;
    }
    
    set_node_map( module );
    dmap_insert( runtime->module , full_name , module );
    return module;
}

SLINT copy_node_param( NODE_PARAM* param_dest , NODE_PARAM* param_temp )
{
    SLINT   i;
    SLINT   size;
    
    if ( param_temp->count > 0 )
    {
        size = sizeof( PARAM_ITEM ) * param_temp->count;
        param_dest->count = param_temp->count;
        param_dest->list = memalloc_zero( size );
        memcpy( param_dest->list , param_temp->list , size );
        
        for ( i = 0; i < param_dest->count; i++ )
        {
            param_dest->list[i].value = NULL;
        }
    }
    else
    {
        param_dest->count = 0;
        param_dest->list = NULL;
    }
    
    return 1;
}

SLINT run_code( RUNTIME* runtime , CODE* code , SLINT* errorno );

SLINT call_user_node(
    RUNTIME*    runtime             ,
    bchar*      mod_node_name       ,
    PARAM_ITEM* paramvalue          ,
    SLINT       paramvalue_count    ,
    PARAM_ITEM* retvalue            ,
    SLINT       retvalue_count      ,
    SLINT*      errorno
) {
    SLINT       ret;
    bchar*      node_name;
    bchar       module_name[MAX_NAME_LEN * 2];
    MODULE*     module;
    NODE*       node;
    RT_VALUE*   dmpval;
    SLINT       i;
    bchar       full_name[MAX_NAME_LEN];
    NODE_PARAM  node_param;
    
    if ( strlen( mod_node_name ) >= MAX_NAME_LEN * 2 )
    {
        *errorno = __LINE__;
        return RET_ERROR;
    }
    
    if ( mod_node_name[0] != '.' )
    {
        *errorno = __LINE__;
        return RET_NO_NODE;
    }
    
    memset( module_name , 0 , MAX_NAME_LEN * 2 );
    strcpy( module_name , mod_node_name );
    
    node_name = strrchr( module_name , '.' );
    
    if ( node_name == NULL )
    {
        *errorno = __LINE__;
        return RET_ERROR;
    }
    
    *node_name = 0;
    node_name++;
    
    if ( strlen( node_name ) == 0 )
    {
        *errorno = __LINE__;
        return RET_NO_NODE;
    }
    
    if ( module_name[0] != 0 )
    {
        module = dmap_query( runtime->module , module_name );
        
        if ( ! module )
        {
            get_module_fullname( runtime , full_name , module_name );
            module = dmap_query( runtime->module , full_name );
            
            if ( ! module )
            {
                module = load_module( runtime , module_name );
            }
        }
    }
    else
    {
        module = runtime->current.module;
    }
    
    if ( ! module )
    {
        *errorno = __LINE__;
        log_error( "load %s error" , mod_node_name );
        return RET_ERROR;
    }
    
    if ( module->nodemap )
    {
        node = dmap_query( module->nodemap , node_name );
    }
    else
    {
        node = module->node;
        
        while ( node )
        {
            if ( strcmp( node->name , node_name ) == 0 )
            {
                break;
            }
            
            node = node->next;
        }
    }
    
    if ( node == NULL )
    {
        *errorno = __LINE__;
        return RET_NO_NODE;
    }
    
    memcpy( &runtime->caller , &runtime->current , sizeof( CALLCONTEXT ) );
    
    if ( ( node->type == NT_EXTNODE ) && ( node->extfunc != NULL ) )
    {
        memset( &node_param , 0, sizeof( NODE_PARAM ) );
        node_param.count = paramvalue_count;
        
        if ( paramvalue_count > 0 )
        {
            node_param.list = paramvalue;
        }
        
        runtime->current.module = module;
        runtime->current.node = node;
        runtime->current.param = &node_param;
        runtime->current.retvalue = retvalue;
        runtime->current.retvalue_count = retvalue_count;
        
        ret = node->extfunc( runtime );
        return ret;
    }
    
    if ( ( node->type == NT_EXTNODE )
        || ( node->body == NULL )
        || ( node->body->code == NULL ) )
    {
        *errorno = __LINE__;
        return RET_ERROR;
    }
    
    if ( ( node->param != NULL )
        && ( node->param->count > 0 ) )
    {
        if ( node->status != NS_LOADDATA )
        {
            /* read old data from file */
            for ( i = node->param->count1 + node->param->count2;
                i < node->param->count; i++ )
            {
                dmpval = dmp_get_rtvalue(
                    runtime ,
                    module->name ,
                    node->name ,
                    node->param->list[i].name
                );
                
                if ( dmpval )
                {
                    node->param->list[i].value = dmpval;
                }
            }
            
            node->status = NS_LOADDATA;
        }
        
        if ( node->param->count1 > 0 )
        {
            for ( i = 0; i < node->param->count1; i++ )
            {
                if ( i < paramvalue_count )
                {
                    unref_value( runtime , node->param->list[i].value );
                    node->param->list[i].value = ref_value(
                        runtime ,
                        paramvalue[i].value
                    );
                }
            }
        }
    }
    
    runtime->current.module = module;
    runtime->current.node = node;
    runtime->current.param = node->param;
    runtime->current.retvalue = retvalue;
    runtime->current.retvalue_count = retvalue_count;
    
    ret = run_code( runtime , node->body->code , errorno );
    
    for ( i = 0; i < node->param->count1; i++ )
    {
        unref_value( runtime , node->param->list[i].value );
        node->param->list[i].value = NULL;
    }
    
    if ( ( node->param != NULL ) && ( node->param->count > 0 ) )
    {
        /* save new data */
        for ( i = node->param->count1 + node->param->count2;
            i < node->param->count; i++ )
        {
            if ( ! dmp_set_rtvalue(
                runtime ,
                module->name ,
                node->name ,
                node->param->list[i].name ,
                node->param->list[i].value
            ) ) {
                log_error( "save data error[%s.%s-%s]" ,
                    module->name , node->name , node->param->list[i].name
                );
            }
        }
    }
    
    return ret;
}

SLINT call_node(
    RUNTIME*        runtime             ,
    bchar*          mod_node_name       ,
    PARAM_ITEM*     paramvalue          ,
    SLINT           paramvalue_count    ,
    PARAM_ITEM*     retvalue            ,
    SLINT           retvalue_count      ,
    SLINT*          errorno
) {
    SLINT       ret;
    SLINT       i;
    CALLCONTEXT oldcontext;
    CALLCONTEXT oldcallercontext;
    NODE_PARAM  node_param;
    
    memcpy( &oldcontext , &runtime->current , sizeof( CALLCONTEXT ) );
    memcpy( &oldcallercontext , &runtime->caller , sizeof( CALLCONTEXT ) );
    
    ret = call_user_node(
        runtime             ,
        mod_node_name       ,
        paramvalue          ,
        paramvalue_count    ,
        retvalue            ,
        retvalue_count      ,
        errorno
    );
    
    if ( ret == RET_NO_NODE )
    {
        memset( &node_param , 0 , sizeof( NODE_PARAM ) );
        
        node_param.count = paramvalue_count;
        
        if ( paramvalue_count > 0 )
        {
            node_param.list = paramvalue;
        }
        
        runtime->current.param = &node_param;
        runtime->current.retvalue = retvalue;
        runtime->current.retvalue_count = retvalue_count;
        
        ret = call_sys_node( runtime , mod_node_name );
    }
    
    if ( ret == RET_RETURN )
    {
        ret = RET_OK;
    }
    
    memcpy( &runtime->current , &oldcontext , sizeof( CALLCONTEXT ) );
    memcpy( &runtime->caller , &oldcallercontext , sizeof( CALLCONTEXT ) );
    
    if ( ret == RET_NO_NODE )
    {
        log_error( "call error, '%s' not found" , mod_node_name );
        ret = RET_ERROR;
    }
    else if ( ret != RET_OK )
    {
        log_error( "call '%s' error(%d)" , mod_node_name , ret );
    }
    
    return ret;
}

bchar* get_cvalue_string( NODE_PARAM* param , CODE_VALUE* value )
{
    PARAM_ITEM* item = param->list;
    bchar*      ret = NULL;
    
    if ( value->type == CVT_VARIABLE )
    {
        item = &(item[value->index]);
        
        if ( item->value != NULL )
        {
            if ( item->value->type == RVT_STRING )
            {
                if ( item->value->data != NULL )
                {
                    ret = item->value->data;
                }
            }
        }
    }
    else if ( value->type == CVT_CONST )
    {
        ret = value->data;
    }
    
    if ( ret == NULL )
    {
        return NULL;
    }
    
    return ret;
}

bchar* eval_format( RUNTIME* runtime , CODE_VALUE* value )
{
    bchar* ret = get_cvalue_string( runtime->current.param , value );
    return strformat(
        runtime->current.param ,
        ret
    );
}

typedef struct _FOREACH_CALLBACK_PARM
{
    RUNTIME*        runtime;
    CODE*           code;
    SLINT*          errorno;

    PARAM_ITEM*     iterator;
    PARAM_ITEM*     iterator_value;
    SLINT           step;
    SLINT           stepi;
    SLINT           ret;
}
FOREACH_CALLBACK_PARM;

SLINT foreach_callback( bchar* name , void* data , void* param )
{
    SLINT                   ret;
    FOREACH_CALLBACK_PARM*  run_code_param = ( FOREACH_CALLBACK_PARM* )param;
    
    if ( run_code_param->step > 1 )
    {
        run_code_param->stepi++;
        
        if ( run_code_param->stepi > run_code_param->step )
        {
            run_code_param->stepi = 1;
        }
        
        if ( run_code_param->stepi != 1 )
        {
            return 1;
        }
    }
    
    unref_value( run_code_param->runtime , run_code_param->iterator->value );
    run_code_param->iterator->value = ref_value_str(
        run_code_param->runtime ,
        name ,
        strlen( name ) ,
        1
    );
    
    if ( run_code_param->iterator_value )
    {
        unref_value(
            run_code_param->runtime ,
            run_code_param->iterator_value->value
        );
        
        run_code_param->iterator_value->value = ref_value(
            run_code_param->runtime ,
            data
        );
    }
    
    ret = run_code(
        run_code_param->runtime ,
        run_code_param->code ,
        run_code_param->errorno
    );
    
    if ( ret == 0 )
    {
        run_code_param->ret = ret;
        return 0;
    }
    else if ( ret == 2 )
    {
        run_code_param->ret = ret;
        return 0;
    }
    
    return 1;
}

SLINT run_foreach( RUNTIME* runtime , CODE* code , SLINT* errorno )
{
    SLINT       ret;
    CODE_VALUE* val_array;
    CODE_VALUE* valueptr;
    CODE*       foreachcode;
    PARAM_ITEM* paramarray;
    PARAM_ITEM* paramptr;
    PARAM_ITEM* iterator;
    PARAM_ITEM* iterator_value;
    SLINT       step;
    bchar       stepchar;
    bchar*      stepstr;
    bchar*      stepfind;
    bchar*      pstr;
    SLINT       str_size;
    SLINT       pos;
    SLINT       c;
    SLINT       start_pos;
    SLINT       i;
    
    if ( code->val_array_count < 3 )
    {
        *errorno = __LINE__;
        return RET_ERROR;
    }
    
    val_array = code->value;
    paramarray = runtime->current.param->list;
    step = 1;
    iterator_value = NULL;
    
    valueptr = &(val_array[code->val_array_count-1]);
    
    if ( valueptr->type != CVT_CODE )
    {
        *errorno = __LINE__;
        return RET_ERROR;
    }
    
    foreachcode = valueptr->data;
    
    valueptr = &(val_array[0]);
    
    if ( valueptr->type != CVT_VARIABLE )
    {
        *errorno = __LINE__;
        return RET_ERROR;
    }
    
    iterator = &(paramarray[valueptr->index]);
    
    if ( code->val_array_count >= 4 )
    {
        valueptr = &(val_array[1]);
        
        if ( valueptr->type != CVT_VARIABLE )
        {
            *errorno = __LINE__;
            return RET_ERROR;
        }
        
        iterator_value = &(paramarray[valueptr->index]);
    }
    
    if ( code->val_array_count == 5 )
    {
        valueptr = &(val_array[2]);
        
        if ( valueptr->type != CVT_CONST )
        {
            *errorno = __LINE__;
            return RET_ERROR;
        }
        
        pos = 0;
        c = shift_word_skip_space(
            valueptr->data ,
            strlen( valueptr->data ) ,
            &pos
        );
        
        if ( c == '*' )
        {
            step = -1;  /* space as separator */
            stepchar = ' ';
        }
        else if ( c == '-' )
        {
            step = -2;  /* custom separator (line feed) */
            stepchar = '\n';
        }
        else if ( ! is_number( c ) )
        {
            step = -2;  /* custom delimiter */
            stepchar = c;
            stepstr = valueptr->data;
            start_pos = strlen( stepstr );
            
            if ( start_pos > 1 )
            {
                step = -3;  /* custom delimited string */
            }
        }
        else
        {
            step = atoi( valueptr->data );
            
            if ( step <= 0 )
            {
                step = 1;
            }
        }
    }
    
    pstr = NULL;
    str_size = 0;
    
    valueptr = &(val_array[code->val_array_count - 2]);
    
    if ( valueptr->type == CVT_CONST )
    {
        pstr = valueptr->data;
        str_size = strlen( pstr );
    }
    else if ( valueptr->type == CVT_VARIABLE )
    {
        paramptr = &(paramarray[valueptr->index]);
        
        if ( ( paramptr->value != NULL ) && ( paramptr->value->data != NULL ) )
        {
            if ( paramptr->value->type == RVT_STRING )
            {
                pstr = paramptr->value->data;
                str_size = paramptr->value->size;
            }
            else if ( paramptr->value->type == RVT_TABLE )
            {
                if ( step <= 0 )
                {
                    step = 1;
                }
                
                FOREACH_CALLBACK_PARM foreach_param = { 0 };
                foreach_param.runtime = runtime;
                foreach_param.code = foreachcode;
                foreach_param.errorno = errorno;
                foreach_param.iterator = iterator;
                foreach_param.iterator_value = iterator_value;
                foreach_param.step = step;
                foreach_param.stepi = 0;
                foreach_param.ret = 1;
                
                dmap_foreach(
                    paramptr->value->data ,
                    foreach_callback ,
                    &foreach_param
                );
                
                if ( foreach_param.ret != RET_OK )
                {
                    return foreach_param.ret;
                }
            }
        }
    }
    
    if ( ( pstr != NULL ) && ( str_size > 0 ) )
    {
        ret = 1;
        
        if ( step <= 0 )
        {
            pos = 0;
            i = 0;
            
            if ( step == -1 ) /* space as separator */
            {
                while ( TRUE )
                {
                    c = shift_word_skip_space(
                        pstr ,
                        str_size ,
                        &pos
                    );
                    
                    if ( c == 0 )
                    {
                        break;
                    }
                    
                    start_pos = pos - 1;
                    
                    while ( ! is_space(
                        c = shift_word(
                            pstr ,
                            str_size ,
                            &pos
                        )
                    ) && ( c != 0 ) );
                    
                    i++;
                    
                    if ( iterator_value )
                    {
                        unref_value( runtime , iterator_value->value );
                        iterator_value->value = ref_value_str(
                            runtime ,
                            pstr + start_pos ,
                            pos - 1 - start_pos ,
                            1
                        );
                    }
                    
                    unref_value( runtime , iterator->value );
                    iterator->value = ref_value_int( runtime , i );
                    
                    ret = run_code( runtime , foreachcode , errorno );
                    
                    if ( ( ret == 0 ) || ( ret == 2 ) )
                    {
                        break;
                    }
                }
            }
            else if ( step == -2 )    /* custom delimiter */
            {
                while ( TRUE )
                {
                    start_pos = pos;
                    
                    while ( ( ( c = shift_word(
                        pstr ,
                        str_size ,
                        &pos
                    ) ) != stepchar ) && c != 0 );
                    
                    if ( pos <= start_pos )
                    {
                        break;
                    }
                    
                    i++;
                    
                    if ( iterator_value )
                    {
                        unref_value( runtime , iterator_value->value );
                        
                        if ( c == 0 )
                        {
                            iterator_value->value = ref_value_str(
                                runtime ,
                                pstr + start_pos ,
                                pos - start_pos ,
                                1
                            );
                        }
                        else
                        {
                            iterator_value->value = ref_value_str(
                                runtime ,
                                pstr + start_pos ,
                                pos - 1 - start_pos ,
                                1
                            );
                        }
                    }
                    
                    unref_value( runtime , iterator->value );
                    iterator->value = ref_value_int( runtime , i );
                    
                    ret = run_code( runtime , foreachcode , errorno );
                    
                    if ( ( ret == 0 ) || ( ret == 2 ) )
                    {
                        break;
                    }
                }
            }
            else if ( step == -3 )  /* custom delimited string */
            {
                while ( TRUE )
                {
                    stepfind = strstr( pstr , stepstr );
                    
                    if ( stepfind == NULL )
                    {
                        pos = str_size;
                        str_size = 0;
                    }
                    else
                    {
                        pos = stepfind - pstr;
                        str_size -= ( pos + start_pos );
                    }
                    
                    i++;
                    
                    if ( iterator_value )
                    {
                        unref_value( runtime , iterator_value->value );
                        iterator_value->value = ref_value_str(
                            runtime ,
                            pstr ,
                            pos ,
                            1
                        );
                    }
                    
                    unref_value( runtime , iterator->value );
                    iterator->value = ref_value_int( runtime , i );
                    
                    ret = run_code( runtime , foreachcode , errorno );
                    
                    if ( ( ret == 0 ) || ( ret == 2 ) )
                    {
                        break;
                    }
                    
                    if ( str_size > 0 )
                    {
                        pstr = stepfind + start_pos;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        else
        {
            pos = 0;
            
            while ( pos < str_size )
            {
                start_pos = pos;
                
                for ( i = 0; i < step; i++ )
                {
                    c = shift_word(
                        pstr ,
                        str_size ,
                        &pos
                    );
                }
                
                if ( iterator_value )
                {
                    unref_value( runtime , iterator_value->value );
                    iterator_value->value = ref_value_str(
                        runtime ,
                        pstr + start_pos ,
                        pos - start_pos ,
                        1
                    );
                }
                
                unref_value( runtime , iterator->value );
                iterator->value = ref_value_int( runtime , pos );
                
                ret = run_code( runtime , foreachcode , errorno );
                
                if ( ( ret == 0 ) || ( ret == 2 ) )
                {
                    break;
                }
            }
        }
        
        if ( ret != RET_OK )
        {
            return ret;
        }
    }
    
    return RET_OK;
}

RT_VALUE* get_sub_string(
    RUNTIME*    runtime     ,
    bchar*      str         ,
    SLINT       str_size    ,
    bchar*      key
) {
    RT_VALUE*   ret;
    SLINT       pos;
    SLINT       start_pos;
    SLINT       end_pos;
    SLINT       key_len;
    bchar       c;
    SLINT       word_len;
    bchar*      keystart;
    bchar*      keyend;
    SLINT       nkeyend;
    SLINT       nkeystart;
    
    keystart = NULL;
    keyend = NULL;
    pos = 0;
    key_len = strlen( key );
    c = shift_word_skip_space( key , key_len , &pos );
    
    if ( c == 0 )
    {
        return NULL;
    }
    
    start_pos = pos - 1;
    
    if ( c != '-' )
    {
        pos--;
    }
    
    word_len = shift_namestr( key , key_len , &pos );
    
    if ( word_len == 0 )
    {
        return NULL;
    }
    
    keystart = memalloc_zero( word_len + 1 );
    memcpy( keystart , key + start_pos , word_len );
    
    c = shift_word_skip_space( key , key_len , &pos );
    
    if ( c == '~' )
    {
        c = shift_word_skip_space( key , key_len , &pos );
        
        if ( c != 0 )
        {
            start_pos = pos - 1;
            
            if ( c != '-' )
            {
                pos--;
            }
            
            word_len = shift_namestr( key , key_len , &pos );
            
            if ( word_len > 0 )
            {
                keyend = memalloc_zero( word_len + 1 );
                memcpy( keyend , key + start_pos , word_len );
            }
        }
    }
    
    nkeystart = atoi( keystart );
    memfree( keystart );
    nkeystart = ( nkeystart ) % ( str_size + 1 );
    
    if ( nkeystart < 0 )
    {
        nkeystart += str_size;
    }
    else if ( nkeystart == 0 )
    {
        nkeystart += str_size - 1;
    }
    else
    {
        nkeystart--;
    }
    
    if ( keyend )
    {
        nkeyend = atoi( keyend );
        memfree( keyend );
        nkeyend = ( nkeyend ) % ( str_size + 1 );
        
        if ( nkeyend < 0 )
        {
            nkeyend += str_size;
        }
        else if ( nkeyend == 0 )
        {
            nkeyend += str_size - 1;
        }
        else
        {
            nkeyend--;
        }
        
        if ( nkeyend < nkeystart )
        {
            return NULL;
        }
        
        if ( nkeyend >= str_size )
        {
            nkeyend = str_size - 1;
        }
    }
    else
    {
        nkeyend = nkeystart;
    }
    
    if ( nkeystart >= str_size )
    {
        return NULL;
    }
    
    return ref_value_str(
        runtime ,
        str + nkeystart ,
        (nkeyend - nkeystart) + 1 ,
        1
    );
}

SLINT run_equ( RUNTIME* runtime , CODE* code , SLINT* errorno )
{
    CODE_VALUE* val_array;
    PARAM_ITEM* paramarray;
    PARAM_ITEM* dest;
    PARAM_ITEM* src;
    
    val_array = code->value;
    paramarray = runtime->current.param->list;
    
    if ( val_array[0].type == CVT_VARIABLE )
    {
        dest = &(paramarray[val_array[0].index]);
        
        if ( val_array[1].type == CVT_VARIABLE )
        {
            src = &(paramarray[val_array[1].index]);
            
            if ( dest->value != src->value )
            {
                unref_value( runtime , dest->value );
                dest->value = ref_value( runtime , src->value );
            }
        }
        else if ( val_array[1].type == CVT_CONST )
        {
            unref_value( runtime , dest->value );
            dest->value = ref_value_str(
                runtime ,
                val_array[1].data ,
                strlen( val_array[1].data ) ,
                1
            );
        }
        else if ( val_array[1].type == CVT_INIT )
        {
            unref_value( runtime , dest->value );
            dest->value = NULL;
        }
    }
    else
    {
        *errorno = __LINE__;
        return RET_ERROR;
    }
    
    return RET_OK;
}

SLINT run_eval( RUNTIME* runtime , CODE* code , SLINT* errorno )
{
    bchar*      evalres = NULL;
    CODE_VALUE* val_array;
    PARAM_ITEM* paramarray;
    PARAM_ITEM* dest;
    PARAM_ITEM* src;
    
    val_array = code->value;
    paramarray = runtime->current.param->list;
    
    if ( val_array[0].type == CVT_VARIABLE )
    {
        dest = &(paramarray[val_array[0].index]);
        
        if ( val_array[1].type == CVT_VARIABLE )
        {
            src = &(paramarray[val_array[1].index]);
            
            if ( ( src->value != NULL )
                && ( src->value->type == RVT_STRING )
                && ( src->value->data != NULL ) )
            {
                evalres = evaluate(
                    runtime->current.param ,
                    src->value->data
                );
            }
        }
        else if ( val_array[1].type == CVT_CONST )
        {
            evalres = evaluate(
                runtime->current.param ,
                val_array[1].data
            );
        }
        else if ( val_array[1].type == CVT_EVAL )
        {
            evalres = run_evaluate(
                runtime->current.param ,
                val_array[1].data
            );
        }
    }
    
    if ( evalres == NULL )
    {
        *errorno = __LINE__;
        return RET_ERROR;
    }
    
    unref_value( runtime , dest->value );
    dest->value = ref_value_str( runtime , evalres , strlen( evalres ) , 0 );
    return RET_OK;
}

SLINT run_format( RUNTIME* runtime , CODE* code , SLINT* errorno )
{
    bchar*      evalres = NULL;
    CODE_VALUE* val_array;
    PARAM_ITEM* paramarray;
    PARAM_ITEM* dest;
    
    val_array = code->value;
    paramarray = runtime->current.param->list;
    
    if ( val_array[0].type == CVT_VARIABLE )
    {
        dest = &(paramarray[val_array[0].index]);
        evalres = eval_format( runtime , &val_array[1] );
    }
    
    if ( evalres == NULL )
    {
        *errorno = __LINE__;
        return RET_ERROR;
    }
    
    unref_value( runtime , dest->value );
    dest->value = ref_value_str( runtime , evalres , strlen( evalres ) , 0 );
    return RET_OK;
}

SLINT run_if( RUNTIME* runtime , CODE* code , SLINT* errorno )
{
    bchar*      evalres = NULL;
    SLINT       evalrescode = 0;
    CODE_VALUE* val_array;
    CODE_VALUE* valueptr;
    SLINT       subret;
    
    PARAM_ITEM* paramarray;
    PARAM_ITEM* evalitem;
    SLINT       i;
    
    val_array = code->value;
    paramarray = runtime->current.param->list;
    
    i = 0;
    
    while ( i < code->val_array_count )
    {
        evalres = NULL;
        valueptr = &(val_array[i]);
        
        if ( valueptr->type != CVT_CODE )
        {
            if ( valueptr->type == CVT_VARIABLE )
            {
                evalitem = &(paramarray[valueptr->index]);
                
                if ( ( evalitem->value != NULL )
                    && ( evalitem->value->type == RVT_STRING )
                    && ( evalitem->value->data != NULL ) )
                {
                    evalres = evaluate(
                        runtime->current.param ,
                        evalitem->value->data
                    );
                }
            }
            else if ( valueptr->type == CVT_CONST )
            {
                evalres = evaluate(
                    runtime->current.param ,
                    valueptr->data
                );
            }
            else if ( valueptr->type == CVT_EVAL )
            {
                evalres = run_evaluate(
                    runtime->current.param ,
                    valueptr->data
                );
            }
            
            if ( evalres == NULL )
            {
                evalrescode = 0;
            }
            else
            {
                evalrescode = 0;
                evalrescode = (*evalres == '1');
                memfree( evalres );
            }
            
            i++;
            valueptr = &(val_array[i]);
            
            if ( valueptr->type != CVT_CODE )
            {
                *errorno = __LINE__;
                return RET_ERROR;
            }
        }
        else
        {
            evalrescode = 1;
        }
        
        if ( evalrescode )
        {
            subret = run_code( runtime , valueptr->data , errorno );
            
            if ( subret != RET_OK )
            {
                return subret;
            }
            
            break;
        }
        else
        {
            i++;
        }
    }
    
    return RET_OK;
}

SLINT run_while( RUNTIME* runtime , CODE* code , SLINT* errorno )
{
    bchar*      evalres = NULL;
    SLINT       evalrescode = 0;
    CODE_VALUE* val_array;
    CODE_VALUE* valueptr;
    CODE_VALUE* subcodeptr;
    SLINT       subret;
    
    PARAM_ITEM* paramarray;
    PARAM_ITEM* evalitem;
    
    val_array = code->value;
    paramarray = runtime->current.param->list;
    
    subcodeptr = &(val_array[1]);
    
    if ( subcodeptr->type != CVT_CODE )
    {
        *errorno = __LINE__;
        return RET_ERROR;
    }
    
    valueptr = &(val_array[0]);
    
    while ( TRUE )
    {
        evalres = NULL;
        
        if ( valueptr->type == CVT_VARIABLE )
        {
            evalitem = &(paramarray[valueptr->index]);
            
            if ( ( evalitem->value != NULL )
                && ( evalitem->value->type == RVT_STRING )
                && ( evalitem->value->data != NULL ) )
            {
                evalres = evaluate(
                    runtime->current.param ,
                    evalitem->value->data
                );
            }
            else
            {
                evalres = evaluate(
                    runtime->current.param ,
                    "0"
                );
            }
        }
        else if ( valueptr->type == CVT_CONST )
        {
            evalres = evaluate(
                runtime->current.param ,
                valueptr->data
            );
        }
        else if ( valueptr->type == CVT_EVAL )
        {
            evalres = run_evaluate(
                runtime->current.param ,
                valueptr->data
            );
        }
        
        if ( evalres == NULL )
        {
            *errorno = __LINE__;
            return RET_ERROR;
        }
        
        evalrescode = (*evalres == '1');
        memfree( evalres );
        
        if ( evalrescode )
        {
            subret = run_code( runtime , subcodeptr->data , errorno );
            
            if ( subret != RET_OK )
            {
                return subret;
            }
        }
        else
        {
            break;
        }
    }
    
    return RET_OK;
}

SLINT run_return( RUNTIME* runtime , CODE* code , SLINT* errorno )
{
    SLINT       i;
    CODE_VALUE* val_array;
    CODE_VALUE* valueptr;
    
    PARAM_ITEM* paramarray;
    PARAM_ITEM* retitemptr;
    
    val_array = code->value;
    paramarray = runtime->current.param->list;
    
    if ( code->val_array_count > 0 )
    {
        PARAM_ITEM* retvalue = runtime->current.retvalue;
        SLINT retvalue_count = runtime->current.retvalue_count;
        
        for ( i = 0; i < code->val_array_count; i++ )
        {
            valueptr = &(val_array[i]);
            
            if ( i < retvalue_count )
            {
                retitemptr = &(retvalue[i]);
                unref_value( runtime , retitemptr->value );
                
                if ( valueptr->type == CVT_CONST )
                {
                    retitemptr->value = ref_value_str(
                        runtime ,
                        valueptr->data ,
                        strlen( valueptr->data ) ,
                        1
                    );
                }
                else if ( valueptr->type == CVT_VARIABLE )
                {
                    retitemptr->value = ref_value(
                        runtime ,
                        paramarray[valueptr->index].value
                    );
                }
            }
        }
    }
    
    return RET_RETURN;
}

SLINT run_call( RUNTIME* runtime , CODE* code , SLINT* errorno )
{
    SLINT       i;
    bchar*      node_name;
    CODE_VALUE* val_array;
    CODE_VALUE* valueptr;
    
    PARAM_ITEM* paramarray;
    PARAM_ITEM* itemptr;
    PARAM_ITEM* subparamarray;
    PARAM_ITEM* subretarray;
    SLINT       subret;
    
    val_array = code->value;
    paramarray = runtime->current.param->list;
    
    node_name = NULL;
    
    if ( val_array[0].type == CVT_CONST )
    {
        node_name = val_array[0].data;
    }
    else if ( val_array[0].type == CVT_VARIABLE )
    {
        itemptr = &(paramarray[val_array[0].index]);
        
        if ( ( itemptr->value != NULL )
            && ( itemptr->value->type == RVT_STRING )
            && ( itemptr->value->data != NULL ) )
        {
            node_name = itemptr->value->data;
        }
    }
    
    if ( node_name == NULL )
    {
        *errorno = __LINE__;
        return RET_ERROR;
    }
    
    SLINT call_flag = val_array[1].index & 0xFF000000;
    val_array[1].index &= 0x00FFFFFF;
    SLINT call_param_count = val_array[1].index / 1000;
    SLINT call_return_count = val_array[1].index % 1000;
    
    if ( call_param_count > 0 )
    {
        subparamarray = memalloc_zero(
            sizeof( PARAM_ITEM ) * call_param_count
        );
    }
    else
    {
        subparamarray = NULL;
    }
    
    if ( call_return_count > 0 )
    {
        subretarray = memalloc_zero(
            sizeof( PARAM_ITEM ) * call_return_count
        );
    }
    else
    {
        subretarray = NULL;
    }
    
    for ( i = 0; i < call_param_count; i++ )
    {
        valueptr = &(val_array[2 + i]);
        unref_value( runtime , subparamarray[i].value );
        
        if ( valueptr->type == CVT_CONST )
        {
            subparamarray[i].value = ref_value_str(
                runtime ,
                valueptr->data ,
                strlen( valueptr->data ) ,
                1
            );
        }
        else if ( valueptr->type == CVT_VARIABLE )
        {
            subparamarray[i].value = ref_value(
                runtime ,
                runtime->current.param->list[valueptr->index].value
            );
        }
    }
    
    if ( call_flag & NODE_ASYNC )
    {
        /*TODO:  asynchronous call*/
    }
    
    subret = call_node(
        runtime ,
        node_name ,
        subparamarray ,
        call_param_count ,
        subretarray ,
        call_return_count ,
        errorno
    );
    
    if ( subparamarray != NULL )
    {
        free_param_list_value( runtime , subparamarray , call_param_count );
        free_param_list( subparamarray , call_param_count );
    }
    
    if ( subret != RET_OK )
    {
        if ( subretarray )
        {
            free_param_list_value( runtime , subretarray , call_return_count );
            free_param_list( subretarray , call_return_count );
        }
        
        return subret;
    }
    
    for ( i = 0; i < call_return_count; i++ )
    {
        CODE_VALUE* pretvalue = &(val_array[2 + call_param_count + i]);
        
        if ( pretvalue->type == CVT_VARIABLE )
        {
            unref_value( runtime , paramarray[pretvalue->index].value );
            paramarray[pretvalue->index].value = ref_value(
                runtime ,
                subretarray[i].value
            );
        }
        
        unref_value( runtime , subretarray[i].value );
    }
    
    if ( subretarray )
    {
        memfree( subretarray );
    }
    
    return RET_OK;
}

SLINT run_getitemcount( RUNTIME* runtime , CODE* code , SLINT* errorno )
{
    CODE_VALUE* val_array;
    CODE_VALUE* valueptr;
    
    PARAM_ITEM* paramarray;
    PARAM_ITEM* dest;
    PARAM_ITEM* src;
    SLINT       subret;
    
    val_array = code->value;
    paramarray = runtime->current.param->list;
    
    valueptr = &(val_array[0]);
    
    if ( valueptr->type == CVT_VARIABLE )
    {
        dest = &(paramarray[valueptr->index]);
        
        valueptr = &(val_array[1]);
        
        if ( valueptr->type == CVT_VARIABLE )
        {
            src = &(paramarray[valueptr->index]);
            
            if ( src->value != NULL )
            {
                if ( src->value->type == RVT_TABLE )
                {
                    subret = dmap_getcount( src->value->data );
                    unref_value( runtime , dest->value );
                    dest->value = ref_value_int( runtime , subret );
                }
                else if ( src->value->type == RVT_STRING )
                {
                    unref_value( runtime , dest->value );
                    dest->value = ref_value_int( runtime , src->value->size );
                }
                else
                {
                    unref_value( runtime , dest->value );
                    dest->value = ref_value_int( runtime , 0 );
                }
            }
            else
            {
                unref_value( runtime , dest->value );
                dest->value = ref_value_int( runtime , 0 );
            }
        }
        else if ( valueptr->type == CVT_CONST )
        {
            subret = strlen( val_array[1].data );
            unref_value( runtime , dest->value );
            dest->value = ref_value_int( runtime , subret );
        }
        else
        {
            *errorno = __LINE__;
            return RET_ERROR;
        }
        
        if ( dest->value == NULL )
        {
            dest->value = ref_value_int( runtime , 0 );
        }
    }
    else
    {
        *errorno = __LINE__;
        return RET_ERROR;
    }
    
    return RET_OK;
}

SLINT run_getitem( RUNTIME* runtime , CODE* code , SLINT* errorno )
{
    bchar*      evalstr;
    CODE_VALUE* val_array;
    CODE_VALUE* valueptr;
    
    PARAM_ITEM* paramarray;
    PARAM_ITEM* dest;
    PARAM_ITEM* src;
    
    val_array = code->value;
    paramarray = runtime->current.param->list;
    
    evalstr = get_cvalue_string( runtime->current.param , &val_array[2] );
    valueptr = &(val_array[0]);
    
    if ( ( evalstr != NULL ) && ( valueptr->type == CVT_VARIABLE ) )
    {
        dest = &(paramarray[valueptr->index]);
        
        valueptr = &(val_array[1]);
        
        if ( valueptr->type == CVT_VARIABLE )
        {
            src = &(paramarray[valueptr->index]);
            
            if ( src->value != NULL )
            {
                if ( src->value->type == RVT_TABLE )
                {
                    RT_VALUE* subitem = dmap_query(
                        src->value->data ,
                        evalstr
                    );
                    
                    if ( subitem != NULL )
                    {
                        if ( dest->value != subitem )
                        {
                            unref_value( runtime , dest->value );
                            dest->value = ref_value( runtime , subitem );
                        }
                    }
                    else
                    {
                        unref_value( runtime , dest->value );
                        dest->value = NULL;
                    }
                }
                else if ( src->value->type == RVT_STRING )
                {
                    unref_value( runtime , dest->value );
                    dest->value = get_sub_string(
                        runtime ,
                        src->value->data ,
                        src->value->size ,
                        evalstr
                    );
                }
                else
                {
                    unref_value( runtime , dest->value );
                    dest->value = NULL;
                }
            }
            else
            {
                unref_value( runtime , dest->value );
                dest->value = NULL;
            }
        }
        else if ( valueptr->type == CVT_CONST )
        {
            unref_value( runtime , dest->value );
            dest->value = get_sub_string(
                runtime ,
                valueptr->data ,
                strlen( valueptr->data ) ,
                evalstr
            );
        }
        else
        {
            *errorno = __LINE__;
            return RET_ERROR;
        }
    }
    else
    {
        *errorno = __LINE__;
        return RET_ERROR;
    }
    
    return RET_OK;
}

SLINT run_setitem( RUNTIME* runtime, CODE* code , SLINT* errorno )
{
    SLINT       i;
    bchar*      evalstr;
    CODE_VALUE* val_array;
    CODE_VALUE* valueptr;
    
    PARAM_ITEM* paramarray;
    PARAM_ITEM* dest;
    PARAM_ITEM* src;
    
    val_array = code->value;
    paramarray = runtime->current.param->list;
    
    evalstr = get_cvalue_string( runtime->current.param , &val_array[1] );
    valueptr = &(val_array[0]);
    
    if ( ( evalstr != NULL ) && ( valueptr->type == CVT_VARIABLE ) )
    {
        dest = &(paramarray[valueptr->index]);
        
        if ( dest->value == NULL )
        {
            dest->value = new_table_value( runtime );
        }
        
        if ( dest->value->type == RVT_STRING )
        {
            valueptr = &(val_array[2]);
            bchar* strparam = NULL;
            SLINT strparamsize = 0;
            
            if ( valueptr->type == CVT_VARIABLE )
            {
                src = &(paramarray[valueptr->index]);
                
                if ( src->value != NULL )
                {
                    if ( ( src->value->type == RVT_STRING )
                        && ( src->value->data != NULL ) )
                    {
                        strparam = src->value->data;
                        strparamsize = src->value->size;
                    }
                    else
                    {
                        strparam = "";
                    }
                }
            }
            else if ( valueptr->type == CVT_CONST )
            {
                strparam = valueptr->data;
                
                if ( strparam == NULL )
                {
                    strparam = "";
                }
                else
                {
                    strparamsize = strlen( strparam );
                }
            }
            else if ( valueptr->type == CVT_INIT )
            {
                strparam = "";
            }
            
            if ( strparam == NULL )
            {
                *errorno = __LINE__;
                log_error( "error" );
                return RET_ERROR;
            }
            
            i = atoi( evalstr ) - 1;
            
            if ( ( strparam != NULL ) && ( i >= 0 ) )
            {
                SLINT len = dest->value->size;
                
                if ( i < len )
                {
                    bchar* newstr = memalloc_zero( len + strparamsize );
                    memcpy( newstr , dest->value->data , i );
                    memcpy( newstr + i , strparam , strparamsize );
                    memcpy(
                        newstr + i + strparamsize ,
                        dest->value->data + i + 1 ,
                        len - i - 1
                    );
                    
                    unref_value( runtime , dest->value );
                    dest->value = ref_value_str(
                        runtime ,
                        newstr ,
                        len + strparamsize - 1 ,
                        0
                    );
                }
                else
                {
                    bchar* newstr = memalloc_zero( len + strparamsize + 1 );
                    memcpy( newstr , dest->value->data , len );
                    memcpy( newstr + len , strparam , strparamsize );
                    
                    unref_value( runtime , dest->value );
                    dest->value = ref_value_str(
                        runtime ,
                        newstr ,
                        len + strparamsize ,
                        0
                    );
                }
            }
        }
        else
        {
            if ( dest->value->type != RVT_TABLE )
            {
                unref_value( runtime , dest->value );
                dest->value = new_table_value( runtime );
            }
            
            valueptr = &(val_array[2]);
            RT_VALUE* pnewitem = NULL;
            
            if ( valueptr->type == CVT_VARIABLE )
            {
                src = &(paramarray[valueptr->index]);
                pnewitem = ref_value( runtime , src->value );
            }
            else if ( valueptr->type == CVT_CONST )
            {
                pnewitem = ref_value_str(
                    runtime ,
                    valueptr->data ,
                    strlen( valueptr->data ) ,
                    1
                );
            }
            else if ( valueptr->type == CVT_INIT )
            {
                pnewitem = NULL;
            }
            else
            {
                *errorno = __LINE__;
                log_error( "error" );
                return RET_ERROR;
            }
            
            RT_VALUE* polditem = dmap_query( dest->value->data , evalstr );
            
            if ( polditem != NULL )
            {
                if ( polditem != pnewitem )
                {
                    unref_value( runtime , polditem );
                    
                    if ( pnewitem != NULL )
                    {
                        dmap_insert( dest->value->data , evalstr , pnewitem );
                        update_value( runtime , dest->value );
                    }
                    else
                    {
                        dmap_erase( dest->value->data , evalstr );
                        update_value( runtime , dest->value );
                    }
                }
                else
                {
                    unref_value( runtime , pnewitem );
                }
            }
            else if ( pnewitem != NULL )
            {
                dmap_insert( dest->value->data , evalstr , pnewitem );
                update_value( runtime , dest->value );
            }
        }
    }
    else
    {
        return RET_OK;
    }
    
    return RET_OK;
}

SLINT run_code( RUNTIME* runtime , CODE* code , SLINT* errorno )
{
    SLINT subret = RET_OK;
    
    while ( code )
    {
        switch ( code->op )
        {
        case OP_EQU:
            subret = run_equ( runtime , code , errorno );
            break;
        case OP_EVALUATE:
            subret = run_eval( runtime , code , errorno );
            break;
        case OP_STRFORMAT:
            subret = run_format( runtime , code , errorno );
            break;
        case OP_IF:
            subret = run_if( runtime , code , errorno );
            break;
        case OP_WHILE:
            subret = run_while( runtime , code , errorno );
            break;
        case OP_RETURN:
            subret = run_return( runtime , code , errorno );
            break;
        case OP_CALL:
            subret = run_call( runtime , code , errorno );
            break;
        case OP_GETITEMCOUNT:
            subret = run_getitemcount( runtime , code , errorno );
            break;
        case OP_GETITEM:
            subret = run_getitem( runtime , code , errorno );
            break;
        case OP_SETITEM:
            subret = run_setitem( runtime , code , errorno );
            break;
        case OP_FOREACH:
            subret = run_foreach( runtime , code , errorno );
            break;
        case OP_NOOP:
            break;
        default:
            break;
        }
        
        if ( subret != RET_OK )
        {
            return subret;
        }
        
        code = code->next;
    }
    
    return RET_OK;
}
