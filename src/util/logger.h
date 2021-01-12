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

#ifndef __UTIL_LOGGER_H_INCLUDED__
#define __UTIL_LOGGER_H_INCLUDED__


#ifdef __cplusplus
extern "C" {
#endif

extern const char* LOG_ERROR;
extern const char* LOG_INFO;

void _logger(
    const char*     srcfile     ,
    int             nline       ,
    const char*     prefix      ,
    const char*     format_str  ,
    ...
);

#define logger( ... ) \
    _logger( __FILE__ , __LINE__ , LOG_INFO , __VA_ARGS__ );

#define log_info( ... ) \
    _logger( __FILE__ , __LINE__ , LOG_INFO , __VA_ARGS__ );

#define log_error( ... ) \
    _logger( __FILE__ , __LINE__ , LOG_ERROR , __VA_ARGS__ );

#ifdef __cplusplus
}
#endif

#endif
