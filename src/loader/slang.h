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

#ifndef __LOADER_SLANG_H_INCLUDED__
#define __LOADER_SLANG_H_INCLUDED__

#include "file.h"
#include "stypes.h"

#define MAX_WORD_NAME_LEN       256
#define MAX_NAME_LEN            MAX_WORD_NAME_LEN

#define SLINT                   int
#define INT_SIZE                sizeof( int )

#define SLINT64                 long long
#define INT64_SIZE              sizeof( long long )

#ifdef WINDOWS
    #define PS                  '\\'
#else
    #define PS                  '/'
#endif

#define SRC_EXT_FILENAME        ".sl"
#define CODE_EXT_FILENAME       ".sc"
#define DATA_DUMP_DATANAME      "sl.data"

#ifdef WINDOWS
    #define LIB_EXT_FILENAME    ".dll"
#else
    #define LIB_EXT_FILENAME    ".so"
#endif

#define NODE_ASYNC              0x01000000

typedef char                    bchar;
typedef UINT32                  word;

enum NODE_PARSE_STAGE
{
    NPS_BEGIN               = 1 ,
    NPS_NAME                = 2 ,
    NPS_PARAM               = 3 ,
    NPS_BODY                = 4 ,
    NPS_END                 = 5
};

enum OPERATE
{
    OP_NOOP                 = 0  ,
    OP_EQU                  = 1  ,
    OP_EVALUATE             = 2  ,
    OP_STRFORMAT            = 3  ,
    OP_IF                   = 4  ,
    OP_WHILE                = 5  ,
    OP_RETURN               = 6  ,
    OP_CALL                 = 7  ,
    OP_GETITEMCOUNT         = 8  ,
    OP_GETITEM              = 9  ,
    OP_SETITEM              = 10 ,
    OP_FOREACH              = 11 ,
    OP_END                  = 12
};

enum CLAUSE_TYPE
{
    CT_VARIABLE             = 1 ,
    CT_NODE                 = 2 ,
    CT_KEY                  = 3 ,
    CT_CONST                = 4 ,
    CT_ERRORWORD            = 5 ,
    CT_ENDFILE              = 6
};

typedef struct _CLAUSE      CLAUSE;

struct _CLAUSE
{
    int                     type;   /* CLAUSE_TYPE */
    word                    key;
    int                     index;
    int                     bufpos;
    char                    name[ MAX_WORD_NAME_LEN ];
    char*                   constvalue;
};

enum CODE_VALUE_TYPE
{
    CVT_VARIABLE            = 1 ,
    CVT_CONST               = 2 ,
    CVT_CODE                = 3 ,
    CVT_EVAL                = 4 ,
    CVT_INIT                = 5
};

typedef struct _CODE_VALUE  CODE_VALUE;

struct _CODE_VALUE
{
    int                     type;   /* CODE_VALUE_TYPE */
    int                     index;
    void*                   data;
    CODE_VALUE*             next;
};

typedef struct _CODE        CODE;

struct _CODE
{
    int                     op;     /* OPERATE */
    CODE_VALUE*             value;
    
    int                     val_array_count; /* -1: linked list */
                                             /* otherwise: array */
    CODE*                   next;
};

enum NODE_RET_CODE
{
    RET_ERROR               = 0 ,
    RET_OK                  = 1 ,
    RET_RETURN              = 2 ,
    RET_NO_NODE             = 3
};

enum RT_VALUE_TYPE
{
    RVT_NULL                = 0 ,
    RVT_STRING              = 1 ,
    RVT_TABLE               = 2 ,
    RVT_TBL_ITEM_UNLOAD     = 3
};

typedef struct _RT_VALUE    RT_VALUE;

struct _RT_VALUE
{
    int                     type;
    UINT                    size;
    void*                   data;
    UINT                    ref;
    UINT64                  fpos;
};

typedef struct _PARAM_ITEM  PARAM_ITEM;

struct _PARAM_ITEM
{
    char                    name[MAX_NAME_LEN];
    RT_VALUE*               value;
};

typedef struct _NODE_PARAM  NODE_PARAM;

struct _NODE_PARAM
{
    PARAM_ITEM*             list;
    UINT                    count;
    UINT                    count1;
    UINT                    count2;
    void*                   hmap;
};

typedef struct _NODE_BODY   NODE_BODY;

struct _NODE_BODY
{
    CODE*                   code;
};

enum NODE_TYPE
{
    NT_SCNODE               = 0 , /* default */
    NT_FISSIONNODE          = 1 ,
    NT_EXTNODE              = 2 
};

enum NODE_STATUS
{
    NS_INIT                 = 0 , /* default */
    NS_LOADDATA             = 1
};

typedef struct _RUNTIME     RUNTIME;

typedef int (* EXTFUNC )( RUNTIME* runtime );

typedef struct _NODE        NODE;

struct _NODE
{
    char                    name[MAX_WORD_NAME_LEN];
    NODE_PARAM*             param;
    NODE_BODY*              body;
    int                     status;
    int                     type;
    EXTFUNC                 extfunc;
    NODE*                   next;
};

void free_node( NODE* node );

int dump_node( NODE* node , char* dest_file );

NODE* parse_source_file( char* filename );

NODE* parse_source_buffer(
    char*   src_buf     ,
    int     src_fsize   ,
    int*    errorno     ,
    int*    errstartpos ,
    int*    errpos      ,
    char*   errmsg
);

NODE* load_node( char* bin_file );

#endif
