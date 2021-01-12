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
#include <stdint.h>
#include <string.h>

#include "stypes.h"
#include "memalloc.h"
#include "file.h"
#include "logger.h"
#include "map.h"
#include "errorstr.h"
#include "slang.h"
#include "slanglex.h"

typedef struct _CODE_PARSE_STATUS   CODE_PARSE_STATUS;

struct _CODE_PARSE_STATUS
{
    CODE*           cur_code;
    NODE_PARAM*     param;
    char*           src_buf;
    int             src_fsize;
    int*            cur_pos;
    int*            errorno;
    int*            errstartpos;
    int*            errpos;
    char*           errmsg;
};

static int wstrncpy( char* dst , char* src , int max_len );

static int wstrncat( char* dst , char* src , int max_len );

static int shift_clause(
    NODE_PARAM*     param       ,
    char*           src_buf     ,
    int             src_fsize   ,
    int*            cur_pos     ,
    CLAUSE*         clause
);

CODE* parse_code_buffer( CODE_PARSE_STATUS* pcps );

enum KEYWORD
{
    KWI_NODEBEGIN = 0 ,
#define KEY_NODEBEGIN           "."

    KWI_NODEDEF ,
#define KEY_NODEDEF             "."

    KWI_GETITEMCOUNT ,
#define KEY_GETITEMCOUNT        "#"

    KWI_IF ,
#define KEY_IF                  "?"

    KWI_ELSE ,
#define KEY_ELSE                "~"

    KWI_WHILE ,
#define KEY_WHILE               "@"

    KWI_PLEFT ,
#define KEY_PLEFT               "("

    KWI_PRIGHT ,
#define KEY_PRIGHT              ")"

    KWI_ALEFT ,
#define KEY_ALEFT               "<"

    KWI_ARIGHT ,
#define KEY_ARIGHT              ">"

    KWI_MLEFT ,
#define KEY_MLEFT               "["

    KWI_MRIGHT ,
#define KEY_MRIGHT              "]"

    KWI_BLEFT ,
#define KEY_BLEFT               "{"

    KWI_BRIGHT ,
#define KEY_BRIGHT              "}"

    KWI_COMMA ,
#define KEY_COMMA               ","

    KWI_SEMICOLON ,
#define KEY_SEMICOLON           ";"

    KWI_EQUAL ,
#define KEY_EQUAL              "="

    KWI_ASTERISK ,
#define KEY_ASTERISK            "*"

    KWI_COMPARE ,
#define KEY_COMPARE             "=="

    KWI_NEQUAL ,
#define KEY_NEQUAL              "!="

    KWI_LESSEQU ,
#define KEY_LESSEQU             "<="

    KWI_GREATEREQU ,
#define KEY_GREATEREQU          ">="

    KWI_QUOTBEGIN ,
#define KEY_QUOTBEGIN           "\""

    KWI_QUOTEND ,
#define KEY_QUOTEND             "\""

    KWI_ESCAPE ,
#define KEY_ESCAPE              "\\"

    KWI_COMMENTBEGIN ,
#define KEY_COMMENTBEGIN        "/*"

    KWI_COMMENTEND ,
#define KEY_COMMENTEND          "*/"

    KWI_SPACE ,
#define KEY_SPACE               " "

    KWI_NULL
};

static int key_count = 0;

static const char* key_list[] = {
    KEY_NODEDEF         ,
    KEY_NODEBEGIN       ,
    KEY_GETITEMCOUNT    ,
    KEY_IF              ,
    KEY_ELSE            , 
    KEY_WHILE           , 
    KEY_PLEFT           ,
    KEY_PRIGHT          ,
    KEY_ALEFT           ,
    KEY_ARIGHT          ,
    KEY_MLEFT           ,
    KEY_MRIGHT          ,
    KEY_BLEFT           ,
    KEY_BRIGHT          ,
    KEY_COMMA           ,
    KEY_SEMICOLON       ,
    KEY_EQUAL           ,
    KEY_ASTERISK        ,
    KEY_COMPARE         ,
    KEY_NEQUAL          ,
    KEY_LESSEQU         ,
    KEY_GREATEREQU      ,
    KEY_QUOTBEGIN       ,
    KEY_QUOTEND         ,
    KEY_ESCAPE          ,
    KEY_COMMENTBEGIN    ,
    KEY_COMMENTEND      ,
    KEY_SPACE           ,
    NULL
};

word shift_word(
    char*   src_buf     ,
    int     src_fsize   ,
    int*    cur_pos
) {
    char    c;
    int     i;
    word    w;
    
    if ( *cur_pos >= src_fsize )
    {
        return 0;
    }
    
    c = src_buf[ ( *cur_pos )++ ];
    w = ( word )c;
    
    if ( ( 0x80 & c ) != 0 )
    {
        for ( i = 1; ( i < 6 ) && ( ( ( 0x80 >> i ) & c ) != 0 )
            && ( *cur_pos < src_fsize ); NOOP )
        {
            i++;
            w = ( ( w << 6 )
                | ( src_buf[ ( *cur_pos )++ ] & 0x3f ) )
                & ( 0x7fffffff >> ( ( 5 - i ) * 5 ) );
        }
    }
    
    return w;
}

/**
 * return keyword index
 * */
int shift_keyword(
    char*   src_buf     ,
    int     src_fsize   ,
    int*    cur_pos     ,
    BOOL    asc
) {
    int     i;
    int     n;
    int     kwi;
    int     kwl;
    char*   buf;
    int     buf_len;
    
    if ( *cur_pos >= src_fsize )
    {
        return -1;
    }
    
    buf = src_buf + *cur_pos;
    buf_len = src_fsize - *cur_pos;
    kwi = -1;
    kwl = 0;
    
    if ( asc )
    {
        for ( i = 0; key_list[i] != NULL; i++ )
        {
            n = strlen( key_list[i] );
            
            if ( n > buf_len )
            {
                continue;
            }
            
            if ( strncmp( buf , key_list[i] , n ) == 0 )
            {
                if ( n > kwl )
                {
                    kwl = n;
                    kwi = i;
                }
            }
        }
    }
    else
    {
        if ( key_count == 0 )
        {
            for ( i = 0; key_list[i] != NULL; i++ ){};       
            key_count = i - 1;
        }
        
        for ( i = key_count; i >= 0 ; i-- )
        {
            if ( key_list[i] == NULL )
            {
                continue;
            }
            
            n = strlen( key_list[i] );
            
            if ( n > buf_len )
            {
                continue;
            }
            
            if ( strncmp( buf , key_list[i] , n ) == 0 )
            {
                if ( n > kwl )
                {
                    kwl = n;
                    kwi = i;
                }
            }
        }
    }
    
    if ( kwi != -1 )
    {
        *cur_pos += kwl;
    }
    
    return kwi;
}

BOOL is_space( word c )
{
    return ( ( c == ( word )' ' )
        || ( c == ( word )'\t' )
        || ( c == ( word )'\n' )
        || ( c == ( word )'\r' ) );
}

BOOL is_number( word c )
{
    return ( c >= ( word )'0' && c <= ( word )'9' );
}

static bchar* specword_list = ".#?~@()<>[]{},:;=*!'/`%&^-+|\\\"";

BOOL is_specword( word c )
{
    SLINT i;
    
    for ( i = 0; i < strlen( specword_list ); i++ )
    {
        if ( c == specword_list[i] )
        {
            return TRUE;
        }
    }
    
    return FALSE;
}

BOOL is_namestart( word c )
{
    return ( !is_number( c ) )
        && ( ! is_space( c ) )
        && ( ! is_specword( c ) );
}

BOOL is_name( word c )
{
    return ( ! is_space( c ) )
        && ( ! is_specword( c ) );
}

static void shift_space( char* src_buf , int src_fsize , int* cur_pos )
{
    word    w;
    int     pos;
    
    pos = *cur_pos;
    
    do
    {
        *cur_pos = pos;
        w = shift_word( src_buf , src_fsize , &pos );
    }
    while ( is_space( w ) );
}

static int commentbegin_len = 0;
static int commentend_len   = 0;

