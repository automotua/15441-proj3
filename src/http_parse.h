/*
 * http_parse.h
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *          Junqiang Li <junqiangl@andrew.cmu.edu>
 *
 * Date: 12-13-2015
 *
 * Description: header file of http_parse.c
 *
 */
#ifndef _HTTP_PARSE_H_
#define _HTTP_PARSE_H_

#include "px_parse.h"

void parse_browser_request(px_config_t * config, browser_conn_t * b_conn);

void clean_bconn_after_parse(browser_conn_t * b_conn);

void parse_server_response(px_config_t * config, server_conn_t * s_conn);

void clean_sconn_after_parse(server_conn_t * s_conn);

char* get_request_header_value(char* line, char* key);

#endif /* _HTTP_PARSE_H_ */