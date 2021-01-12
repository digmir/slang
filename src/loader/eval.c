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
#include "errorstr.h"
#include "eval.h"
#include "stack.h"
#include "mem.h"
#include "slanglex.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define EVAL_GE     ( 0x10000 | '>' )
#define EVAL_LE     ( 0x10000 | '<' )
#define EVAL_EQU    ( 0x10000 | '=' )
#define EVAL_NEQ    ( 0x10000 | '!' )

bchar* evaluate( NODE_PARAM* param , bchar* str )
{
    SLINT       pos = 0;
    EVAL_VALUE* evalret;
    
    evalret = build_evaluate( param , str , strlen( str ) , &pos );
    
    if ( evalret == NULL )
    {
        log_error( ERROR_EVAL );
        return ( bchar* )dup_str( "0" );
    }
    
    bchar* ret = run_evaluate( param , evalret );
    free_evaluate( evalret );
    
    return ret;
}

SLINT e_shift_nameword( bchar* buffer , SLINT buffer_size , SLINT* cur_pos )
{
    SLINT   c;
    SLINT   backpos;
    
    c = shift_word( buffer , buffer_size , cur_pos );
    
    if ( c == 0 )
    {
        return -1;
    }
    
    if ( ! is_namestart( c ) && ( c != '-' ) )
    {
        return -1;
    }
    
    while ( TRUE )
    {
        backpos = *cur_pos;
        c = shift_word( buffer , buffer_size , cur_pos );
        
        if ( ! is_name( c ) )
        {
            break;
        }
    }
    
    if ( c == 0 )
    {
        return ( *cur_pos );
    }
    else
    {
        ( *cur_pos ) = backpos;
        return ( *cur_pos );
    }
}

SLINT e_shift_number( bchar* buffer , SLINT buffer_size , SLINT* cur_pos )
{
    SLINT c;
    SLINT backpos;
    SLINT getdot;
    
    c = shift_word( buffer , buffer_size , cur_pos );
    
    if ( c == 0 )
    {
        return -1;
    }
    
    if ( ( c != '-' ) && ( c < '0' || c > '9' ) )
    {
        return -1;
    }
    
    if ( c == '-' )
    {
        c = shift_word( buffer , buffer_size , cur_pos );
    }
    
    if ( c < '0' || c > '9' )
    {
        return -1;
    }
    
    getdot = 0;
    
    while ( TRUE )
    {
        backpos = *cur_pos;
        c = shift_word( buffer , buffer_size , cur_pos );
        
        if ( getdot )
        {
            if ( c < '0' || c > '9' )
            {
                break;
            }
        }
        else
        {
            if ( c == '.' )
            {
                getdot = 1;
            }
            else if ( c < '0' || c > '9' )
            {
                break;
            }
        }
    }
    
    if ( c == 0 )
    {
        return ( *cur_pos );
    }
    else
    {
        ( *cur_pos ) = backpos;
        return ( *cur_pos );
    }
}

bchar* ekey_list = "()<>=!+-*/";

SLINT e_iskey( SLINT c )
{
    SLINT i;
    
    for ( i = 0; i < strlen( ekey_list ); i++ )
    {
        if ( is_space( c ) || c == 0 )
        {
            return 0;
        }
        
        if ( c == ekey_list[i] )
        {
            return 1;
        }
    }
    
    return 0;
}

SLINT e_shift_clause(
    bchar*  buffer      ,
    SLINT   buffer_size ,
    SLINT*  cur_pos     ,
    CLAUSE* clause      ,
    SLINT   stack_status
) {
    SLINT c;
    SLINT start_pos;
    SLINT end_pos;
    SLINT backpos;
    SLINT i;
    
    if ( clause->constvalue != NULL )
    {
        memfree( clause->constvalue );
        clause->constvalue = NULL;
    }
    
    if ( *cur_pos >= buffer_size )
    {
        clause->type = CT_ENDFILE;
        return -2; /*read end*/
    }
    
    backpos = *cur_pos;
    c = shift_word( buffer , buffer_size , cur_pos );
    
    while ( is_space( c ) )
    {
        backpos = *cur_pos;
        c = shift_word( buffer , buffer_size , cur_pos );
    }
    
    if ( c == 0 )
    {
        clause->type = CT_ENDFILE;
        return -2; /*read end*/
    }
    
    start_pos = backpos;
    
    if ( c == '\'' )
    {
        start_pos = *cur_pos;
        
        while ( TRUE )
        {
            backpos = *cur_pos;
            c = shift_word( buffer , buffer_size , cur_pos );
            
            if ( ( c == '\'' ) || ( c == 0 ) )
            {
                break;
            }
        }
        
        end_pos = backpos;
        set_clause_info(
            buffer      ,
            start_pos   ,
            end_pos     ,
            clause      ,
            CT_CONST    ,
            0
        );
        return end_pos;
    }
    
    if ( e_iskey( c ) )
    {
        if ( ( c == '-' ) && ( stack_status == 0 ) )
        {
            
        }
        else
        {
            end_pos = *cur_pos;
            set_clause_info(
                buffer      ,
                start_pos   ,
                end_pos     ,
                clause      ,
                CT_KEY      ,
                c
            );
            return end_pos;
        }
    }
    
    ( *cur_pos ) = backpos;
    
    end_pos = e_shift_number( buffer , buffer_size , cur_pos );
    
    if ( end_pos > 0 )
    {
        set_clause_info( buffer , start_pos , end_pos , clause , CT_CONST , 0 );
    }
    else
    {
        ( *cur_pos ) = backpos;
        end_pos = e_shift_nameword( buffer , buffer_size , cur_pos );
        set_clause_info( buffer , start_pos , end_pos , clause , CT_CONST , 0 );
        clause->type = CT_VARIABLE;
    }
    
    return end_pos;
}