static void shift_space_comment( char* src_buf , int src_fsize , int* cur_pos )
{
    word    w;
    int     status;
    int     pos;
    int     cb_len;
    int     ce_len;
    
    status = 0;
    pos = *cur_pos;
    
    if ( commentbegin_len == 0 )
    {
        commentbegin_len = strlen( KEY_COMMENTBEGIN );
        commentend_len = strlen( KEY_COMMENTEND );
    }
    
    while ( pos < src_fsize )
    {
        if ( status == 0 )
        {
            w = shift_word( src_buf , src_fsize , &pos );
            
            if ( is_space( w ) )
            {
                *cur_pos = pos;
                continue;
            }
            
            if ( ( ( src_fsize - *cur_pos ) >= commentbegin_len )
                && ( strncmp(
                    src_buf + *cur_pos ,
                    KEY_COMMENTBEGIN ,
                    commentbegin_len
                ) == 0 ) )
            {
                status = 1;
                pos = ( *cur_pos ) + commentbegin_len;
                *cur_pos = pos;
                continue;
            }
            
            break;
        }
        else
        {
            if ( ( ( src_fsize - pos ) >= commentend_len )
                && ( strncmp( src_buf + pos , KEY_COMMENTEND , commentend_len )
                    == 0 ) )
            {
                status = 0;
                pos += commentend_len;
            }
            else
            {
                shift_word( src_buf , src_fsize , &pos );
            }
            
            *cur_pos = pos;
        }
    }
}

word shift_word_skip_space(
    char*   src_buf     ,
    int     src_fsize   ,
    int*    cur_pos
) {
    shift_space( src_buf , src_fsize , cur_pos );
    return shift_word( src_buf , src_fsize , cur_pos );
}

static word shift_word_skip_space_comment(
    char*   src_buf     ,
    int     src_fsize   ,
    int*    cur_pos
) {
    shift_space_comment( src_buf , src_fsize , cur_pos );
    return shift_word( src_buf , src_fsize , cur_pos );
}

static int wordlen( char c )
{
    int     i;
    
    if ( ( 0x80 & c ) != 0 )
    {
        for ( i = 1; i < 6 ; i++ )
        {
            if ( ( ( 0x80 >> i ) & c ) == 0 )
            {
                break;
            }
        }
        
        return i;
    }
    else
    {
        return 1;
    }
}

void print_error(
    char*   filename        ,
    char*   buf             ,
    int     buf_size        ,
    int     errorno         ,
    int     errstartpos     ,
    int     errpos          ,
    char*   errmsg
) {
    int     lineno;
    int     linepos;
    int     posno;
    int     i;
    word    w;
    int     backpos;
    char*   line;
    char*   p;
    
    if ( errpos >= buf_size )
    {
        log_error( "%s[unknown](%d)%s" , filename , errorno , errmsg );
        return;
    }
    
    shift_space_comment( buf , buf_size , &errstartpos );
    
    if ( errpos < errstartpos )
    {
        errpos = errstartpos;
    }
    
    lineno  = 1;
    linepos = 0;
    i       = 0;
    
    while ( i < errpos )
    {
        backpos = i;
        w       = shift_word( buf , buf_size , &i );
        
        if ( w == ( word )'\n')
        {
            lineno++;
            linepos = i;
        }
        else if ( w == ( word )'\r' )
        {
            linepos = i;
        }
    }
    
    posno = errpos - linepos;
    
    log_error( "%s [%d:%d] %s(%d)" ,
        filename , lineno , posno , errmsg , errorno
    );
    
    /* output source info */
    i = errpos;
    
    while ( ( i < buf_size ) && ( i < ( errpos + 128 ) ) )
    {
        w = shift_word( buf , buf_size , &i );
        
        if ( w == ( word )'\n' )
        {
            break;
        }
    }
    
    lineno = i - errstartpos;
    line   = memalloc_zero( lineno + 128 );
    
    memcpy( line , buf + errstartpos , lineno );
    
    /* output error position */
    posno = 0;
    
    for ( i = errstartpos; i < errpos; )
    {
        w = shift_word( buf , buf_size , &i );
        
        if ( w > 0x7f )
        {
            posno += 2;
        }
        else
        {
            posno++;
        }
    }
    
    p = line + lineno;
    
    for ( i = 0; i < posno; i++ )
    {
        if ( line[i] == '\t' )
        {
            p[i] = '\t';
        }
        else
        {
            p[i] = ' ';
        }
    }
    
    p[i] = 0;
    
    log_info( "\n--------\n%s^\n--------" , line );
    
    memfree( line );
    
    return;
}

static int wstrncpy( char* dst , char* src , int max_len )
{
    int     i;
    int     j;
    char    c;
    int     srclen;
    
    srclen = strlen( src );
    
    if ( max_len > srclen )
    {
        max_len = srclen;
    }
    
    max_len++;
    
    memset( dst , 0 , max_len );
    
    for ( i = 0; i < max_len; )
    {
        c = src[i];
        j = wordlen( c );
        
        if ( i + j >= max_len )
        {
            break;
        }
        
        memcpy( dst + i , src + i , j );
        i += j;
    }
    
    return i;
}

static int wstrncat( char* dst , char* src , int max_len )
{
    return wstrncpy( dst + strlen( dst ) , src , max_len );
}

void set_clause_info(
    char*   src_buf     ,
    int     start_pos   ,
    int     end_pos     ,
    CLAUSE* clause      ,
    int     type   ,
    word    key
) {
    int     len;
    int     i;
    int     backpos;
    word    w;
    
    if ( end_pos == -1 )
    {
        clause->type = CT_ERRORWORD;
        return;
    }
    
    clause->bufpos      = start_pos;
    clause->constvalue  = NULL;
    
    if ( type == CT_CONST || type == CT_NODE )
    {
        len = end_pos - start_pos;
        clause->constvalue = memalloc_zero( len + 1 );
        
        if ( type == CT_CONST )
        {
            while ( start_pos < end_pos )
            {
                backpos = start_pos;
                w = shift_word( src_buf , end_pos , &start_pos );
                
                if ( w == '\\' )
                {
                    backpos = start_pos;
                    w = shift_word( src_buf , end_pos , &start_pos );
                }
                
                wstrncat(
                    clause->constvalue  ,
                    src_buf + backpos   ,
                    start_pos - backpos
                );
            }
        }
        else
        {
            wstrncpy( clause->constvalue , src_buf + start_pos , len );
        }
        
        clause->name[0] = 0;
    }
    else
    {
        len = MIN( MAX_WORD_NAME_LEN - 1 , end_pos - start_pos );
        wstrncpy( clause->name , src_buf + start_pos , len );
    }
    
    clause->type = type;
    clause->key = key;
}

int get_varindex( NODE_PARAM* param , char* name )
{
    int ret;
    
    if ( param == NULL || param->hmap == NULL )
    {
        return -1;
    }
    
    ret = ( int )dmap_query( param->hmap , name );
    
    if ( ret == 0 )
    {
        return -1;
    }
    
    return ret - 1; /* start with 1, so subtract 1 */
}

/**
 * return name length 
 * */
int shift_namestr( char* src_buf , int src_fsize , int* cur_pos )
{
    word    w;
    int     pos;
    int     start;
    int     kwi;
    
    shift_space_comment( src_buf , src_fsize , cur_pos );
    
    pos = *cur_pos;
    kwi = shift_keyword( src_buf , src_fsize , &pos , TRUE );
    
    if ( kwi != -1 )
    {
        return 0;
    }
    
    start = *cur_pos;
    shift_word( src_buf , src_fsize , cur_pos );
    
    pos = *cur_pos;
    
    while ( pos < src_fsize )
    {
        w = shift_word( src_buf , src_fsize , &pos );
        
        if ( is_space( w ) || is_specword( w ) )
        {
            break;
        }
        
        *cur_pos = pos;
    }
    
    return *cur_pos - start;
}

