/*
 * f4m.h
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *          Junqiang Li <junqiangl@andrew.cmu.edu>
 *
 */

#ifndef _F4M_H_
#define _F4M_H_

#include "px_parse.h"

int process_html_request(px_config_t * config, px_conn_t * px_conn);

int process_html_response(px_config_t * config, px_conn_t * px_conn);

int process_f4m_request(px_config_t * config, px_conn_t * px_conn);

int process_f4m_response(px_config_t * config, px_conn_t * px_conn);

int process_nolist_f4m_response(px_config_t * config, px_conn_t * px_conn);

#endif