bchar* get_param_value( NODE_PARAM* param , bchar* name )
{
    bchar*      ret;
    SLINT       i;
    PARAM_ITEM* pitem = param->list;
    
    if ( name == NULL )
    {
        return "";
    }
    
    for ( i = 0; i < param->count; i++ )
    {
        pitem = &(param->list[i]);
        
        if ( strcmp( pitem->name , name ) == 0 )
        {
            if ( ( pitem->value != NULL )
                && ( pitem->value->type == RVT_STRING ) )
            {
                if ( pitem->value->data != NULL )
                {
                    return pitem->value->data;
                }
            }
        }
    }
    
    return name;
}

bchar* copy_param_value( NODE_PARAM* param , bchar* name )
{
    bchar*      ret;
    SLINT       i;
    PARAM_ITEM* pitem = param->list;
    
    for ( i = 0; i < param->count; i++ )
    {
        pitem = &(param->list[i]);
        
        if ( strcmp( pitem->name , name ) == 0 )
        {
            if ( ( pitem->value != NULL )
                && ( pitem->value->type == RVT_STRING ) )
            {
                if ( pitem->value->data != NULL )
                {
                    return dup_str( pitem->value->data );
                }
            }
            
            return dup_str( "" );
        }
    }
    
    return dup_str( name );
}

bchar* strformat( NODE_PARAM* param , bchar* value )
{
    bchar*  ret;
    bchar*  tmp;
    bchar*  substr;
    SLINT   tsize = 0;
    SLINT   nsize = 0;
    SLINT   pos = 0;
    SLINT   str_len = strlen( value );
    CLAUSE  clause;
    bchar   tempstr[16];
    
    memset( &clause , 0 , sizeof( CLAUSE ) );
    memset( tempstr , 0 , 16 );
    
    tsize = 128;
    ret = memalloc_zero( tsize );
    
    while ( pos < str_len )
    {
        e_shift_clause( value , str_len , &pos , &clause , 0 );
        
        if ( clause.type == CT_ENDFILE )
        {
            break;
        }
        else if ( clause.type == CT_ERRORWORD )
        {
            log_error( "error" );
            free_clause( &clause );
            strcpy( ret , "error" );
            return ret;
        }
        
        substr = NULL;
        
        if ( ( clause.type == CT_VARIABLE ) && clause.constvalue )
        {
            substr = get_param_value( param , clause.constvalue );
        }
        else if ( ( clause.type == CT_CONST ) && clause.constvalue )
        {
            substr = clause.constvalue;
        }
        else if ( clause.type == CT_KEY )
        {
            snprintf( tempstr , 16 , "%c" , clause.key );
            substr = tempstr;
        }
        
        if ( substr != NULL )
        {
            nsize = strlen( ret ) + strlen( substr );
            
            if ( nsize >= tsize )
            {
                tsize = nsize + 32;
                tmp = memalloc_zero( tsize );
                strcpy( tmp , ret );
                memfree( ret );
                ret = tmp;
            }
            
            strcat( ret , substr );
        }
    }
    
    free_clause( &clause );
    return ret;
}

void free_stack_code( EVAL_CODE* code )
{
    EVAL_CODE* temp;
    
    if ( code == NULL )
    {
        return;
    }
    
    while ( code )
    {
        temp = code;
        code = code->next;
        
        if ( temp->value.type == EVT_SUBEVAL )
        {
            free_stack_code( ( EVAL_CODE* )temp->value.data );
        }
        else
        {
            memfree( temp->value.data );
        }
        
        memfree( temp );
    }
}

void free_stack_data( STACK* stackdata )
{
    EVAL_VALUE* data = pop( stackdata );
    
    while ( data )
    {
        free_evaluate( data );
        data = pop( stackdata );
    }
}