static int shift_clause(
    NODE_PARAM*     param       ,
    char*           src_buf     ,
    int             src_fsize   ,
    int*            cur_pos     ,
    CLAUSE*         clause
) {
    int     kwi;
    int     len;
    int     start_pos;
    int     end_pos;
    word    w;
    char    name[MAX_NAME_LEN];
    
    if ( clause->constvalue != NULL )
    {
        memfree( clause->constvalue );
        clause->constvalue = NULL;
    }
    
    shift_space_comment( src_buf , src_fsize , cur_pos );
    
    if ( *cur_pos >= src_fsize )
    {
        clause->type = CT_ENDFILE;
        return -2; /* read end */
    }
    
    start_pos = *cur_pos;
    
    kwi = shift_keyword( src_buf , src_fsize , cur_pos , TRUE );
    
    switch ( kwi )
    {
    case KWI_NODEDEF:
    case KWI_NODEBEGIN:
        start_pos = *cur_pos;
        
        name[0] = 0;
        
        while ( TRUE )
        {
            CLAUSE  follow_clause;
            int     backpos;
            int     nextkwi;
            
            len = shift_namestr( src_buf , src_fsize , cur_pos );
            
            if ( len == 0 )
            {
                clause->constvalue = dup_str( ERROR_NODENAME );
                clause->type = CT_ERRORWORD;
                return -1;
            }
            
            if ( strlen( name ) + strlen( KEY_NODEBEGIN ) + len
                >= MAX_NAME_LEN )
            {
                clause->constvalue = dup_str( ERROR_NODENAME );
                clause->type = CT_ERRORWORD;
                return -1;
            }
            
            strcat( name , KEY_NODEBEGIN );
            strncat( name , src_buf + *cur_pos - len , len );
            
            shift_space_comment( src_buf , src_fsize , cur_pos );
            backpos = *cur_pos;
            nextkwi = shift_keyword( src_buf , src_fsize , cur_pos , TRUE );
            
            if ( nextkwi != KWI_NODEBEGIN )
            {
                *cur_pos = backpos;
                break;
            }
        }
        
        set_clause_info(
            name            ,
            0               ,
            strlen( name )  ,
            clause          ,
            CT_NODE         ,
            0
        );
        
        return *cur_pos;
        
    case KWI_GETITEMCOUNT:
    case KWI_IF:
    case KWI_ELSE:
    case KWI_WHILE:
    case KWI_PLEFT:
    case KWI_PRIGHT:
    case KWI_ALEFT:
    case KWI_ARIGHT:
    case KWI_MLEFT:
    case KWI_MRIGHT:
    case KWI_BLEFT:
    case KWI_BRIGHT:
    case KWI_COMMA:
    case KWI_SEMICOLON:
    case KWI_EQUAL:
    case KWI_ASTERISK:
        set_clause_info(
            src_buf     ,
            start_pos   ,
            *cur_pos    ,
            clause      ,
            CT_KEY      ,
            kwi
        );
        return *cur_pos;
        
    case KWI_QUOTBEGIN:
        start_pos = *cur_pos;
        
        while ( *cur_pos < src_fsize )
        {
            end_pos = *cur_pos;
            kwi = shift_keyword( src_buf , src_fsize , cur_pos , FALSE );
            
            if ( kwi == KWI_QUOTEND )
            {
                break;
            }
            else if ( kwi == KWI_ESCAPE )
            {
                shift_word( src_buf , src_fsize , cur_pos );
            }
            else if ( kwi == -1 )
            {
                shift_word( src_buf , src_fsize , cur_pos );
            }
        }
        
        if ( kwi != KWI_QUOTEND )
        {
            clause->constvalue = dup_str( ERROR_STRING );
            clause->type = CT_ERRORWORD;
            return -1;
        }
        
        set_clause_info(
            src_buf     ,
            start_pos   ,
            end_pos     ,
            clause      ,
            CT_CONST    ,
            0
        );
        return end_pos;
        
    default:
        ( *cur_pos ) = start_pos;
        len = shift_namestr( src_buf , src_fsize , cur_pos );
        
        if ( len == 0 )
        {
            clause->constvalue = dup_str( ERROR_KEYWORD );
            clause->type = CT_ERRORWORD;
            return -1;
        }
        
        end_pos = start_pos + len;
        set_clause_info(
            src_buf     ,
            start_pos   ,
            end_pos     ,
            clause      ,
            CT_VARIABLE ,
            0
        );
        
        clause->index = get_varindex( param , clause->name );
        
        if ( clause->index == -1 )
        {
            log_error( "undefined reference to (%s)" , clause->name );
            clause->constvalue = dup_str( ERROR_VARIABLE );
            clause->type = CT_ERRORWORD;
            return -1;
        }
        
        return end_pos;
    }
    
    clause->constvalue = dup_str( ERROR_KEYWORD );
    clause->type = CT_ERRORWORD;
    return -1;
}

static int peek_clause(
    NODE_PARAM*     param       ,
    char*           src_buf     ,
    int             src_fsize   ,
    int*            cur_pos     ,
    CLAUSE*         clause
) {
    int pos;
    int ret;
    
    pos = *cur_pos;
    ret = shift_clause( param , src_buf , src_fsize , &pos , clause );
    
    if ( clause->constvalue != NULL )
    {
        memfree( clause->constvalue );
        clause->constvalue = NULL;
    }
    
    return ret;
}

static int shift_clause_cps(
    CODE_PARSE_STATUS*  pcps    ,
    CLAUSE*             clause
) {
    return shift_clause(
        pcps->param         ,
        pcps->src_buf       ,
        pcps->src_fsize     ,
        pcps->cur_pos       ,
        clause
    );
}

void free_clause( CLAUSE* clause )
{
    if ( clause->constvalue != NULL )
    {
        memfree( clause->constvalue );
        clause->constvalue = NULL;
    }
}

