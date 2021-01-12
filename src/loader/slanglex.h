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

#ifndef __LOADER_SLANGLEX_H_INCLUDED__
#define __LOADER_SLANGLEX_H_INCLUDED__

BOOL is_space( word c );

BOOL is_number( word c );

BOOL is_namestart( word c );

BOOL is_name( word c );

word shift_word(
    char*   src_buf     ,
    int     src_fsize   ,
    int*    cur_pos
);

int shift_keyword(
    char*   src_buf     ,
    int     src_fsize   ,
    int*    cur_pos     ,
    BOOL    asc
);

void set_clause_info(
    char*   src_buf     ,
    int     start_pos   ,
    int     end_pos     ,
    CLAUSE* clause      ,
    int     type   ,
    word    key
);

void free_clause( CLAUSE* clause );

word shift_word_skip_space(
    char*   src_buf     ,
    int     src_fsize   ,
    int*    cur_pos
);

int shift_namestr( char* src_buf , int src_fsize , int* cur_pos );

#endif
