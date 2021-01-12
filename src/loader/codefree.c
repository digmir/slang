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
#include "map.h"
#include "mem.h"
#include "memalloc.h"

void free_code( CODE* code );

void free_value( CODE_VALUE* value )
{
    CODE_VALUE* temp;
    
    while ( value )
    {
        temp = value;
        value = value->next;
        
        if ( temp->data != NULL )
        {
            if ( temp->type == CVT_CONST )
            {
                memfree( temp->data );
            }
            else if ( temp->type == CVT_CODE )
            {
                free_code( temp->data );
            }
            else if ( temp->type == CVT_EVAL )
            {
                memfree( temp->data );
            }
        }
        
        memfree( temp );
    }
}

void free_value_array( CODE_VALUE* value , int count )
{
    int i;
    
    for ( i = 0; i < count; i++ )
    {
        if ( value[i].data != NULL )
        {
            if ( value[i].type == CVT_CONST )
            {
                memfree( value[i].data );
            }
            else if ( value[i].type == CVT_CODE )
            {
                free_code( value[i].data );
            }
            else if ( value[i].type == CVT_EVAL )
            {
                memfree( value[i].data );
            }
        }
    }
    
    memfree( value );
}

void free_param_list( PARAM_ITEM* list , int count )
{
    memfree( list );
}

void free_param_list_value( RUNTIME* runtime , PARAM_ITEM* list , int count )
{
    int         i;
    RT_VALUE*   value;
    
    for ( i = 0; i < count; i++ )
    {
        value = list[i].value;
        
        if ( value != NULL )
        {
            unref_value( runtime , value );
            list[i].value = NULL;
        }
    }
}

void free_param( NODE_PARAM* param )
{
    if ( param->list != NULL )
    {
        free_param_list( param->list , param->count );
    }
    
    if ( param->hmap != NULL )
    {
        dmap_release( param->hmap , NULL , NULL );
    }
    
    memfree( param );
}

void free_code( CODE* code )
{
    CODE*   temp;
    
    while ( code )
    {
        temp = code;
        code = code->next;
        
        if ( temp->value != NULL )
        {
            if ( temp->val_array_count >= 0 )
            {
                free_value_array( temp->value , temp->val_array_count );
            }
            else
            {
                free_value( temp->value );
            }
        }
        
        memfree( temp );
    }
}

void free_body( NODE_BODY* body )
{
    if ( body->code != NULL )
    {
        free_code( body->code );
    }
    
    memfree( body );
}

void free_node( NODE* node )
{
    NODE*   temp;
    
    while ( node )
    {
        temp = node;
        node = node->next;
        
        if ( temp->param != NULL )
        {
            free_param( temp->param );
        }
        
        if ( temp->type == NT_SCNODE )
        {
            if ( temp->body != NULL )
            {
                free_body( temp->body );
            }
        }
        
        memfree( temp );
    }
}

void free_node_data( RUNTIME* runtime , NODE* node )
{
    NODE*   temp;
    
    while ( node )
    {
        temp = node;
        node = node->next;
        
        if ( temp->param != NULL )
        {
            if ( temp->param->list != NULL )
            {
                free_param_list_value(
                    runtime             ,
                    temp->param->list   ,
                    temp->param->count
                );
            }
        }
    }
}