int parse_call(
    CODE_PARSE_STATUS*  pcps        ,
    CODE*               prev_code   /* prev_code is processed tran_code_param */
) {
    int         ret;
    CLAUSE      follow_clause;
    CODE_VALUE* value;
    CODE_VALUE* valueflag;
    int         async;
    int         call_param_count;
    int         call_return_count;
    
    ret = 1;
    async = 0;
    memset( &follow_clause , 0 , sizeof( CLAUSE ) );
    pcps->cur_code->op = OP_CALL;
    value = pcps->cur_code->value;
    
    valueflag = memalloc_zero( sizeof( CODE_VALUE ) );
    valueflag->type = CVT_VARIABLE;
    valueflag->index = 0;
    value->next = valueflag;
    value = valueflag;
    
    call_param_count = 0;
    call_return_count = 0;
    
    if ( prev_code == NULL )
    {
        while ( TRUE )
        {
            shift_clause_cps( pcps , &follow_clause );
            
            if ( ( follow_clause.type == CT_KEY )
                && ( follow_clause.key == KWI_PRIGHT )
                && ( call_param_count == 0 ) )
            {
                break;
            }
            
            if ( follow_clause.type == CT_VARIABLE )
            {
                value->next = memalloc_zero( sizeof( CODE_VALUE ) );
                value = value->next;
                
                value->type = CVT_VARIABLE;
                value->index = follow_clause.index;
            }
            else if ( follow_clause.type == CT_CONST )
            {
                value->next = memalloc_zero( sizeof( CODE_VALUE ) );
                value = value->next;
                
                value->data = follow_clause.constvalue;
                follow_clause.constvalue = NULL;
                value->type = CVT_CONST;
            }
            else if ( ( follow_clause.type == CT_KEY )
                && ( follow_clause.key == KWI_ASTERISK ) )
            {
                value->next = memalloc_zero( sizeof( CODE_VALUE ) );
                value = value->next;
                
                value->data = NULL;
                value->type = CVT_INIT;
            }
            else
            {
                *pcps->errorno = __LINE__;
                *pcps->errpos = *pcps->cur_pos;
                strcpy( pcps->errmsg , ERROR_WORD );
                free_clause( &follow_clause );
                return 0;
            }
            
            call_param_count++;
            
            shift_clause_cps( pcps , &follow_clause );
            
            if ( follow_clause.type == CT_KEY )
            {
                if ( follow_clause.key == KWI_PRIGHT )
                {
                    break;
                }
                else if ( follow_clause.key == KWI_COMMA )
                {
                }
                else
                {
                    *pcps->errorno = __LINE__;
                    *pcps->errpos = *pcps->cur_pos;
                    strcpy( pcps->errmsg , ERROR_WORD );
                    free_clause( &follow_clause );
                    return 0;
                }
            }
            else
            {
                *pcps->errorno = __LINE__;
                *pcps->errpos = *pcps->cur_pos;
                strcpy( pcps->errmsg , ERROR_WORD );
                free_clause( &follow_clause );
                return 0;
            }
        }
        
        shift_clause_cps( pcps , &follow_clause );
        
        if ( ( follow_clause.type != CT_KEY )
            || ( follow_clause.key != KWI_ARIGHT ) )
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_WORD );
            free_clause( &follow_clause );
            return 0;
        }
    }
    else
    {
        /* take the parameter from the previous function result */
        int prev_return_offset = ( prev_code->value[1].index / 1000 ) + 2;
        call_param_count = ( prev_code->value[1].index % 1000 );
        int i;
        
        for ( i = 0; i < call_param_count; i++ )
        {
            value->next = memalloc_zero( sizeof( CODE_VALUE ) );
            value = value->next;
            
            value->type = prev_code->value[ prev_return_offset + i ].type;
            
            if ( value->type == CVT_VARIABLE )
            {
                value->index = prev_code->value[ prev_return_offset + i ].index;
            }
            else if ( value->type == CVT_INIT )
            {
                value->data = NULL;
            }
            else
            {
                *pcps->errorno = __LINE__;
                *pcps->errpos = *pcps->cur_pos;
                strcpy( pcps->errmsg , ERROR_WORD );
                free_clause( &follow_clause );
                return 0;
            }
        }
    }
    
    shift_clause_cps( pcps , &follow_clause );
    
    if ( ( follow_clause.type == CT_KEY )
        && ( follow_clause.key == KWI_MLEFT ) )
    {
        pcps->cur_code->value->type = CVT_VARIABLE;
        
        shift_clause_cps( pcps , &follow_clause );
        
        /* node name */
        if ( follow_clause.type != CT_VARIABLE )
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_WORD );
            free_clause( &follow_clause );
            return 0;
        }
        
        pcps->cur_code->value->index = follow_clause.index;
        
        shift_clause_cps( pcps , &follow_clause );
        
        if ( ( follow_clause.type != CT_KEY )
            || ( follow_clause.key != KWI_MRIGHT ) )
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_WORD );
            free_clause( &follow_clause );
            return 0;
        }
        
        shift_clause_cps( pcps , &follow_clause );
    }
    else
    {
        pcps->cur_code->value->type = CVT_CONST;
        
        /* node name */
        if ( follow_clause.type == CT_NODE )
        {
            pcps->cur_code->value->data = follow_clause.constvalue;
            follow_clause.constvalue = NULL;
            
            shift_clause_cps( pcps , &follow_clause );
        }
        else
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_WORD );
            free_clause( &follow_clause );
            return 0;
        }
    }
    
    if ( ( follow_clause.type == CT_KEY )
        && ( follow_clause.key == KWI_SEMICOLON ) )
    {
        /* nothing return */
    }
    else
    {
        if ( ( follow_clause.type != CT_KEY ) 
            || ( follow_clause.key != KWI_ARIGHT ) )
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_WORD );
            free_clause( &follow_clause );
            return 0;
        }
        
        shift_clause_cps( pcps , &follow_clause );
        
        if ( ( follow_clause.type == CT_KEY )
            && ( follow_clause.key == KWI_ARIGHT ) )
        {
            /* asynchronous */
            async = 1;
            shift_clause_cps( pcps , &follow_clause );
        }
        
        if ( ( follow_clause.type != CT_KEY )
            || ( follow_clause.key != KWI_PLEFT ) )
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_WORD );
            free_clause( &follow_clause );
            return 0;
        }
        
        while ( TRUE )
        {
            shift_clause_cps( pcps , &follow_clause );
            
            if ( ( follow_clause.type == CT_KEY )
                && ( follow_clause.key == KWI_PRIGHT )
                && ( call_return_count == 0 ) )
            {
                break;
            }
            
            if ( follow_clause.type == CT_VARIABLE )
            {
                value->next = memalloc_zero( sizeof( CODE_VALUE ) );
                value = value->next;
                
                value->type = CVT_VARIABLE;
                value->index = follow_clause.index;
                
            }
            else if ( ( follow_clause.type == CT_KEY )
                && ( follow_clause.key == KWI_ASTERISK ) )
            {
                value->next = memalloc_zero( sizeof( CODE_VALUE ) );
                value = value->next;
                
                value->type = CVT_INIT;
            }
            else
            {
                *pcps->errorno = __LINE__;
                *pcps->errpos = *pcps->cur_pos;
                strcpy( pcps->errmsg , ERROR_WORD );
                free_clause( &follow_clause );
                return 0;
            }
            
            call_return_count++;
            
            shift_clause_cps( pcps , &follow_clause );
            
            if ( follow_clause.type == CT_KEY )
            {
                if ( follow_clause.key == KWI_PRIGHT )
                {
                    break;
                }
                else if ( follow_clause.key == KWI_COMMA )
                {
                }
                else
                {
                    *pcps->errorno = __LINE__;
                    *pcps->errpos = *pcps->cur_pos;
                    strcpy( pcps->errmsg , ERROR_WORD );
                    free_clause( &follow_clause );
                    return 0;
                }
            }
            else
            {
                *pcps->errorno = __LINE__;
                *pcps->errpos = *pcps->cur_pos;
                strcpy( pcps->errmsg , ERROR_WORD );
                free_clause( &follow_clause );
                return 0;
            }
        }
        
        shift_clause_cps( pcps , &follow_clause );
        
        if ( ( follow_clause.type == CT_KEY )
            && ( follow_clause.key == KWI_ARIGHT ) )
        {
            ret = 2;
        }
        else if ( ( follow_clause.type != CT_KEY )
            || ( follow_clause.key != KWI_SEMICOLON ) )
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_WORD );
            free_clause( &follow_clause );
            return 0;
        }
    }
    
    if ( call_param_count > 1000 )
    {
        *pcps->errorno = __LINE__;
        *pcps->errpos = *pcps->cur_pos;
        strcpy( pcps->errmsg , ERROR_PARAM_COUNT );
        free_clause( &follow_clause );
        return 0;
    }
    
    if ( call_return_count > 1000 )
    {
        *pcps->errorno = __LINE__;
        *pcps->errpos = *pcps->cur_pos;
        strcpy( pcps->errmsg , ERROR_RET_COUNT );
        free_clause( &follow_clause );
        return 0;
    }
    
    valueflag->index = ( call_param_count * 1000 + call_return_count );
    
    if ( async )
    {
        valueflag->index |= NODE_ASYNC;
    }
    
    free_clause( &follow_clause );
    return ret;
}

void code_value_array( CODE* code )
{
    int         i;
    int         count;
    CODE_VALUE* temp;
    CODE_VALUE* temp2;
    CODE_VALUE* temp3;
    
    count = 0;
    temp = code->value;
    
    while ( temp )
    {
        count++;
        temp = temp->next;
    }
    
    if ( count == 0 )
    {
        return;
    }
    
    temp2 = memalloc_zero( sizeof( CODE_VALUE ) * count );
    temp = code->value;
    
    for ( i = 0; i < count; i++ )
    {
        temp2[i].type = temp->type;
        temp2[i].data = temp->data;
        temp2[i].index = temp->index;
        temp2[i].next = NULL;
        
        temp3 = temp;
        temp = temp->next;
        memfree( temp3 );
    }
    
    code->value = temp2;
    code->val_array_count = count;
}

void value_link_to_array( NODE_PARAM* param )
{
    int         i;
    PARAM_ITEM* valuearray;
    PARAM_ITEM* tempvalue;
    PARAM_ITEM* tempvalue2;
    
    valuearray = memalloc_zero( sizeof( PARAM_ITEM ) * param->count );
    tempvalue = param->list;
    
    for ( i = 0; i < param->count; i++ )
    {
        memcpy( &valuearray[i] , tempvalue, sizeof( PARAM_ITEM ) );
        valuearray[i].value = NULL;
        tempvalue2 = tempvalue;
        tempvalue = ( PARAM_ITEM* )tempvalue->value;
        memfree( tempvalue2 );
    }
    
    param->list = valuearray;
}