/*
string to bin, "-12.34" => ± 1,2,.,3,4
*/
bchar* strnumtobin(
    bchar*  data            ,
    SLINT   base            ,
    SLINT   containdotsign  ,
    SLINT*  prlen           ,
    SLINT*  pdotlen         ,
    SLINT*  psign
) {
    SLINT   len , dlen , i;
    bchar*  pbuf;
    
    if ( data == NULL || prlen == NULL || psign == NULL )
    {
        return NULL;
    }
    
    if ( base > 16 )
    {
        return NULL;
    }
    
    /* remove spaces */
    while ( *data )
    {
        if ( ( ( *data ) != ' ')
            && ( ( *data ) != '\t' )
            && ( ( *data ) != '\n' )
            && ( ( *data ) != '\r' ) )
        {
            break;
        }
        
        data++;
    }
    
    /* remove prefix 0 */
    while ( *data )
    {
        if ( ( *data ) != '0' )
        {
            break;
        }
        
        data++;
    }
    
    len = 0;
    while ( data[len] )
    {
        if ( data[len] == '.' )
        {
            break;
        }
        
        len++;
    }
    
    if ( len == 0 )
    {
        return NULL;
    }
    
    if ( data[len] == '.' )
    {
        dlen = strlen( data + len + 1 );
    }
    else
    {
        dlen = 0;
    }
    
    if ( data[0] == '-' )
    {
        len--;
        
        if ( len == 0 )
        {
            return NULL;
        }
        
        *psign = 1;
    }
    else
    {
        *psign = 0;
    }
    
    if ( containdotsign )
    {
        pbuf = memalloc_zero( len + dlen + 3 );
        
        if ( *psign )
        {
            memcpy( pbuf , data , len + dlen + 2 );
        }
        else
        {
            memcpy( pbuf + 1 , data , len + dlen + 1 );
            pbuf[0] = ' ';
        }
        
        for ( i = 1; i <= len; i++ )
        {
            pbuf[i] -= '0';
        }
        
        for ( i = len + 2; i <= len + dlen + 1; i++ )
        {
            pbuf[i] -= '0';
        }
    }
    else
    {
        pbuf = memalloc_zero( len + dlen + 1 );
        
        if ( *psign )
        {
            memcpy( pbuf , data + 1 , len );
            memcpy( pbuf + len , data + len + 2 , dlen );
        }
        else
        {
            memcpy( pbuf , data , len );
            memcpy( pbuf + len , data + len + 1 , dlen );
        }
        
        for ( i = 0; i < len + dlen; i++ )
        {
            pbuf[i] -= '0';
        }
    }
    
    *prlen = len;
    *pdotlen = dlen;
    return pbuf;
}

void trimzero( bchar* ret )
{
    SLINT   i;
    SLINT   len;
    
    len = strlen( ret );
    
    i = 1;
    
    while ( ( ret[i] == ' ' ) || ( ret[i] == '0' ) )
    {
        i++;
    }
    
    if ( ret[0] == ' ' )
    {
        memmove( ret , ret + i , len - i + 1 );
    }
    else
    {
        if ( i > 1 )
        {
            memmove( ret + 1 , ret + i , len - i + 1 );
        }
    }
    
    i = len - 1;
    
    while ( ( ret[i] == ' ' ) || ( ret[i] == '0' ) )
    {
        ret[i] = 0;
        i--;
    }
}

