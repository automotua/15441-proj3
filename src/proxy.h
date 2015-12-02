/*
 * proxy.h
 *
 * Authors:
 *
 *
 * Date:
 *
 * Description:
 *
 */

#ifndef _PROXY_H_
#define _PROXY_H_

#include "px_parse.h"

void proxy_run(px_config_t * config);

int process_browser_conn(int listenSock, px_config_t * config);

int process_browser_request(px_config_t * config, px_conn_t * px_conn);

int process_server_response(px_config_t * config, px_conn_t * px_conn);

void close_connection(px_config_t * config, px_conn_t* px_conn);

#endif /* _PROXY_H_ */
