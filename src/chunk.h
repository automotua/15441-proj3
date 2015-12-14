/*
 * chunk.h
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *          Junqiang Li <junqiangl@andrew.cmu.edu>
 *
 * Date: 12-13-2015
 *
 * Description: header file of chunk.c
 *
 */

#ifndef _CHUNK_H_
#define _CHUNK_H_

#include "px_parse.h"

int process_chunk_request(px_config_t * config, px_conn_t * px_conn);

int process_chunk_response(px_config_t * config, px_conn_t * px_conn);

void calculate_throughput(px_config_t * config, px_conn_t* conn, int byte_size,
											double diff_time, int max_bitrate);

void replace_url(browser_conn_t* b_conn, int bitrate);

void logging(px_config_t * config, px_conn_t* conn, double duration, 
				double tput, double avg_tput);
#endif