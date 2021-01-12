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

#include "file.h"

#include <stdlib.h>
#include <stdio.h>

struct _FILE_DESC
{
    int fd;
};

FILE_DESC* fileopen( const char* filename , const char* param )
{
    return ( FILE_DESC* )fopen( filename , param );
}

void fileclose( FILE_DESC* fd_ptr )
{
    fclose( ( FILE* )fd_ptr );
}

long filesize( FILE_DESC* fd_ptr )
{
    long    pos;
    long    size;
    
    pos = ftell( ( FILE* )fd_ptr );
    fseek( ( FILE* )fd_ptr , 0 , SEEK_END );
    size = ftell( ( FILE* )fd_ptr );
    fseek( ( FILE* )fd_ptr , pos , SEEK_SET );
    
    return size;
}

int fileread( FILE_DESC* fd_ptr , void* buf , int size )
{
    return fread( buf , 1 , size , ( FILE* )fd_ptr );
}

int filewrite( FILE_DESC* fd_ptr , void* buf , int size )
{
    return fwrite( buf , 1 , size , ( FILE* )fd_ptr );
}

long fileseek( FILE_DESC* fd_ptr , long offset , int seek_type )
{
    return fseek( ( FILE* )fd_ptr , offset , seek_type );
}

long filetell( FILE_DESC* fd_ptr )
{
    return ftell( ( FILE* )fd_ptr );
}