static BOOL parse_expression(
    CODE_PARSE_STATUS*  pcps            ,
    CLAUSE*             pfirst_clause   ,
    CLAUSE*             pfollow_clause
) {
    CODE_VALUE*     cvalue;
    int             kwi;
    
    cvalue = memalloc_zero( sizeof( CODE_VALUE ) );
    pcps->cur_code->value = cvalue;
    
    cvalue->type = CVT_VARIABLE;
    cvalue->index = pfirst_clause->index;
    
    kwi = pfollow_clause->key;
    
    switch ( kwi )
    {
    case KWI_EQUAL:
        pcps->cur_code->op = OP_EQU;
        
        cvalue->next = memalloc_zero( sizeof( CODE_VALUE ) );
        cvalue = cvalue->next;
        
        shift_clause_cps( pcps , pfollow_clause );
        
        if ( pfollow_clause->type == CT_VARIABLE )
        {
            cvalue->type = CVT_VARIABLE;
            cvalue->index = pfollow_clause->index;
            
            peek_clause(
                pcps->param         ,
                pcps->src_buf       ,
                pcps->src_fsize     ,
                pcps->cur_pos       ,
                pfollow_clause
            );
            
            if ( ( pfollow_clause->type == CT_KEY )
                && ( pfollow_clause->key == KWI_MLEFT ) ) /*'['*/
            {
                shift_clause_cps( pcps , pfollow_clause );
                
                pcps->cur_code->op = OP_GETITEM;
                cvalue->next = memalloc_zero( sizeof( CODE_VALUE ) );
                cvalue = cvalue->next;
                
                shift_clause_cps( pcps , pfollow_clause );
                
                if ( pfollow_clause->type == CT_VARIABLE )
                {
                    cvalue->type = CVT_VARIABLE;
                    cvalue->index = pfollow_clause->index;
                }
                else if ( pfollow_clause->type == CT_CONST )
                {
                    cvalue->data = pfollow_clause->constvalue;
                    pfollow_clause->constvalue = NULL;
                    cvalue->type = CVT_CONST;
                }
                else
                {
                    *pcps->errorno = __LINE__;
                    *pcps->errpos = *pcps->cur_pos;
                    strcpy( pcps->errmsg , ERROR_WORD );
                    return FALSE;
                }
                
                shift_clause_cps( pcps , pfollow_clause );
                
                if ( ( pfollow_clause->type != CT_KEY )
                    || ( pfollow_clause->key != KWI_MRIGHT ) )
                {
                    *pcps->errorno = __LINE__;
                    *pcps->errpos = *pcps->cur_pos;
                    strcpy( pcps->errmsg , ERROR_WORD );
                    return FALSE;
                }
            }
        }
        else if ( pfollow_clause->type == CT_CONST )
        {
            cvalue->data = pfollow_clause->constvalue;
            pfollow_clause->constvalue = NULL;
            cvalue->type = CVT_CONST;
        }
        else if ( pfollow_clause->type == CT_KEY )
        {
            if ( pfollow_clause->key == KWI_ASTERISK )
            {
                cvalue->data = NULL;
                cvalue->type = CVT_INIT;
            }
            else if ( ( pfollow_clause->key == KWI_IF )
                || ( pfollow_clause->key == KWI_GETITEMCOUNT ) )
            {
                if ( pfollow_clause->key == KWI_IF )
                {
                    pcps->cur_code->op = OP_EVALUATE;
                }
                else if ( pfollow_clause->key == KWI_GETITEMCOUNT )
                {
                    pcps->cur_code->op = OP_GETITEMCOUNT;
                }
                
                shift_clause_cps( pcps , pfollow_clause );
                
                if ( pfollow_clause->type == CT_VARIABLE )
                {
                    cvalue->type = CVT_VARIABLE;
                    cvalue->index = pfollow_clause->index;
                }
                else if ( pfollow_clause->type == CT_CONST )
                {
                    cvalue->data = pfollow_clause->constvalue;
                    pfollow_clause->constvalue = NULL;
                    cvalue->type = CVT_CONST;
                }
                else
                {
                    *pcps->errorno = __LINE__;
                    *pcps->errpos = *pcps->cur_pos;
                    strcpy( pcps->errmsg , ERROR_WORD );
                    return FALSE;
                }
            }
            else if ( pfollow_clause->key == KWI_MLEFT )
            {
                pcps->cur_code->op = OP_STRFORMAT;
                
                shift_clause_cps( pcps , pfollow_clause );
                
                if ( pfollow_clause->type == CT_VARIABLE )
                {
                    cvalue->type = CVT_VARIABLE;
                    cvalue->index = pfollow_clause->index;
                }
                else if ( pfollow_clause->type == CT_CONST )
                {
                    cvalue->data = pfollow_clause->constvalue;
                    pfollow_clause->constvalue = NULL;
                    cvalue->type = CVT_CONST;
                }
                else
                {
                    *pcps->errorno = __LINE__;
                    *pcps->errpos = *pcps->cur_pos;
                    strcpy( pcps->errmsg , ERROR_WORD );
                    return FALSE;
                }
                
                shift_clause_cps( pcps , pfollow_clause );
                
                if ( pfollow_clause->type != CT_KEY
                    || pfollow_clause->key != KWI_MRIGHT )
                {
                    *pcps->errorno = __LINE__;
                    *pcps->errpos = *pcps->cur_pos;
                    strcpy( pcps->errmsg , ERROR_WORD );
                    return FALSE;
                }
            }
            else
            {
                *pcps->errorno = __LINE__;
                *pcps->errpos = *pcps->cur_pos;
                strcpy( pcps->errmsg , ERROR_SYNTAX );
                return FALSE;
            }
        }
        else
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_SYNTAX );
            return FALSE;
        }
        break;
        
    case KWI_MLEFT:
        pcps->cur_code->op = OP_SETITEM;
        
        cvalue->next = memalloc_zero( sizeof( CODE_VALUE ) );
        cvalue = cvalue->next;
        
        shift_clause_cps( pcps , pfollow_clause );
        
        if ( pfollow_clause->type == CT_VARIABLE )
        {
            cvalue->type = CVT_VARIABLE;
            cvalue->index = pfollow_clause->index;
        }
        else if ( pfollow_clause->type == CT_CONST )
        {
            cvalue->data = pfollow_clause->constvalue;
            pfollow_clause->constvalue = NULL;
            cvalue->type = CVT_CONST;
        }
        else
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_WORD );
            return FALSE;
        }
        
        shift_clause_cps( pcps , pfollow_clause );
        
        if ( ( pfollow_clause->type != CT_KEY )
            || ( pfollow_clause->key != KWI_EQUAL ) )
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_WORD );
            return FALSE;
        }
        
        cvalue->next = memalloc_zero( sizeof( CODE_VALUE ) );
        cvalue = cvalue->next;
        
        shift_clause_cps( pcps , pfollow_clause );
        
        if ( pfollow_clause->type == CT_VARIABLE )
        {
            cvalue->type = CVT_VARIABLE;
            cvalue->index = pfollow_clause->index;
        }
        else if ( pfollow_clause->type == CT_CONST )
        {
            cvalue->data = pfollow_clause->constvalue;
            pfollow_clause->constvalue = NULL;
            cvalue->type = CVT_CONST;
        }
        else if ( ( pfollow_clause->type == CT_KEY )
            && ( pfollow_clause->key == KWI_ASTERISK ) )
        {
            cvalue->data = NULL;
            cvalue->type = CVT_INIT;
        }
        else
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_WORD );
            return FALSE;
        }
        
        shift_clause_cps( pcps , pfollow_clause );
        
        if ( ( pfollow_clause->type != CT_KEY )
            || ( pfollow_clause->key != KWI_MRIGHT ) )
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_WORD );
            return FALSE;
        }
        break;
        
    case KWI_COMMA:
        cvalue->next = memalloc_zero( sizeof( CODE_VALUE ) );
        cvalue = cvalue->next;
        
        shift_clause_cps( pcps , pfollow_clause );
        
        if ( pfollow_clause->type == CT_VARIABLE )
        {
            cvalue->type = CVT_VARIABLE;
            cvalue->index = pfollow_clause->index;
        }
        else
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_WORD );
            return FALSE;
        }
        
        shift_clause_cps( pcps , pfollow_clause );
        
        if ( ( pfollow_clause->type == CT_KEY )
            && ( pfollow_clause->key == KWI_COMMA ) )
        {
            cvalue->next = memalloc_zero( sizeof( CODE_VALUE ) );
            cvalue = cvalue->next;
            
            shift_clause_cps( pcps , pfollow_clause );
            
            if ( pfollow_clause->type == CT_VARIABLE )
            {
                cvalue->type = CVT_VARIABLE;
                cvalue->index = pfollow_clause->index;
            }
            else if ( pfollow_clause->type == CT_CONST )
            {
                cvalue->type = CVT_CONST;
                cvalue->data = pfollow_clause->constvalue;
                pfollow_clause->constvalue = NULL;
            }
            else
            {
                *pcps->errorno = __LINE__;
                *pcps->errpos = *pcps->cur_pos;
                strcpy( pcps->errmsg , ERROR_WORD );
                return FALSE;
            }
            
            shift_clause_cps( pcps , pfollow_clause );
        }
        
        if ( ( pfollow_clause->type != CT_KEY )
            || ( pfollow_clause->key != KWI_WHILE ) )
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_WORD );
            return FALSE;
        }
        
        kwi = KWI_WHILE;
        
    case KWI_WHILE:
        pcps->cur_code->op = OP_FOREACH;
        cvalue->next = memalloc_zero( sizeof( CODE_VALUE ) );
        cvalue = cvalue->next;
        
        shift_clause_cps( pcps , pfollow_clause );
        
        if ( pfollow_clause->type == CT_VARIABLE )
        {
            cvalue->type = CVT_VARIABLE;
            cvalue->index = pfollow_clause->index;
        }
        else if ( pfollow_clause->type == CT_CONST )
        {
            cvalue->data = pfollow_clause->constvalue;
            pfollow_clause->constvalue = NULL;
            cvalue->type = CVT_CONST;
        }
        else
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_WORD );
            return FALSE;
        }
        
        shift_clause_cps( pcps , pfollow_clause );
        
        if ( ( pfollow_clause->type != CT_KEY )
            || ( pfollow_clause->key != KWI_BLEFT ) )
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_WORD );
            return FALSE;
        }
        
        cvalue->next = memalloc_zero( sizeof( CODE_VALUE ) );
        cvalue = cvalue->next;
        cvalue->type = CVT_CODE;
        cvalue->data = parse_code_buffer( pcps );
        
        if ( *pcps->errmsg != 0 )
        {
            return FALSE;
        }
        
        if ( cvalue->data == NULL )
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_SYNTAX );
            return FALSE;
        }
        
        break;
    
    default:
        *pcps->errorno = __LINE__;
        *pcps->errpos = *pcps->cur_pos;
        strcpy( pcps->errmsg , ERROR_WORD );
        return FALSE;
    }
    
    if ( kwi != KWI_WHILE && kwi != KWI_NODEBEGIN )
    {
        shift_clause_cps( pcps , pfollow_clause );
        
        if ( ( pfollow_clause->type != CT_KEY )
            || ( pfollow_clause->key != KWI_SEMICOLON ) )
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = pfollow_clause->bufpos;
            strcpy( pcps->errmsg , ERROR_WORD );
            return FALSE;
        }
    }
    
    return TRUE;
}

