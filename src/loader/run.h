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

#ifndef __LOADER_RUN_H_INCLUDED__
#define __LOADER_RUN_H_INCLUDED__

#include "memalloc.h"
#include "map.h"
#include "map_int.h"
#include "list.h"
#include "slang.h"

enum MODULE_TYPE
{
    MT_SCMOD = 0, /*default*/
    MT_EXTMOD
};

typedef struct _MODULE          MODULE;

struct _MODULE
{
    NODE*       node;
    HDMAP       nodemap; /* NODE */
    char        name[MAX_NAME_LEN];
    int         modtype;
    void*       handle;
};

typedef struct _CALLCONTEXT     CALLCONTEXT;

struct _CALLCONTEXT
{
    MODULE*     module;
    NODE*       node;
    NODE_PARAM* param;
    
    PARAM_ITEM* retvalue;
    int         retvalue_count;
};

typedef struct _RUNTIME         RUNTIME;

struct _RUNTIME
{
    char        basepath[MAX_NAME_LEN];
    HDMAP       module;     /*MODULE*/
    HDMAP       sysnode;    /*SYSLIBNODE*/
    char*       src_ext_filename;
    char*       bin_ext_filename;
    
    /*memory*/
    int         total_ref;
    HDLIST      hdelvalue;
    
    /* call context*/
    CALLCONTEXT current;
    CALLCONTEXT caller;
    
    int         errorno;
    
    /*root table*/
    RT_VALUE*   root;
    
    /*data dump*/
    FILE_DESC*  fData;
    
    HMAP_INT    loadvar; /*pos_func => void**/
};

RUNTIME* new_runtime( char* basepath );

int release_runtime( RUNTIME* runtime );

MODULE* load_module( RUNTIME* runtime , char* name );

MODULE* load_module_from_node( RUNTIME* runtime , char* name , NODE* node );

int call_node(
    RUNTIME*    runtime ,
    char*       mod_node_name ,
    PARAM_ITEM* value ,
    int         value_count ,
    PARAM_ITEM* retvalue ,
    int         retvalue_count ,
    int*        errorno
);

void free_node_data( RUNTIME* runtime , NODE* node );

void free_param_list_value( RUNTIME* runtime , PARAM_ITEM* list , int count );

void free_param_list( PARAM_ITEM* list , int count );

#endif