bchar* evaluate_add( bchar* data1 , bchar* data2 )
{
    bchar*  ret;
    SLINT   dlen1 , dlen2 , dotlen1 , dotlen2 , rlen , rdotlen , i;
    bchar*  bin1;
    bchar*  bin2;
    SLINT   base;
    SLINT   sign1 , sign2 , rsign;
    bchar*  tmpret;
    bchar*  tmpbin1;
    bchar*  tmpbin2;
    
    sign1 = 0;
    sign2 = 0;
    rsign = 0;
    base = 10;
    
    bin1 = strnumtobin( data1 , base , 1 , &dlen1 , &dotlen1 , &sign1 );
    bin2 = strnumtobin( data2 , base , 1 , &dlen2 , &dotlen2 , &sign2 );
    
    if ( ( bin1 == NULL ) || ( dlen1 == 0 ) )
    {
        return dup_str( data2 );
    }
    
    if ( ( bin2 == NULL ) || ( dlen2 == 0 ) )
    {
        return dup_str( data1 );
    }
    
    /* take the maximum length */
    if ( dlen1 > dlen2 )
    {
        rlen = dlen1;
        rsign = sign1;
        sign2 =sign1 ^ sign2;
        sign1 = 0;
    }
    else if ( dlen1 < dlen2 )
    {
        rlen = dlen2;
        rsign = sign2;
        sign1 =sign1 ^ sign2;
        sign2 = 0;
    }
    else
    {
        rlen = dlen1;
        
        rsign = 1;
        
        for ( i = 1; i <= dlen1; i++ )
        {
            if ( bin1[i] < bin2[i] )
            {
                rsign = 0;
                break;
            }
        }
        
        if ( rsign == 1 )
        {
            for ( i = 0; i <= MIN( dotlen1 , dotlen2 ); i++ )
            {
                if ( bin1[dlen1 + 2 + i] < bin2[dlen1 + 2 + i] )
                {
                    rsign = 0;
                    break;
                }
            }
            
            if ( rsign == 1 )
            {
                if ( dotlen2 > dotlen1 )
                {
                    rsign = 0;
                }
            }
        }
        
        if ( rsign )
        {
            rsign = sign1;
            sign2 =sign1 ^ sign2;
            sign1 = 0;
        }
        else
        {
            rsign = sign2;
            sign1 =sign1 ^ sign2;
            sign2 = 0;
        }
    }
    
    rdotlen = MAX( dotlen1 , dotlen2 );
    
    /* sign, carry, end char, decimal point*/
    ret = memalloc_zero( rlen + rdotlen + 4 );
    
    if ( rdotlen > 0 )
    {
        *(ret + rlen + 2) = '.';
        
        for ( i = rdotlen; i > 0; i-- )
        {
            tmpret = ret + rlen + 2 + i;
            
            if ( dotlen1 >= i )
            {
                tmpbin1 = bin1 + dlen1 + 1 + i;
                
                if ( sign1 )
                {
                    *tmpret -= *tmpbin1;
                }
                else
                {
                    *tmpret += *tmpbin1;
                }
                
                dotlen1--;
            }
            
            if ( dotlen2 >= i )
            {
                tmpbin2 = bin2 + dlen2 + 1 + i;
                
                if ( sign2 )
                {
                    *tmpret -= *tmpbin2;
                }
                else
                {
                    *tmpret += *tmpbin2;
                }
                
                dotlen2--;
            }
            
            if ( *tmpret >= base )
            {
                *tmpret -= base;
                
                if ( i == 1 )
                {
                    *(tmpret - 2) += 1;
                }
                else
                {
                    *(tmpret - 1) += 1;
                }
            }
            else if ( *tmpret < 0 )
            {
                if ( i == 1 )
                {
                    *(tmpret - 2) -= 1;
                }
                else
                {
                    *(tmpret - 1) -= 1;
                }
                
                *tmpret += base;
            }
            
            *tmpret += '0';
        }
    }
    
    for (i = rlen; i > 0; i-- )
    {
        tmpret = ret + 1 + i;
        tmpbin1 = bin1 + dlen1;
        tmpbin2 = bin2 + dlen2;
        
        if ( dlen1 > 0 )
        {
            if ( sign1 )
            {
                *tmpret -= *tmpbin1;
            }
            else
            {
                *tmpret += *tmpbin1;
            }
            
            dlen1--;
        }
        
        if ( dlen2 > 0 )
        {
            if ( sign2 )
            {
                *tmpret -= *tmpbin2;
            }
            else
            {
                *tmpret += *tmpbin2;
            }
            
            dlen2--;
        }
        
        if ( *tmpret >= base )
        {
            (*tmpret) -= base;
            *(tmpret - 1) += 1;
        }
        else if ( *tmpret < 0 )
        {
            *(tmpret - 1) -= 1;
            (*tmpret) += base;
        }
        
        *tmpret += '0';
    }
    
    memfree( bin1 );
    memfree( bin2 );
    
    ret[1] += '0';
    
    if ( rsign )
    {
        ret[0] = '-';
    }
    else
    {
        ret[0] = ' ';
    }
    
    trimzero( ret );
    
    return ret;
}

bchar* evaluate_sub( bchar* data1 , bchar* data2 )
{
    int     len;
    bchar*  temp;
    bchar*  ret;
    
    if ( data2[0] == '-' )
    {
        len = strlen( data2 );
        temp = memalloc_zero( len + 1 );
        strcpy( temp , data2 );
        temp[0] = ' ';
    }
    else
    {
        len = strlen( data2 );
        temp = memalloc_zero( len + 2 );
        strcpy( temp + 1 , data2 );
        temp[0] = '-';
    }
    
    ret = evaluate_add( data1 , temp );
    
    memfree( temp );
    return ret;
}