static BOOL parse_statement(
    CODE_PARSE_STATUS*  pcps            ,
    CLAUSE*             pfirst_clause   ,
    CLAUSE*             pfollow_clause
) {
    int             pcalltype;
    CODE*           prev_code;
    CODE_VALUE*     cvalue;
    
    switch ( pfirst_clause->key )
    {
    case KWI_PLEFT:
        prev_code = NULL;
        pcalltype = 0;
        
        do
        {
            pcps->cur_code->op = OP_CALL;
            pcps->cur_code->value = memalloc_zero( sizeof( CODE_VALUE ) );
            pcps->cur_code->value->type = CVT_INIT;
            
            pcalltype = parse_call( pcps , prev_code );
            
            if ( pcalltype == 0 )
            {
                *pcps->errorno = __LINE__;
                *pcps->errpos = *pcps->cur_pos;
                strcpy( pcps->errmsg , ERROR_SYNTAX );
                return FALSE;
            }
            else if ( pcalltype == 2 )
            {
                prev_code = pcps->cur_code;
                code_value_array( pcps->cur_code );
                pcps->cur_code->next = memalloc_zero( sizeof( CODE ) );
                pcps->cur_code = pcps->cur_code->next;
                pcps->cur_code->op = OP_NOOP;
                pcps->cur_code->val_array_count = -1;
            }
        }
        while ( pcalltype == 2 );
        
        break;
        
    case KWI_ALEFT:
        pcps->cur_code->op = OP_RETURN;
        cvalue = pcps->cur_code->value;
        
        while ( TRUE )
        {
            shift_clause_cps( pcps , pfollow_clause );
            
            if ( pfollow_clause->type == CT_KEY )
            {
                if ( pfollow_clause->key == KWI_ARIGHT )
                {
                    break;
                }
                else if ( pfollow_clause->key == KWI_COMMA )
                {
                    continue;
                }
            }
            
            if ( cvalue == NULL )
            {
                cvalue = memalloc_zero( sizeof( CODE_VALUE ) );
                pcps->cur_code->value = cvalue;
            }
            else
            {
                cvalue->next = memalloc_zero( sizeof( CODE_VALUE ) );
                cvalue = cvalue->next;
            }
            
            if ( pfollow_clause->type == CT_VARIABLE )
            {
                cvalue->type = CVT_VARIABLE;
                cvalue->index = pfollow_clause->index;
            }
            else if ( pfollow_clause->type == CT_CONST )
            {
                cvalue->data = pfollow_clause->constvalue;
                pfollow_clause->constvalue = NULL;
                cvalue->type = CVT_CONST;
            }
            else
            {
                *pcps->errorno = __LINE__;
                *pcps->errpos = pfollow_clause->bufpos;
                strcpy( pcps->errmsg , ERROR_WORD );
                return FALSE;
            }
        }
        break;
        
    case KWI_IF:
        pcps->cur_code->op = OP_IF;
        
        cvalue = memalloc_zero( sizeof( CODE_VALUE ) );
        pcps->cur_code->value = cvalue;
        
        shift_clause_cps( pcps , pfollow_clause );
        
        if ( pfollow_clause->type == CT_VARIABLE )
        {
            cvalue->type = CVT_VARIABLE;
            cvalue->index = pfollow_clause->index;
        }
        else if ( pfollow_clause->type == CT_CONST )
        {
            cvalue->data = pfollow_clause->constvalue;
            pfollow_clause->constvalue = NULL;
            cvalue->type = CVT_CONST;
        }
        else
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = pfollow_clause->bufpos;
            strcpy( pcps->errmsg , ERROR_WORD );
            return FALSE;
        }
        
        shift_clause_cps( pcps , pfollow_clause );
        
        if ( ( pfollow_clause->type != CT_KEY )
            || ( pfollow_clause->key != KWI_BLEFT ) )
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = pfollow_clause->bufpos;
            strcpy( pcps->errmsg , ERROR_WORD );
            return FALSE;
        }
        
        cvalue->next = memalloc_zero( sizeof( CODE_VALUE ) );
        cvalue = cvalue->next;
        cvalue->type = CVT_CODE;
        cvalue->data = parse_code_buffer( pcps );
        
        if ( *pcps->errmsg != 0 )
        {
            return FALSE;
        }
        
        if ( cvalue->data == NULL )
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_SYNTAX );
            return FALSE;
        }
        
        int b_if_loop = 1;
        
        while ( b_if_loop )
        {
            peek_clause(
                pcps->param         ,
                pcps->src_buf       ,
                pcps->src_fsize     ,
                pcps->cur_pos       ,
                pfollow_clause
            );
            
            if ( ( pfollow_clause->type != CT_KEY )
                || ( pfollow_clause->key != KWI_ELSE ) )
            {
                break;
            }
            
            cvalue->next = memalloc_zero( sizeof( CODE_VALUE ) );
            cvalue = cvalue->next;
            
            shift_clause_cps( pcps , pfollow_clause ); /*'~'*/
            
            shift_clause_cps( pcps , pfollow_clause );
            
            if ( ( pfollow_clause->type == CT_VARIABLE )
                || ( pfollow_clause->type == CT_CONST ) )
            {
                if ( pfollow_clause->type == CT_VARIABLE )
                {
                    cvalue->type = CVT_VARIABLE;
                    cvalue->index = pfollow_clause->index;
                }
                else if ( pfollow_clause->type == CT_CONST )
                {
                    cvalue->data = pfollow_clause->constvalue;
                    pfollow_clause->constvalue = NULL;
                    cvalue->type = CVT_CONST;
                }
                
                cvalue->next = memalloc_zero( sizeof( CODE_VALUE ) );
                cvalue = cvalue->next;
                
                shift_clause_cps( pcps , pfollow_clause );
            }
            else
            {
                b_if_loop = 0;
            }
            
            if ( ( pfollow_clause->type != CT_KEY )
                || ( pfollow_clause->key != KWI_BLEFT ) )
            {
                *pcps->errorno = __LINE__;
                *pcps->errpos = pfollow_clause->bufpos;
                strcpy( pcps->errmsg , ERROR_WORD );
                return FALSE;
            }
            
            cvalue->type = CVT_CODE;
            cvalue->data = parse_code_buffer( pcps );
            
            if ( *pcps->errmsg != 0 )
            {
                return FALSE;
            }
            
            if ( cvalue->data == NULL )
            {
                *pcps->errorno = __LINE__;
                *pcps->errpos = *pcps->cur_pos;
                strcpy( pcps->errmsg , ERROR_SYNTAX );
                return FALSE;
            }
        }
        
        break;
        
    case KWI_WHILE:
        pcps->cur_code->op = OP_WHILE;
        
        cvalue = memalloc_zero( sizeof( CODE_VALUE ) );
        pcps->cur_code->value = cvalue;
        
        shift_clause_cps( pcps , pfollow_clause );
        
        if ( pfollow_clause->type == CT_VARIABLE )
        {
            cvalue->type = CVT_VARIABLE;
            cvalue->index = pfollow_clause->index;
        }
        else if ( pfollow_clause->type == CT_CONST )
        {
            cvalue->data = pfollow_clause->constvalue;
            pfollow_clause->constvalue = NULL;
            cvalue->type = CVT_CONST;
        }
        else
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = pfollow_clause->bufpos;
            strcpy( pcps->errmsg , ERROR_WORD );
            return FALSE;
        }
        
        shift_clause_cps( pcps , pfollow_clause );
        
        if ( ( pfollow_clause->type != CT_KEY )
            || ( pfollow_clause->key != KWI_BLEFT ) )
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = pfollow_clause->bufpos;
            strcpy( pcps->errmsg , ERROR_WORD );
            return FALSE;
        }
        
        cvalue->next = memalloc_zero( sizeof( CODE_VALUE ) );
        cvalue = cvalue->next;
        cvalue->type = CVT_CODE;
        cvalue->data = parse_code_buffer( pcps );
        
        if ( *pcps->errmsg != 0 )
        {
            return FALSE;
        }
        
        if ( cvalue->data == NULL )
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_SYNTAX );
            return FALSE;
        }
        
        break;
        
    case KWI_BRIGHT:
        pcps->cur_code->op = OP_END;
        return TRUE;
        
    case KWI_SEMICOLON:
        break;
        
    default:
        *pcps->errorno = __LINE__;
        *pcps->errpos = pfirst_clause->bufpos;
        strcpy( pcps->errmsg , ERROR_SYNTAX );
        return FALSE;
    }
    
    return TRUE;
}

