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

#include "text_conv.h"
#include "memalloc.h"

#include <string.h>

#ifdef CSBASE
#else
    #ifndef WINDOWS
        #include <iconv.h>
    #endif
#endif

char* convert_code(
    int         src_charset ,
    char*       src         ,
    int         dst_charset
) {
    size_t  nlen;
    size_t  mlen;
    char*   ret;
    char*   ret_str;
    char*   tmp_str;
    
#ifndef WINDOWS
    iconv_t hiconv;
#endif
    
    nlen = strlen( src );
    mlen = nlen * 2;
    
    ret_str = memalloc_zero( ( nlen + 2 ) * 2 );
    
    ret = ret_str;
    
    if ( src_charset == dst_charset )
    {
        strcpy( ret , src );
        return ret;
    }
    
#ifdef CSBASE
    //TODO:
    strcpy( ret , src );
    return ret;
#endif
    
    if ( src_charset == CL_UTF8 )
    {
#ifdef WINDOWS
        tmp_str = memalloc_zero( ( nlen + 1 ) * sizeof( WCHAR ) );
        mlen = MultiByteToWideChar(
            CP_UTF8 ,
            0 ,
            src ,
            nlen ,
            tmp_str ,
            sizeof( WCHAR ) * nlen
        );
        mlen = WideCharToMultiByte(
            CP_ACP ,
            0 ,
            tmp_str ,
            mlen ,
            ret_str ,
            sizeof( WCHAR ) * nlen ,
            NULL ,
            FALSE
        );
        ret_str[mlen] = 0;
        memfree( tmp_str );
#else
        hiconv = iconv_open( "gb2312//TRANSLIT", "utf-8" );
        
        if ( hiconv == 0 )
        {
            strcpy( ret , src );
            return ret;
        }
        
        tmp_str = src;
        size_t nlen2 = nlen;
        size_t mlen2 = mlen;
        
        if ( iconv( hiconv , &tmp_str , &nlen2 , &ret_str , &mlen2 ) == -1 )
        {
            strcpy( ret , src );
            return ret;
        }
        
        iconv_close( hiconv );
#endif
    }
    else
    {
#ifdef WINDOWS
        tmp_str = memalloc_zero( ( nlen + 1 ) * sizeof( WCHAR ) );
        mlen = MultiByteToWideChar(
            CP_ACP ,
            0 ,
            src ,
            nlen ,
            tmp_str ,
            sizeof( WCHAR ) * nlen
        );
        mlen = WideCharToMultiByte(
            CP_UTF8 ,
            0 ,
            tmp_str ,
            mlen ,
            ret_str ,
            sizeof( WCHAR ) * nlen ,
            NULL ,
            FALSE
        );
        ret_str[mlen] = 0;
        memfree( tmp_str );
#else
        hiconv = iconv_open( "utf-8" , "gb2312" );
        
        if ( hiconv == 0 )
        {
            strcpy( ret , src );
            return ret;
        }
        
        if ( iconv( hiconv , &src , &nlen , &ret_str , &mlen ) == -1 )
        {
            strcpy( ret , src );
            return ret;
        }
        
        iconv_close( hiconv );
#endif
    }
    
    return ret;
}
