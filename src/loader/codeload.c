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

#include <string.h>

#include "logger.h"
#include "slang.h"
#include "memalloc.h"

void free_value( CODE_VALUE* value );

void free_code( CODE* code );

CODE* load_code( FILE_DESC* pf , int* haverror );

CODE_VALUE* load_value( FILE_DESC* pf , int* pcount , int* haverror )
{
    int         count;
    int         temp;
    int         i;
    CODE_VALUE* ret;
    CODE_VALUE* value;
    
    ret = NULL;
    value = NULL;
    *haverror = 0;
    *pcount = 0;
    
    if ( fileread( pf , &count , INT_SIZE ) != INT_SIZE )
    {
        *haverror = 1;
        return NULL;
    }
    
    *pcount = count;
    
    if ( count == 0 )
    {
        return NULL;
    }
    
    ret = memalloc_zero( sizeof( CODE_VALUE ) * count );
    
    for ( i = 0; i < count; i++ )
    {
        value = &( ret[i] );
        
        if ( fileread( pf , &value->type , INT_SIZE ) != INT_SIZE )
        {
            *haverror = 1;
            break;
        }
        
        if ( value->type == CVT_CONST )
        {
            if ( fileread( pf , &temp , INT_SIZE ) != INT_SIZE )
            {
                *haverror = 1;
                break;
            }
            
            if ( temp > 0 )
            {
                value->data = memalloc_zero( temp + 1 );
                
                if ( fileread( pf , value->data , temp ) != temp )
                {
                    *haverror = 1;
                    break;
                }
            }
        }
        else if ( value->type == CVT_VARIABLE )
        {
            if ( fileread( pf , &value->index , INT_SIZE ) != INT_SIZE )
            {
                *haverror = 1;
                break;
            }
        }
        else if ( value->type == CVT_CODE )
        {
            value->data = load_code( pf , haverror );
            
            if ( *haverror )
            {
                break;
            }
        }
        else if ( value->type == CVT_EVAL )
        {
            if ( fileread( pf , &temp , INT_SIZE ) != INT_SIZE )
            {
                *haverror = 1;
                break;
            }
            
            if ( temp > 0 )
            {
                value->data = memalloc_zero( temp + 1 );
                
                if ( fileread( pf , value->data , temp ) != temp )
                {
                    *haverror = 1;
                    break;
                }
            }
        }
    }
    
    if ( *haverror )
    {
        free_value( ret );
        ret = NULL;
    }
    
    return ret;
}

CODE* load_code( FILE_DESC* pf , int* haverror )
{
    int     count;
    int     i;
    CODE*   ret;
    CODE*   code;
    int     valcount;
    
    count       = 0;
    ret         = NULL;
    code        = NULL;
    *haverror   = 0;
    
    if ( fileread( pf , &count , INT_SIZE ) != INT_SIZE )
    {
        *haverror = 1;
        return NULL;
    }
    
    if ( count == 0 )
    {
        return NULL;
    }
    
    for ( i = 0; i < count; i++ )
    {
        if ( ret == NULL )
        {
            code = memalloc_zero( sizeof( CODE ) );
            ret = code;
        }
        else
        {
            code->next = memalloc_zero( sizeof( CODE ) );
            code = code->next;
        }
        
        if ( fileread( pf , &code->op , INT_SIZE ) != INT_SIZE )
        {
            *haverror = 1;
            break;
        }
        
        code->value = load_value( pf , &code->val_array_count , haverror );
        
        if ( *haverror && code->value == NULL )
        {
            break;
        }
    }
    
    if ( *haverror )
    {
        free_code( ret );
        ret = NULL;
    }
    
    return ret;
}

int load_body( NODE_BODY* body , FILE_DESC* pf )
{
    int     haverror;
    
    body->code = load_code( pf , &haverror );
    
    if ( haverror )
    {
        return 0;
    }
    
    return 1;
}

int load_param( NODE_PARAM* param , FILE_DESC* pf )
{
    int i;
    
    if ( fileread( pf , &param->count , INT_SIZE ) != INT_SIZE )
    {
        return 0;
    }
    
    if ( fileread( pf , &param->count1 , INT_SIZE ) != INT_SIZE )
    {
        return 0;
    }
    
    if ( fileread( pf , &param->count2 , INT_SIZE ) != INT_SIZE )
    {
        return 0;
    }
    
    if ( param->count == 0 )
    {
        return 1;
    }
    
    param->list = memalloc_zero( sizeof( PARAM_ITEM ) * param->count );
    
    for ( i = 0; i < param->count; i++ )
    {
        param->list[i].value = NULL;
        
        if ( fileread(
            pf                      ,
            &param->list[i].name    ,
            MAX_NAME_LEN
        ) != MAX_NAME_LEN )
        {
            return 0;
        }
    }
    
    return 1;
}

NODE* load_node( char* bin_file )
{
    FILE_DESC*  pf;
    NODE*       ret;
    NODE*       node;
    NODE*       temp;
    int         count;
    int         i;
    int         readok;
    char        fname[MAX_WORD_NAME_LEN];
    
    count   = 0;
    ret     = NULL;
    node    = NULL;
    readok  = 1;
    
    pf = fileopen( bin_file , "rb" );
    
    if ( pf == NULL )
    {
        return NULL;
    }
    
    if ( fileread( pf , &count , INT_SIZE ) != INT_SIZE )
    {
        fileclose( pf );
        return NULL;
    }
    
    if ( count == 0 )
    {
        fileclose( pf );
        return NULL;
    }
    
    for ( i = 0; i < count; i++ )
    {
        if ( ret == NULL )
        {
            node = memalloc_zero( sizeof( NODE ) );
            ret = node;
        }
        else
        {
            node->next = memalloc_zero( sizeof( NODE ) );
            node = node->next;
        }
        
        if ( fileread( pf , &node->type , INT_SIZE ) != INT_SIZE )
        {
            log_info( "error 1" );
            readok = 0;
            break;
        }
        
        if ( fileread(
            pf                  ,
            node->name          ,
            MAX_WORD_NAME_LEN
        ) != MAX_WORD_NAME_LEN )
        {
            log_info( "error 1" );
            readok = 0;
            break;
        }
        
        /*read param*/
        node->param = memalloc_zero( sizeof( NODE_PARAM ) );
        
        if ( !load_param( node->param , pf ) )
        {
            log_info( "error 3" );
            readok = 0;
            break;
        }
        
        if ( node->type == NT_SCNODE )
        {
            /*read body*/
            node->body = memalloc_zero( sizeof( NODE_BODY ) );
            
            if ( ! load_body( node->body , pf ) )
            {
                log_info( "error 5" );
                readok = 0;
                break;
            }
        }
        else
        {
            if ( fileread(
                pf                  ,
                fname               ,
                MAX_WORD_NAME_LEN
            ) != MAX_WORD_NAME_LEN )
            {
                log_info( "error 1" );
                readok = 0;
                break;
            }
            
            temp = ret;
            
            while ( temp )
            {
                if ( temp->type == NT_SCNODE )
                {
                    if ( strcmp( temp->name , fname ) == 0 )
                    {
                        break;
                    }
                }
                
                temp = temp->next;
            }
            
            if ( !temp )
            {
                log_info( "error 1" );
                readok = 0;
                break;
            }
            
            node->body = temp->body;
        }
    }
    
    fileclose( pf );
    
    if ( readok )
    {
        return ret;
    }
    
    free_node( ret );
    
    return NULL;
}