BOOL parse_codes(
    CODE_PARSE_STATUS*  pcps            ,
    CLAUSE*             pfirst_clause   ,
    CLAUSE*             pfollow_clause
) {
    CODE_VALUE*         cvalue;
    int                 kwi;
    
    do
    {
        *pcps->errstartpos = *pcps->cur_pos;
        
        if ( pcps->cur_code->op != OP_NOOP )
        {
            code_value_array( pcps->cur_code );
            pcps->cur_code->next = memalloc_zero( sizeof( CODE ) );
            pcps->cur_code = pcps->cur_code->next;
            pcps->cur_code->op = OP_NOOP;
            pcps->cur_code->val_array_count = -1;
        }
        
        shift_clause_cps( pcps , pfirst_clause );
        
        if ( pfirst_clause->type == CT_ENDFILE )
        {
            break;
        }
        else if ( pfirst_clause->type == CT_ERRORWORD )
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = *pcps->cur_pos;
            strcpy( pcps->errmsg , ERROR_WORD );
            return FALSE;
        }
        
        if ( pfirst_clause->type == CT_VARIABLE )
        {
            shift_clause_cps( pcps , pfollow_clause );
            
            if ( pfollow_clause->type != CT_KEY )
            {
                *pcps->errorno = __LINE__;
                *pcps->errpos = *pcps->cur_pos;
                strcpy( pcps->errmsg , ERROR_WORD );
                return FALSE;
            }
            
            if ( ! parse_expression( pcps , pfirst_clause , pfollow_clause ) )
            {
                return FALSE;
            }
        }
        /*--------------------*/
        /* first KEY */
        else if ( pfirst_clause->type == CT_KEY )
        {
            if ( ! parse_statement( pcps , pfirst_clause , pfollow_clause ) )
            {
                return FALSE;
            }
            
            if ( pcps->cur_code->op == OP_END )
            {
                return TRUE;
            }
        }
        /*---------------------*/
        /*error*/
        else
        {
            *pcps->errorno = __LINE__;
            *pcps->errpos = pfirst_clause->bufpos;
            strcpy( pcps->errmsg , ERROR_SYNTAX );
            return FALSE;
        }
    }
    while ( TRUE );
    
    return TRUE;
}

CODE* parse_code_buffer( CODE_PARSE_STATUS* pcps )
{
    CODE*               prevcode;
    CODE*               ret;
    CLAUSE              first_clause;
    CLAUSE              follow_clause;
    
    prevcode = pcps->cur_code;
    
    ret = memalloc_zero( sizeof( CODE ) );
    pcps->cur_code = ret;
    pcps->cur_code->op = OP_NOOP;
    pcps->cur_code->val_array_count = -1;
    
    memset( &first_clause , 0 , sizeof( CLAUSE ) );
    memset( &follow_clause , 0 , sizeof( CLAUSE ) );
    
    parse_codes( pcps , &first_clause , &follow_clause );
    
    pcps->cur_code = prevcode;
    
    free_clause( &first_clause );
    free_clause( &follow_clause );
    
    return ret;
}