bchar* evaluate_mul( bchar* data1 , bchar* data2 )
{
    bchar*  ret;
    SLINT   dlen1 , dlen2 , dotlen1 , dotlen2 , i , j;
    bchar*  bin1;
    bchar*  bin2;
    SLINT   base;
    SLINT   sign1 , sign2;
    bchar*  tmpret;
    bchar*  tmprettail;
    bchar*  tmpbin1;
    bchar   tmpchar2;
    
    base = 10;
    
    bin1 = strnumtobin( data1 , base , 0 , &dlen1 , &dotlen1 , &sign1 );
    bin2 = strnumtobin( data2 , base , 0 , &dlen2 , &dotlen2 , &sign2 );
    
    dlen1 += dotlen1;
    dlen2 += dotlen2;
    
    if ( bin1 == NULL || dlen1 == 0 || bin2 == NULL || dlen2 == 0 )
    {
        memfree( bin1 );
        memfree( bin2 );
        return dup_str( "0" );
    }
    
    /* sign, end char,  decimal point */
    ret = memalloc_zero( dlen1 + dlen2 + 3 );
    
    tmprettail = ret + dlen1 + dlen2 + 1;
    
    for ( i = 1; i <= dlen2; i++ )
    {
        tmpret = tmprettail - i;
        tmpbin1 = bin1 + dlen1;
        tmpchar2 = *(bin2 + dlen2 - i);
        
        for ( j = 1; j <= dlen1; j++ )
        {
            *tmpret += ( ( *( tmpbin1 - j ) ) * tmpchar2 );
            
            if ( *tmpret >= base )
            {
                *(tmpret - 1) += (*tmpret) / base;
                *tmpret = (*tmpret) % base;
            }
            
            tmpret--;
        }
    }
    
    memfree( bin1 );
    memfree( bin2 );
    
    for ( i = 1; i <= ( dlen1 + dlen2 ); i++ )
    {
        ret[i] += '0';
    }
    
    dotlen1 += dotlen2;
    
    if ( dotlen1 > 0 )
    {
        memmove(
            tmprettail - dotlen1 + 1 ,
            tmprettail - dotlen1 ,
            dotlen1
        );
        *(tmprettail - dotlen1) = '.';
    }
    
    if ( sign1 ^ sign2 )
    {
        ret[0] = '-';
    }
    else
    {
        ret[0] = ' ';
    }
    
    trimzero( ret );
    
    return ret;
}

bchar* evaluate_div( bchar* data1 , bchar* data2 )
{
    /*TODO: */
    
    bchar*  ret;
    SLINT   r = 0;
    SLINT   d1 = atoi( data1 );
    SLINT   d2 = atoi( data2 );
    
    if ( d2 == 0 )
    {
        log_error( "div 0 error" );
        r = 0;
    }
    else
    {
        r = d1 / d2;
    }
    
    ret = memalloc_zero( 16 );
    snprintf( ret , 16 , "%d" , r );
    return ret;
}

bchar* evaluate_cmp( bchar* data1 , bchar* data2 , SLINT code )
{
    bchar*  ret;
    bchar*  subret;
    SLINT   i;
    SLINT   len;
    SLINT   flag;
    SLINT   r;
    
    ret = memalloc_zero( 4 );
    subret = evaluate_sub( data1 , data2 );
    
    if ( subret == NULL )
    {
        strcpy( ret , "0" );
        return ret;
    }
    
    len = strlen( subret );
    
    flag = 0;
    
    for ( i = 0; i < len; i++ )
    {
        if ( ( subret[i] != '0' )
            && ( subret[i] != '-' )
            && ( subret[i] != '.' ) )
        {
            flag = 1;
            break;
        }
    }
    
    if ( code == '<' )
    {
        r = ( ( flag != 0 ) && ( subret[0] == '-' ) );
    }
    else if ( code == '>' )
    {
        r = ( ( flag != 0 ) && ( subret[0] != '-' ) );
    }
    else if ( code == EVAL_GE )
    {
        /* >= */
        r = ( ( flag == 0 ) || ( subret[0] != '-' ) );
    }
    else if ( code == EVAL_LE )
    {
        /* <= */
        r = ( ( flag == 0 ) || ( subret[0] == '-' ) );
    }
    else if ( code == EVAL_EQU )
    {
        r = ( ( flag == 0 ) && ( strcmp( data1 , data2 ) == 0 ) );
    }
    else if ( code == EVAL_NEQ )
    {
        r = ( ( flag != 0 ) || ( strcmp( data1 , data2 ) != 0 ) );
    }
    
    memfree( subret );
    
    if ( r )
    {
        strcpy( ret , "1" );
    }
    else
    {
        strcpy( ret , "0" );
    }
    
    return ret;
}

EVAL_VALUE* new_eval_value( SLINT type , void* value )
{
    EVAL_VALUE* ret = memalloc_zero( sizeof( EVAL_VALUE ) );
    ret->type = type;
    ret->data = value;
    return ret;
}

