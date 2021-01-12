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

#ifndef __UTIL_TEXT_CONV_H_INCLUDED__
#define __UTIL_TEXT_CONV_H_INCLUDED__

enum CHARSET_LIST
{
    CL_ASCII ,
    CL_UTF8
};

char* convert_code(
    int         src_charset ,
    char*       src         ,
    int         dst_charset
);

#endif
