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

#ifndef __LOADER_EVAL_H_INCLUDED__
#define __LOADER_EVAL_H_INCLUDED__

#include "slang.h"
#include "file.h"

#ifndef MAX
    #define MAX(x,y) ((x)>(y)?(x):(y))
#endif

#ifndef MIN
    #define MIN(x,y) ((x)<(y)?(x):(y))
#endif

enum EVAL_VALUE_TYPE
{
    EVT_NUMBER ,
    EVT_VARIABLE ,
    EVT_SUBEVAL
};

typedef struct _EVAL_VALUE
{
    SLINT       type;
    char*       data;
}
EVAL_VALUE;

typedef struct _EVAL_CODE   EVAL_CODE;
struct _EVAL_CODE
{
    SLINT       op;
    EVAL_VALUE  value;
    EVAL_CODE*  next;
};

char* strformat( NODE_PARAM* param , char* value );

char* evaluate( NODE_PARAM* param , char* str );

EVAL_VALUE* build_evaluate(
    NODE_PARAM* param   ,
    char*       str     ,
    SLINT       str_len ,
    SLINT*      pos
);

bchar* run_evaluate(
    NODE_PARAM* param ,
    EVAL_VALUE* eval
);

void free_evaluate( EVAL_VALUE* eval );

SLINT dump_evaluate( EVAL_VALUE* eval , FILE_DESC* pf );

EVAL_VALUE* load_evaluate( FILE_DESC* pf , SLINT* haverror );

#endif
