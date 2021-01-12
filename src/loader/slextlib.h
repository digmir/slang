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

#ifndef __LOADER_SLEXTLIB_H_INCLUDED__
#define __LOADER_SLEXTLIB_H_INCLUDED__

#include "slang.h"
#include "run.h"

#ifdef WINDOWS
#ifdef BUILD_DLL
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT __declspec(dllimport)
#endif
#else
    #define DLL_EXPORT
#endif


#ifdef __cplusplus
extern "C"
{
#endif

#define SLRUNTIME           RUNTIME*
#define SLMODULE            MODULE*
#define SLTABLE             RT_VALUE*
#define SLVALUE             RT_VALUE*

#define SLVALUE_TYPE_NUL    0
#define SLVALUE_TYPE_STR    1
#define SLVALUE_TYPE_TBL    2

#define SLCHARSET_ASCII     0
#define SLCHARSET_UTF8      1

/* return continue? */
typedef SLINT   (* TABLE_CALLBACK)( char* key , SLVALUE data , void* param );


typedef SLINT   (* SLEXT_GET_COUNT)( SLRUNTIME runtime );

typedef SLINT   (* SLEXT_GET_INT)( SLRUNTIME runtime , SLINT i );

typedef char*   (* SLEXT_GET_PTR)(
    SLRUNTIME   runtime ,
    SLINT       i       ,
    SLINT*      outsize
);

typedef char*   (* SLEXT_GET_STR)( SLRUNTIME runtime , SLINT i );

typedef SLTABLE (* SLEXT_GET_TBL)( SLRUNTIME runtime , SLINT i );

typedef SLINT   (* SLEXT_SET_INT)( SLRUNTIME runtime , SLINT i , SLINT value );

typedef SLINT   (* SLEXT_SET_PTR)(
    SLRUNTIME   runtime ,
    SLINT       i       ,
    char*       str     ,
    SLINT       size    ,
    SLINT       bcopy
);

typedef SLINT   (* SLEXT_SET_STR)( SLRUNTIME runtime , SLINT i , char* str );

typedef SLINT   (* SLEXT_SET_TBL)(
    SLRUNTIME   runtime ,
    SLINT       i       ,
    SLTABLE     pvalue
);

typedef SLRUNTIME   (* SLEXT_TMPRUNTIME_NEW)(
    SLRUNTIME   runtime     ,
    SLINT       retcount    ,
    SLINT       paramcount
);

typedef SLINT   (* SLEXT_TMPRUNTIME_DEL)(
    SLRUNTIME   runtime     ,
    SLRUNTIME   tmpruntime
);

typedef SLINT   (* SLEXT_CALL)(
    SLRUNTIME   runtime     ,
    char*       func_name   ,
    SLRUNTIME   paramrt     ,
    SLRUNTIME   retrt
);

typedef SLTABLE (* SLEXT_TBL_NEW)( SLRUNTIME runtime );

typedef SLINT   (* SLEXT_TBL_DEL)( SLRUNTIME runtime , SLTABLE table );

typedef SLINT   (* SLEXT_TBL_GET_INT)(
    SLRUNTIME   runtime ,
    SLTABLE     table   ,
    char*       key
);

typedef char*   (* SLEXT_TBL_GET_PTR)(
    SLRUNTIME   runtime ,
    SLTABLE     table   ,
    char*       key     ,
    SLINT*      outsize
);

typedef SLTABLE (* SLEXT_TBL_GET_TBL)(
    SLRUNTIME   runtime ,
    SLTABLE     table   ,
    char*       key
);

typedef SLINT   (* SLEXT_TBL_SET_INT)(
    SLRUNTIME   runtime ,
    SLTABLE     table   ,
    char*       key     ,
    SLINT       value
);

typedef SLINT   (* SLEXT_TBL_SET_PTR)(
    SLRUNTIME   runtime ,
    SLTABLE     table   ,
    char*       key     ,
    char*       str     ,
    SLINT       size    ,
    SLINT       bcopy
);

typedef SLINT   (* SLEXT_TBL_SET_TBL)(
    SLRUNTIME   runtime ,
    SLTABLE     table   ,
    char*       key     ,
    SLTABLE     pvalue
);

typedef SLINT   (* SLEXT_TBL_SET_VAL)(
    SLRUNTIME   runtime ,
    SLTABLE     table   ,
    char*       key     ,
    SLVALUE     value
);

typedef SLINT   (* SLEXT_TBL_FOREACH)(
    SLRUNTIME       runtime     ,
    SLTABLE         table       ,
    TABLE_CALLBACK  callback    ,
    void*           param
);

typedef SLINT   (* SLEXT_VAL_GET_TYPE)( SLVALUE svalue );

typedef SLINT   (* SLEXT_VAL_GET_INT)( SLVALUE svalue );

typedef char*   (* SLEXT_VAL_GET_PTR)( SLVALUE svalue , SLINT* outsize );

typedef SLTABLE (* SLEXT_VAL_GET_TBL)( SLVALUE svalue );

typedef SLINT   (* SLEXT_REG_FUNC)(
    SLMODULE    module      ,
    char*       func_name   ,
    EXTFUNC     func
);

typedef void    (* SLEXT_FREE_STR)( SLRUNTIME runtime , char* str );

typedef struct _EXTLIB_FUNC {
    SLEXT_GET_COUNT         slext_get_count;
    SLEXT_GET_INT           slext_get_int;
    SLEXT_GET_PTR           slext_get_ptr;
    SLEXT_GET_STR           slext_get_str;
    SLEXT_GET_TBL           slext_get_tbl;
    SLEXT_SET_INT           slext_set_int;
    SLEXT_SET_PTR           slext_set_ptr;
    SLEXT_SET_STR           slext_set_str;
    SLEXT_SET_TBL           slext_set_tbl;

    SLEXT_TMPRUNTIME_NEW    slext_tmpruntime_new;
    SLEXT_TMPRUNTIME_DEL    slext_tmpruntime_del;
    SLEXT_CALL              slext_call;

    SLEXT_TBL_NEW           slext_tbl_new;
    SLEXT_TBL_DEL           slext_tbl_del;
    SLEXT_TBL_GET_INT       slext_tbl_get_int;
    SLEXT_TBL_GET_PTR       slext_tbl_get_ptr;
    SLEXT_TBL_GET_TBL       slext_tbl_get_tbl;
    SLEXT_TBL_SET_INT       slext_tbl_set_int;
    SLEXT_TBL_SET_PTR       slext_tbl_set_ptr;
    SLEXT_TBL_SET_TBL       slext_tbl_set_tbl;
    SLEXT_TBL_SET_VAL       slext_tbl_set_val;
    SLEXT_TBL_FOREACH       slext_tbl_foreach;

	SLEXT_VAL_GET_TYPE		slext_val_get_type;
	SLEXT_VAL_GET_INT		slext_val_get_int;
	SLEXT_VAL_GET_PTR       slext_val_get_ptr;
	SLEXT_VAL_GET_TBL       slext_val_get_tbl;

    SLEXT_REG_FUNC          slext_reg_func;
    SLEXT_FREE_STR          slext_free_str;
}
EXTLIB_FUNC;

extern EXTLIB_FUNC g_extlib_func;

typedef int (* REG_EXTLIB)( SLMODULE module , EXTLIB_FUNC func );
typedef int (* UNREG_EXTLIB)( SLMODULE module );

int DLL_EXPORT reg_extlib( SLMODULE module , EXTLIB_FUNC func );
int DLL_EXPORT unreg_extlib( SLMODULE module );

SLINT slext_load_module( MODULE* module , char* file_path );
SLINT slext_unload_module( MODULE* module );

#ifdef __cplusplus
}
#endif

#endif
