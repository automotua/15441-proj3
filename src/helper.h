/*
 * helper.h
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *          Junqiang Li <junqiangl@andrew.cmu.edu>
 *
 * Date: 12-13-2015
 *
 * Description: header file of helper.c
 *
 */
#ifndef _HELPER_H_
#define _HELPER_H_

#include "px_parse.h"

// generate HTTP/1.1 request that will be send to server
char* generate_request_to_server(browser_conn_t * b_conn, char* request_file, 
									char* serverhost);

// send data(buf) using TCP(sock) 
int send_data_to_socket(int sock, char* buf, int size);

// build connection with server (may connect dns server to resolve)
int init_server_connection(px_config_t * config, px_conn_t * px_conn);

void logmessage(char* msg1, char* msg2, int len);

/* save state of connection (available bitrates, throughput) so that when 
    connection is rebuilt, it can restore the state */
void save_history_bitrates(px_config_t * config, px_conn_t* px_conn);

#endif 