/*
 * helper.h
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *          Junqiang Li <junqiangl@andrew.cmu.edu>
 *
 */
#ifndef _HELPER_H_
#define _HELPER_H_

#include "px_parse.h"

char* generate_request_to_server(browser_conn_t * b_conn, char* request_file, 
									char* serverhost);

int send_data_to_socket(int sock, char* buf, int size);

int init_server_connection(px_config_t * config, px_conn_t * px_conn);

void logmessage(char* msg1, char* msg2, int len);

void save_history_bitrates(px_config_t * config, px_conn_t* px_conn);

#endif 