NODE* parse_source_buffer(
    char*   src_buf     ,
    int     src_fsize   ,
    int*    errorno     ,
    int*    errstartpos ,
    int*    errpos      ,
    char*   errmsg
) {
    NODE*       ret;
    int         pos;
    word        w;
    int         kwi;
    int         start_pos;
    int         end_pos;
    int         len;
    NODE*       cur_node;
    PARAM_ITEM* tempvalue;
    int         paramstep;
    int         nps;
    
    pos                 = 0;
    *errstartpos        = pos;
    cur_node            = NULL;
    ret                 = NULL;
    tempvalue           = NULL;
    nps                 = NPS_BEGIN;
    
    /* skip BOM */
    /* UTF-8 */
    if ( ( src_fsize >= 3 )
        && ( ( ( UINT8 )src_buf[0] ) == 0xEF )
        && ( ( ( UINT8 )src_buf[1] ) == 0xBB )
        && ( ( ( UINT8 )src_buf[2] ) == 0xBF ) )
    {
        pos = 3;
    }
    
    while ( pos < src_fsize )
    {
        switch ( nps )
        {
        case NPS_BEGIN:
            *errstartpos = pos;
            shift_space_comment( src_buf , src_fsize , &pos );
            
            if ( pos >= src_fsize )
            {
                nps = NPS_END;
                break;
            }
            
            kwi = shift_keyword( src_buf , src_fsize , &pos , FALSE );
            
            if ( kwi != KWI_NODEDEF )
            {
                *errorno = __LINE__;
                *errpos = *errstartpos;
                strcpy( errmsg , ERROR_NODEDEF );
                return ret;
            }
            
            if ( cur_node == NULL )
            {
                cur_node = memalloc_zero( sizeof( NODE ) );
                ret = cur_node;
            }
            else
            {
                cur_node->next = memalloc_zero( sizeof( NODE ) );
                cur_node = cur_node->next;
            }
            
            cur_node->type = NT_SCNODE;
            
            nps = NPS_NAME;
            break;
            
        case NPS_NAME:
            start_pos = pos;
            len = shift_namestr( src_buf , src_fsize , &pos );
            
            if ( len == 0 )
            {
                *errorno = __LINE__;
                *errpos = start_pos;
                strcpy( errmsg , ERROR_NODENAME );
                return ret;
            }
            
            wstrncpy(
                cur_node->name                      ,
                src_buf + start_pos                 ,
                MIN( MAX_WORD_NAME_LEN - 1 , len )
            );
            
            shift_space_comment( src_buf , src_fsize , &pos );
            
            if ( pos >= src_fsize )
            {
                *errorno = __LINE__;
                *errpos = pos;
                strcpy( errmsg , ERROR_NODEBODY );
                return ret;
            }
            
            kwi = shift_keyword( src_buf , src_fsize , &pos , TRUE );
            
            if ( kwi != KWI_PLEFT )
            {
                *errorno = __LINE__;
                *errpos = pos;
                strcpy( errmsg , ERROR_NODEPARAM );
                return ret;
            }
            
            paramstep = 1;
            
            nps = NPS_PARAM;
            cur_node->param = memalloc_zero( sizeof( NODE_PARAM ) );
            cur_node->param->hmap = dmap_init( 10 );
            cur_node->param->count = 0;
            cur_node->param->count1 = 0;
            cur_node->param->count2 = 0;
            tempvalue = NULL;
            break;
            
        case NPS_PARAM:
            shift_space_comment( src_buf , src_fsize , &pos );
            kwi = shift_keyword( src_buf , src_fsize , &pos , TRUE );
            
            if ( kwi == KWI_PRIGHT )
            {
                nps = NPS_BODY;
                break;
            }
            
            start_pos = pos;
            len = shift_namestr( src_buf , src_fsize , &pos );
            
            if ( len == 0 )
            {
                *errorno = __LINE__;
                *errpos = pos;
                strcpy( errmsg , ERROR_NODEPARAM );
                value_link_to_array( cur_node->param );
                return ret;
            }
            
            end_pos = pos;
            
            shift_space_comment( src_buf , src_fsize , &pos );
            kwi = shift_keyword( src_buf , src_fsize , &pos , TRUE );
            
            if ( kwi == KWI_PRIGHT )
            {
                nps = NPS_BODY;
            }
            else if ( kwi != KWI_COMMA )
            {
                *errorno = __LINE__;
                *errpos = pos;
                strcpy( errmsg , ERROR_NODEPARAM );
                value_link_to_array( cur_node->param );
                return ret;
            }
            
            if ( tempvalue == NULL )
            {
                tempvalue = memalloc_zero( sizeof( PARAM_ITEM ) );
                cur_node->param->list = tempvalue;
            }
            else
            {
                tempvalue->value = memalloc_zero( sizeof( PARAM_ITEM ) );
                tempvalue = ( PARAM_ITEM* )tempvalue->value;
            }
            
            strncpy(
                tempvalue->name                 ,
                src_buf + start_pos             ,
                MIN( MAX_NAME_LEN - 1 , len )
            );
            
            if ( dmap_query( cur_node->param->hmap , tempvalue->name ) != NULL )
            {
                *errorno = __LINE__;
                *errpos = pos;
                strcpy( errmsg , ERROR_NODEPARAM );
                value_link_to_array( cur_node->param );
                return ret;
            }
            
            cur_node->param->count++;
            dmap_insert(
                cur_node->param->hmap ,
                tempvalue->name ,
                ( void* )( intptr_t )cur_node->param->count
            );
            
            if ( paramstep == 1 )
            {
                cur_node->param->count1++;
            }
            else if ( paramstep == 2 )
            {
                cur_node->param->count2++;
            }
            
            break;
            
        case NPS_BODY:
            shift_space_comment( src_buf , src_fsize , &pos );
            kwi = shift_keyword( src_buf , src_fsize , &pos , TRUE );
            
            if ( kwi == KWI_PLEFT )
            {
                paramstep++;
                nps = NPS_PARAM;
                break;
            }
            
            if ( cur_node->param != NULL && cur_node->param->count > 0)
            {
                value_link_to_array( cur_node->param );
            }
            
            if ( kwi != KWI_BLEFT )
            {
                *errorno = __LINE__;
                *errpos = pos;
                strcpy( errmsg , ERROR_NODEBODY );
                return ret;
            }
            
            *errorno = 0;
            *errpos = 0;
            *errmsg = 0;
            
            CODE_PARSE_STATUS cps = { 0 };
            cps.param = cur_node->param;
            cps.src_buf = src_buf;
            cps.src_fsize = src_fsize;
            cps.cur_pos = &pos;
            cps.errorno = errorno;
            cps.errstartpos = errstartpos;
            cps.errpos = errpos;
            cps.errmsg = errmsg;
            
            CODE* code = parse_code_buffer( &cps );
            cur_node->body = memalloc_zero( sizeof( NODE_BODY ) );
            cur_node->body->code = code;
            
            if ( *errmsg != 0 )
            {
                return ret;
            }
            
            nps = NPS_END;
            
            break;
            
        case NPS_END:
            nps = NPS_BEGIN;
            break;
        }
    }
    
    if ( ( nps != NPS_BEGIN ) && ( nps != NPS_END ) )
    {
        *errorno = __LINE__;
        *errpos = pos;
        strcpy( errmsg , ERROR_KEYWORD );
        return ret;
    }
    
    return ret;
}

NODE* parse_source_file( char* filename )
{
    FILE_DESC*  src_fd;
    UINT        src_fsize;
    char*       src_buf;
    NODE*       ret;
    int         errorno;
    int         errstartpos;
    int         errpos;
    char        errmsg[1024];
    
    errorno     = 0;
    errpos      = 0;
    errmsg[0]   = 0;
    ret         = NULL;
    src_fd      = fileopen( filename , "rb" );
    
    if ( src_fd == NULL )
    {
        return ret;
    }
    
    src_fsize   = filesize( src_fd );
    src_buf     = memalloc_zero( src_fsize + 1 );
    
    if ( src_buf == NULL )
    {
        fileclose( src_fd );
        log_error( "memalloc( %d ) error" , src_fsize + 1 );
        return ret;
    }
    
    if ( fileread( src_fd , src_buf , src_fsize ) != src_fsize )
    {
        fileclose( src_fd );
        memfree( src_buf );
        log_error( "fileread(%s) error" , filename );
        return ret;
    }
    
    fileclose( src_fd );
    
    ret = parse_source_buffer(
        src_buf         ,
        src_fsize       ,
        &errorno        ,
        &errstartpos    ,
        &errpos         ,
        errmsg
    );
    
    if ( errmsg[0] != 0 )
    {
        print_error(
            filename        ,
            src_buf         ,
            src_fsize       ,
            errorno         ,
            errstartpos     ,
            errpos          ,
            errmsg
        );
        
        if ( ret )
        {
            free_node( ret );
            ret = NULL;
        }
    }
    
    memfree( src_buf );
    
    return ret;
}
