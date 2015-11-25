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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void proxy_run(px_config_t * config);
void process_browser_conn(int sock, px_config_t * config);
void process_browser_request(px_config_t * config, px_conn_t * px_conn);
void process_server_response(px_config_t * config, px_conn_t * px_conn);


#endif /* _PROXY_H_ */
