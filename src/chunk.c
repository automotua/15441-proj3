/*
 * chunk.c
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *          Junqiang Li <junqiangl@andrew.cmu.edu>
 *
 * Date: 12-13-2015
 *
 * Description: functions to process chunk reqeust from browser and chunk 
 *              response from server. It also contains function of 
 *              estimating throughput and logging. 
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "chunk.h"
#include "helper.h"
#include "px_parse.h"

int process_chunk_request(px_config_t * config, px_conn_t * px_conn) {
    server_conn_t * s_conn = px_conn->s_conn;
    browser_conn_t* b_conn = px_conn->b_conn;

    if (!s_conn){
        init_server_connection(config, px_conn);
        s_conn = px_conn->s_conn;
        /* server connection is reconstructed, so connection state (available
           bitrates, throughput) should be restored */
        char* slash = strrchr(b_conn->url, '/');
        history_bitrate_t* history_bitrate = config->history_bitrates;
        while(history_bitrate){
            if (strlen(history_bitrate->video_path) == (slash - b_conn->url + 1) 
                && strncmp(history_bitrate->video_path, 
                                b_conn->url, slash - b_conn->url + 1) == 0){
                s_conn->bitrates = history_bitrate->bitrates;
                px_conn->throughput = history_bitrate->throughput;
                strcpy(b_conn->video_path, history_bitrate->video_path);
                break;
            }
            history_bitrate = history_bitrate->next;
        }
    }

    // start timing
    gettimeofday(&px_conn->timer, NULL);

    // adapt bitrate
    int bitrate = -1;
    bitrate_t* p = s_conn->bitrates;
    if (!p){
        fprintf(stderr, "can not find bitrates for the connection\n");
        return -1;
    }
    while (p){
        if (px_conn->throughput >= 1.5 * p->bitrate){
            bitrate = (bitrate < p->bitrate) ? p->bitrate : bitrate;
        }
        p = p->next;
    }

    // throughput is too low to find a bitrate, use the smallest bitrate
    if (bitrate == -1){
        p = s_conn->bitrates;
        bitrate = p->bitrate;
        while (p){
            bitrate = (bitrate > p->bitrate) ? p->bitrate : bitrate;
            p = p->next;
        }
    }

    px_conn->bitrate = bitrate;
    replace_url(b_conn, bitrate);

    s_conn->resp_type = CHUNK_RESP;

    char* request = generate_request_to_server(b_conn, b_conn->url, 
                                                            "localhost:8080");
    if (send_data_to_socket(s_conn->fd, request, strlen(request)) < 0){
        fprintf(stderr, "send data to server error\n");
        free(request);
        return -1;
    }

    free(request);
    
    return 0;
}


int process_chunk_response(px_config_t * config, px_conn_t * px_conn) {
    server_conn_t * s_conn = px_conn->s_conn;
    browser_conn_t* b_conn = px_conn->b_conn;

    // find maximum bitrates;
    int max_bitrate = -1;
    bitrate_t* p = s_conn->bitrates;
    while (p) {
        max_bitrate = (max_bitrate < p->bitrate) ? p->bitrate : max_bitrate;
        p = p->next;
    }
    if (max_bitrate == -1)
        return -1;

    // calculate throughput
    struct timeval now, result;
    gettimeofday(&now, NULL);
    timersub(&now, &px_conn->timer, &result);
    double elapse = result.tv_sec + (double)result.tv_usec / 1000000;
    calculate_throughput(config, px_conn, s_conn->cur_size, elapse,max_bitrate);

    save_history_bitrates(config, px_conn);

    if (send_data_to_socket(b_conn->fd, s_conn->file_data, s_conn->cur_size)< 0)
        return -1;

    return 0;
}

void calculate_throughput(px_config_t * config, px_conn_t* conn, int byte_size, 
                                            double diff_time, int max_bitrate) {

    double new_throughput = byte_size * 8 / 1000 / diff_time;
    // if estimated throughput is too high, then just set it to 1.5 times of 
    // the maximum available bitrate
    if (new_throughput > max_bitrate * 1.5) {
        new_throughput = max_bitrate * 1.5;
    }
    conn->throughput = 
        config->alpha * new_throughput + (1-config->alpha) * conn->throughput;

    logging(config, conn, diff_time, new_throughput, conn->throughput);
}

void replace_url(browser_conn_t* b_conn, int bitrate) {
    char prefix[MAX_URL_LENGTH];
    char suffix[MAX_URL_LENGTH];

    char* slash = strrchr(b_conn->url, '/');
    strncpy(prefix, b_conn->url, slash - b_conn->url + 1);
    prefix[slash - b_conn->url + 1] = '\0';

    char* seg = strstr(slash, "Seg");
    strcpy(suffix, seg);

    sprintf(b_conn->url, "%s%d%s", prefix, bitrate, suffix);
}

void logging(px_config_t * config, px_conn_t* conn, double duration, 
                double tput, double avg_tput) {
    time_t now;
    time(&now);

    char log[1024];
    sprintf(log, "%ld %.2f %.2f %.2f %d %s %s\n", now, duration,
                tput, avg_tput, conn->bitrate, inet_ntoa(conn->s_conn->addr),
                conn->b_conn->url);

    FILE* file = fopen(config->log_file, "a");
    fwrite(log, 1, strlen(log), file);
    fclose(file);
}