SLINT stack_downlevel( STACK* stackcode , STACK* stackdata , SLINT code_level )
{
    EVAL_CODE*  ret = NULL;
    EVAL_CODE*  curcode = NULL;
    EVAL_VALUE* data;
    EVAL_VALUE* preresult;
    bchar*      result;
    SLINT       code;
    SLINT       bContinue;
    
    ret = NULL;
    curcode = NULL;
    preresult = pop( stackdata );
    
    if ( preresult == NULL )
    {
        log_error( "error" );
        return 0;
    }
    
    if ( preresult->type != EVT_NUMBER )
    {
        curcode = memalloc_zero( sizeof( EVAL_CODE ) );
        ret = curcode;
        curcode->op = '=';
        curcode->value.type = preresult->type;
        curcode->value.data = preresult->data;
        memfree( preresult );
        preresult = NULL;
    }
    
    bContinue = 1;
    
    while ( bContinue )
    {
        code = ( SLINT )pop( stackcode );
        
        if ( code == 0 )
        {
            break;
        }
        
        if ( ( code == '+' || code == '-' ) && ( code_level > 1 ) )
        {
            push( stackcode , INT2PTR( code ) );
            break;
        }
        
        if ( ( code == '>' || code == '<' || code == EVAL_GE
                    || code == EVAL_LE || code == EVAL_EQU || code == EVAL_NEQ)
            && (code_level > 0))
        {
            push( stackcode , INT2PTR( code ) );
            break;
        }
        
        data = pop( stackdata );
        
        if ( data == NULL )
        {
            log_error( "error" );
            break;
        }
        
        if ( ret == NULL )
        {
            if ( data->type == EVT_NUMBER )
            {
                switch ( code )
                {
                case '+':
                    result = evaluate_add( data->data , preresult->data );
                    break;
                case '-':
                    result = evaluate_sub( data->data , preresult->data );
                    break;
                case '*':
                    result = evaluate_mul( data->data , preresult->data );
                    break;
                case '/':
                    result = evaluate_div( data->data , preresult->data );
                    break;
                case '>':
                case '<':
                case EVAL_GE:
                case EVAL_LE:
                case EVAL_EQU:
                case EVAL_NEQ:
                    result = evaluate_cmp(
                        data->data ,
                        preresult->data ,
                        code
                    );
                    
                    if ( result[0] == '0' || isempty( stackdata ) )
                    {
                        bContinue = 0;
                    }
                    else
                    {
                        memfree( result );
                        result = data->data;
                        data = NULL;
                    }
                    break;
                default:
                    log_error( "error" );
                    bContinue = 0;
                    memfree( preresult->data );
                    memfree( preresult );
                    preresult = NULL;
                    break;
                }
                
                if ( data )
                {
                    memfree( data->data );
                    memfree( data );
                }
                
                if ( preresult )
                {
                    memfree( preresult->data );
                    preresult->data = result;
                }
            }
            else
            {
                curcode = memalloc_zero( sizeof( EVAL_CODE ) );
                ret = curcode;
                curcode->op = '=';
                curcode->value.type = preresult->type;
                curcode->value.data = preresult->data;
                memfree( preresult );
                preresult = NULL;
            }
        }
        
        if ( ret != NULL )
        {
            curcode->next = memalloc_zero( sizeof( EVAL_CODE ) );
            curcode = curcode->next;
            
            curcode->op = code;
            curcode->value.type = data->type;
            curcode->value.data = data->data;
            
            memfree( data );
        }
    }
    
    if ( ret == NULL )
    {
        if ( preresult == NULL )
        {
            return 0;
        }
        else
        {
            push( stackdata , preresult );
        }
    }
    else
    {
        push( stackdata , new_eval_value( EVT_SUBEVAL , ret ) );
    }
    
    return 1;
}

