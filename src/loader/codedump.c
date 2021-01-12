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
#include "file.h"
//#include "run.h"

int get_value_count( CODE_VALUE* value )
{
    int count;
    
    count = 0;
    
    while ( value )
    {
        count++;
        value = value->next;
    }
    
    return count;
}

int dump_code( CODE* code , FILE_DESC* pf );

void dump_value( CODE_VALUE* value , FILE_DESC* pf )
{
    int temp;
    
    while ( value )
    {
        filewrite( pf , &value->type , INT_SIZE );
        
        if ( value->type == CVT_CONST )
        {
            if ( value->data == NULL )
            {
                temp = 0;
            }
            else
            {
                temp = strlen( ( char* )value->data );
            }
            
            filewrite( pf , &temp , INT_SIZE );
            
            if ( temp > 0 )
            {
                filewrite( pf , value->data , temp );
            }
        }
        else if ( value->type == CVT_VARIABLE )
        {
            filewrite( pf , &value->index , INT_SIZE );
        }
        else if ( value->type == CVT_CODE )
        {
            if ( value->data != NULL )
            {
                dump_code( value->data , pf );
            }
            else
            {
                temp = 0;
                filewrite( pf , &temp , INT_SIZE );
            }
        }
        else if ( value->type == CVT_EVAL )
        {
            if ( value->data == NULL )
            {
                temp = 0;
            }
            else
            {
                temp = strlen( ( char* )value->data );
            }
            
            filewrite( pf , &temp , INT_SIZE );
            
            if ( temp > 0 )
            {
                filewrite( pf , value->data , temp );
            }
        }
        
        value = value->next;
    }
}

void dump_value_array( CODE_VALUE* value , int count , FILE_DESC* pf )
{
    int i;
    int temp;
    
    for ( i = 0; i < count; i++ )
    {
        filewrite( pf , &value->type , INT_SIZE );
        
        if ( value->type == CVT_CONST )
        {
            if ( value->data == NULL )
            {
                temp = 0;
            }
            else
            {
                temp = strlen( ( char* )value->data );
            }
            
            filewrite( pf , &temp , INT_SIZE );
            
            if ( temp > 0 )
            {
                filewrite( pf , value->data , temp );
            }
        }
        else if ( value->type == CVT_VARIABLE )
        {
            filewrite( pf , &value->index , INT_SIZE );
        }
        else if ( value->type == CVT_CODE )
        {
            if ( value->data != NULL )
            {
                dump_code( value->data , pf );
            }
            else
            {
                temp = 0;
                filewrite( pf , &temp , INT_SIZE );
            }
        }
        else if ( value->type == CVT_EVAL )
        {
            if ( value->data == NULL )
            {
                temp = 0;
            }
            else
            {
                temp = strlen( ( char* )value->data );
            }
            
            filewrite( pf , &temp , INT_SIZE );
            
            if ( temp > 0 )
            {
                filewrite( pf , value->data , temp );
            }
        }
        
        value++;
    }
}

void dump_param( NODE_PARAM* param , FILE_DESC* pf )
{
    int i;
    
    filewrite( pf , &param->count , INT_SIZE );
    filewrite( pf , &param->count1 , INT_SIZE );
    filewrite( pf , &param->count2 , INT_SIZE );
    
    if ( param->count > 0 )
    {
        for ( i = 0; i < param->count; i++ )
        {
            filewrite( pf , &param->list[i].name , MAX_NAME_LEN );
        }
    }
}

int dump_code( CODE* code , FILE_DESC* pf )
{
    CODE*   temp;
    int     count;
    int     valuecount;
    
    count       = 0;
    valuecount  = 0;
    temp        = code;
    
    while ( temp )
    {
        count++;
        temp = temp->next;
    }
    
    filewrite( pf , &count , INT_SIZE );
    
    if ( count == 0 )
    {
        return count;
    }
    
    while ( code )
    {
        filewrite( pf , &code->op , INT_SIZE );
        
        if ( code->val_array_count >= 0 )
        {
            filewrite( pf , &code->val_array_count , INT_SIZE );
            
            if ( code->val_array_count > 0 )
            {
                dump_value_array( code->value , code->val_array_count , pf );
            }
        }
        else
        {
            valuecount = 0;
            
            if ( code->value != NULL )
            {
                valuecount = get_value_count( code->value );
            }
            
            filewrite( pf , &valuecount , INT_SIZE );
            
            if ( valuecount > 0 )
            {
                dump_value( code->value , pf );
            }
        }
        
        code = code->next;
    }
    
    return count;
}

void dump_body( NODE_BODY* body , FILE_DESC* pf )
{
    int empty;
    
    if ( body->code != NULL )
    {
        empty = dump_code( body->code , pf );
    }
    else
    {
        empty = 0;
        filewrite( pf , &empty , INT_SIZE );
    }
}

int dump_node( NODE* node , char* dest_file )
{
    FILE_DESC*  pf;
    int     count;
    int     empty;
    NODE*   nodehead;
    NODE*   temp;
    
    count = 0;
    
    pf = fileopen( dest_file , "wb" );
    
    if ( pf == NULL )
    {
        return 0;
    }
    
    nodehead = node;
    temp = nodehead;
    
    while ( temp )
    {
        if ( ( temp->type == NT_SCNODE ) || ( temp->type == NT_FISSIONNODE ) )
        {
            count++;
        }
        
        temp = temp->next;
    }
    
    filewrite( pf , &count , INT_SIZE );
    
    empty = 0;
    
    while ( node )
    {
        if ( ( node->type == NT_SCNODE ) || ( node->type == NT_FISSIONNODE ) )
        {
            filewrite( pf , &node->type , INT_SIZE );
            filewrite( pf , node->name , MAX_WORD_NAME_LEN );
            
            if ( node->param != NULL )
            {
                dump_param( node->param , pf );
            }
            else
            {
                filewrite( pf , &empty , INT_SIZE );
            }
            
            if ( node->type == NT_SCNODE )
            {
                if ( node->body != NULL )
                {
                    dump_body( node->body , pf );
                }
                else
                {
                    filewrite( pf , &empty , INT_SIZE );
                }
            }
            else
            {
                temp = nodehead;
                
                while ( temp )
                {
                    if ( temp->type == NT_SCNODE )
                    {
                        if ( temp->body == node->body )
                        {
                            break;
                        }
                    }
                    
                    temp = temp->next;
                }
                
                if ( !temp )
                {
                    fileclose( pf );
                    return 0;
                }
                
                filewrite( pf , temp->name , MAX_WORD_NAME_LEN );
            }
        }
        
        node = node->next;
    }
    
    fileclose( pf );
    return 1;
}
