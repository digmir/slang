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

#ifndef __UTIL_FILE_H_INCLUDED__
#define __UTIL_FILE_H_INCLUDED__

#define MAX_FILE_NAME       128

typedef struct _FILE_DESC   FILE_DESC;

#define FILETYPE_FILE       1
#define FILETYPE_MEMSTREAM  2

#define FILESEEK_SET        0
#define FILESEEK_CUR        1
#define FILESEEK_END        2

FILE_DESC* fileopen( const char* filename , const char* param );

void fileclose( FILE_DESC* fd_ptr );

long filesize( FILE_DESC* fd_ptr );

int fileread( FILE_DESC* fd_ptr , void* buf , int size );

int filewrite( FILE_DESC* fd_ptr , void* buf , int size );

long fileseek( FILE_DESC* fd_ptr , long offset , int seek_type );

long filetell( FILE_DESC* fd_ptr );

#endif