EVAL_VALUE* build_evaluate(
    NODE_PARAM* param   ,
    bchar*      str     ,
    SLINT       str_len ,
    SLINT*      pos
) {
    EVAL_VALUE* ret;
    CLAUSE      clause;
    bchar*      tempstr;
    STACK*      stackdata;
    STACK*      stackcode;
    SLINT       stack_status = 0;
    SLINT       code_level = 0;
    SLINT       rkey = 0;
    SLINT       backpos;
    
    ret = NULL;
    memset( &clause , 0 , sizeof( CLAUSE ) );
    stackdata = create_stack( 1000 );
    stackcode = create_stack( 1000 );
    
    while ( *pos < str_len )
    {
        e_shift_clause( str , str_len , pos , &clause , stack_status );
        
        if ( clause.type == CT_ENDFILE )
        {
            break;
        }
        else if ( clause.type == CT_ERRORWORD )
        {
            free_clause( &clause );
            free_stack_data( stackdata );
            release_stack( stackdata );
            release_stack( stackcode );
            log_error( "error" );
            return ret;
        }
        
        if ( clause.type == CT_KEY && clause.key == ')' )
        {
            break;
        }
        
        if ( clause.type == CT_VARIABLE
            || clause.type == CT_CONST
            || ( clause.type == CT_KEY && clause.key == '(' ) )
        {
            if ( stack_status > 0 )
            {
                free_clause( &clause );
                free_stack_data( stackdata );
                release_stack( stackdata );
                release_stack( stackcode );
                log_error( "error" );
                return ret;
            }
            
            if ( clause.type == CT_VARIABLE )
            {
                push(
                    stackdata ,
                    new_eval_value( EVT_VARIABLE , clause.constvalue )
                );
                clause.constvalue = NULL;
            }
            else if ( clause.type == CT_CONST )
            {
                push(
                    stackdata ,
                    new_eval_value( EVT_NUMBER , clause.constvalue )
                );
                clause.constvalue = NULL;
            }
            else
            {
                EVAL_VALUE* subret = build_evaluate(
                    param   ,
                    str     ,
                    str_len ,
                    pos
                );
                
                if ( subret == NULL )
                {
                    free_clause( &clause );
                    free_stack_data( stackdata );
                    release_stack( stackdata );
                    release_stack( stackcode );
                    log_error( "error" );
                    return ret;
                }
                
                push( stackdata , subret );
            }
            
            stack_status++;
        }
        else if ( clause.type == CT_KEY )
        {
            if ( stack_status == 0 )
            {
                free_clause( &clause );
                free_stack_data( stackdata );
                release_stack( stackdata );
                release_stack( stackcode );
                log_error( "error" );
                return ret;
            }
            
            stack_status = 0;
            
            switch ( clause.key )
            {
            case '+':
            case '-':
                if ( code_level <= 1 )
                {
                    push( stackcode , INT2PTR( clause.key ) );
                }
                else
                {
                    if ( ! stack_downlevel( stackcode , stackdata , 2 ) )
                    {
                        free_clause( &clause );
                        free_stack_data( stackdata );
                        release_stack( stackdata );
                        release_stack( stackcode );
                        log_error( "error" );
                        return ret;
                    }
                    
                    push( stackcode , INT2PTR( clause.key ) );
                    code_level = 1;
                }
                break;
            case '*':
            case '/':
                push( stackcode , INT2PTR( clause.key ) );
                code_level = 2;
                break;
            case '>':
            case '<':
            case '=':
            case '!':
                backpos = *pos;
                rkey = shift_word( str , str_len , pos );
                
                if ( rkey != '=' )
                {
                    if ( clause.key == '=' || clause.key == '!' )
                    {
                        free_clause( &clause );
                        free_stack_data( stackdata );
                        release_stack( stackdata );
                        release_stack( stackcode );
                        log_error( "error" );
                        return ret;
                    }
                    
                    (*pos) = backpos;
                    rkey = clause.key;
                }
                else
                {
                    rkey = clause.key | 0x10000;
                }
                
                if ( code_level == 0 )
                {
                    push( stackcode , INT2PTR( rkey ) );
                }
                else
                {
                    if ( ! stack_downlevel( stackcode , stackdata , 1 ) )
                    {
                        free_clause( &clause );
                        free_stack_data( stackdata );
                        release_stack( stackdata );
                        release_stack( stackcode );
                        log_error( "error" );
                        return ret;
                    }
                    
                    push( stackcode , INT2PTR( rkey ) );
                    code_level = 0;
                }
                
                break;
            default:
                free_clause( &clause );
                free_stack_data( stackdata );
                release_stack( stackdata );
                release_stack( stackcode );
                log_error( "error" );
                return ret;
            }
        }
        else
        {
            free_clause( &clause );
            free_stack_data( stackdata );
            release_stack( stackdata );
            release_stack( stackcode );
            log_error( "error" );
            return ret;
        }
    }
    
    if ( ! stack_downlevel( stackcode , stackdata , 0 ) )
    {
        free_clause( &clause );
        free_stack_data( stackdata );
        release_stack( stackdata );
        release_stack( stackcode );
        log_error( "error" );
        return ret;
    }
    
    ret = pop( stackdata );
    free_clause( &clause );
    free_stack_data( stackdata );
    release_stack( stackdata );
    release_stack( stackcode );
    
    return ret;
}

bchar* get_eval_value( NODE_PARAM* param , EVAL_VALUE* eval )
{
    if ( eval->type == EVT_NUMBER )
    {
        return dup_str( eval->data );
    }
    else if ( eval->type == EVT_VARIABLE )
    {
        return copy_param_value( param , eval->data );
    }
    else
    {
        return run_evaluate( param , eval );
    }
}

bchar* run_evaluate( NODE_PARAM* param , EVAL_VALUE* eval )
{
    bchar*      result;
    bchar*      data;
    bchar*      preresult;
    EVAL_CODE*  code;
    
    if ( eval->type == EVT_NUMBER )
    {
        return dup_str( eval->data );
    }
    else if ( eval->type == EVT_VARIABLE )
    {
        return copy_param_value( param , eval->data );
    }
    
    code = ( EVAL_CODE* )eval->data;
    
    if ( code->op != '=' )
    {
        return NULL;
    }
    
    preresult = get_eval_value( param , &code->value );
    code = code->next;
    
    while ( code )
    {
        data = get_eval_value( param , &code->value );
        
        switch ( code->op )
        {
        case '=':
            memfree( preresult );
            preresult = data;
            break;
        case '+':
            result = evaluate_add( data , preresult );
            break;
        case '-':
            result = evaluate_sub( data , preresult );
            break;
        case '*':
            result = evaluate_mul( data , preresult );
            break;
        case '/':
            result = evaluate_div( data , preresult );
            break;
        case '>':
        case '<':
        case EVAL_GE:
        case EVAL_LE:
        case EVAL_EQU:
        case EVAL_NEQ:
            result = evaluate_cmp( data , preresult , code->op );
            
            if ( result[0] == '0' || ( code->next == NULL ) )
            {
            }
            else
            {
                memfree( result );
                result = data;
                data = NULL;
            }
            break;
        default:
            log_error( "error" );
            memfree( data );
            memfree( preresult );
            return NULL;
        }
        
        if ( data )
        {
            memfree( data );
        }
        
        if ( preresult )
        {
            memfree( preresult );
        }
        
        preresult = result;
        code = code->next;
    }
    
    return preresult;
}

