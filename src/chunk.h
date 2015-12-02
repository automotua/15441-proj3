#ifndef _CHUNK_H_
#define _CHUNK_H_

#include "px_parse.h"

int process_chunk_request(px_config_t * config, px_conn_t * px_conn);

int process_chunk_response(px_config_t * config, px_conn_t * px_conn);

void calculate_throughput(px_config_t * config, px_conn_t* conn, int byte_size, double diff_time);

void replace_url(browser_conn_t* b_conn, int bitrate);

void logging(px_config_t * config, px_conn_t* conn, double duration, double tput, double avg_tput);
#endif