void free_evaluate( EVAL_VALUE* eval )
{
    if ( eval )
    {
        if ( eval->type == EVT_SUBEVAL )
        {
            free_stack_code( ( EVAL_CODE* )eval->data );
        }
        else
        {
            memfree( eval->data );
        }
        
        memfree( eval );
    }
}

SLINT dump_evaluate_code( EVAL_CODE* eval , FILE_DESC* pf )
{
    EVAL_CODE* temp;
    SLINT count;
    
    if ( eval == NULL )
    {
        count = 0;
        filewrite( pf , &count , INT_SIZE );
        return 1;
    }
    
    count = 0;
    temp = eval;
    
    while ( temp )
    {
        count++;
        temp = temp->next;
    }
    
    filewrite( pf , &count , INT_SIZE );
    
    while ( eval )
    {
        filewrite( pf , &eval->op , INT_SIZE );
        dump_evaluate( &eval->value , pf );
        eval = eval->next;
    }
    
    return 1;
}

SLINT dump_evaluate( EVAL_VALUE* eval , FILE_DESC* pf )
{
    SLINT count;
    
    if ( eval == NULL )
    {
        count = 0;
        filewrite( pf , &count , INT_SIZE );
    }
    else
    {
        count = 1;
        filewrite( pf , &count , INT_SIZE );
        
        filewrite( pf , &eval->type , INT_SIZE );
        
        if ( eval->type == EVT_SUBEVAL )
        {
            dump_evaluate_code( ( EVAL_CODE* ) eval->data , pf );
        }
        else
        {
            count = strlen( eval->data );
            filewrite( pf , &count , INT_SIZE );
            filewrite( pf , eval->data , count );
        }
    }
    
    return 1;
}

EVAL_CODE* load_evaluate_code( FILE_DESC* pf , SLINT* haverror )
{
    SLINT       i;
    SLINT       count;
    EVAL_CODE*  ret;
    EVAL_CODE*  cur;
    
    if ( fileread( pf , &count , INT_SIZE ) != INT_SIZE )
    {
        *haverror = 1;
        return NULL;
    }
    
    if ( count == 0 )
    {
        return NULL;
    }
    
    ret = NULL;
    cur = NULL;
    
    for ( i = 0; i < count; i++ )
    {
        if ( cur == NULL )
        {
            ret = memalloc_zero( sizeof( EVAL_CODE ) );
            cur = ret;
        }
        else
        {
            cur->next = memalloc_zero( sizeof( EVAL_CODE ) );
            cur = cur->next;
        }
        
        if ( fileread( pf , &cur->op , INT_SIZE ) != INT_SIZE )
        {
            *haverror = 1;
            free_stack_code( ret );
            return NULL;
        }
        
        EVAL_VALUE* value = load_evaluate( pf , haverror );
        
        if ( value == NULL || *haverror )
        {
            *haverror = 1;
            free_stack_code( ret );
            return NULL;
        }
        
        cur->value.type = value->type;
        cur->value.data = value->data;
        memfree( value );
    }
    
    return ret;
}

EVAL_VALUE* load_evaluate( FILE_DESC* pf , SLINT* haverror )
{
    EVAL_VALUE* ret;
    SLINT       count;
    
    if ( fileread( pf , &count ,INT_SIZE ) != INT_SIZE )
    {
        *haverror = 1;
        return NULL;
    }
    
    if ( count == 0 )
    {
        return NULL;
    }
    
    ret = memalloc_zero( sizeof( EVAL_VALUE ) );
    
    if ( fileread( pf , &ret->type , INT_SIZE ) != INT_SIZE )
    {
        *haverror = 1;
        free_evaluate( ret );
        return NULL;
    }
    
    if ( ret->type == EVT_SUBEVAL )
    {
        ret->data = ( char* )load_evaluate_code( pf , haverror );
        
        if ( *haverror )
        {
            free_evaluate( ret );
            return NULL;
        }
    }
    else
    {
        if ( fileread( pf , &count , INT_SIZE ) != INT_SIZE )
        {
            *haverror = 1;
            free_evaluate( ret );
            return NULL;
        }
        
        if ( count > 0 )
        {
            ret->data = memalloc_zero( count + 1 );
            
            if ( fileread( pf , ret->data , count ) != count )
            {
                *haverror = 1;
                free_evaluate( ret );
                return NULL;
            }
        }
    }
    
    return ret;
}

