/*
 * helper.c
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *          Junqiang Li <junqiangl@andrew.cmu.edu>
 *
 * Date: 12-13-2015
 *
 * Description: including some helper functions.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include "helper.h"
#include "px_parse.h"
#include "mydns.h"

// generate HTTP/1.1 request that will be send to server
char* generate_request_to_server(browser_conn_t * b_conn, char* request_file, 
                                                            char* serverhost){
    char *request = malloc(MAX_HEADER_LENGTH);

    // generate the first line
    strcpy(request, "GET ");
    strcat(request, request_file);
    strcat(request, " HTTP/1.1\r\n");

    struct MessageHeader* header = b_conn->request->headers;
    // add header
    while (header){
        strcat(request, header->fieldName);
        strcat(request, ":");
        // modify host header
        if (strcmp(header->fieldName, "Host") == 0 ||
            strcmp(header->fieldName, "host") == 0) {
            strcat(request, serverhost);
        } else
            strcat(request, header->fieldValue);
        strcat(request, "\r\n");
        header = header->next;
    }
    
    strcat(request, "\r\n");

    return request;
}

// send data(buf) using TCP(sock) 
int send_data_to_socket(int sock, char* buf, int size){
    /* send data to browser, may need send many times */
    int writeret;
    while (size > 0) {
        writeret = send(sock, buf, size, 0);
        if (writeret < 0)
        {
            /* Interrupted by signal, will send() again */
            if (errno == EINTR) 
                writeret = 0;
            else {
                /* send data error */
                return -1;
            }
        }
        size -= writeret;
        buf += writeret;
    }
    return 0;
}

// build connection with server (may connect dns server to resolve)
int init_server_connection(px_config_t * config, px_conn_t * px_conn) {
    if (px_conn->s_conn)
        return 0;

    int sock;

    struct sockaddr_in proxyaddr;
    bzero(&proxyaddr, sizeof(proxyaddr));
    proxyaddr.sin_family = AF_INET;
    proxyaddr.sin_port = htons(0);
    proxyaddr.sin_addr = config->fake_ip_in_addr;

    struct sockaddr_in serveraddr;
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(SERVER_LISTEN_PORT);
    
    if (config->www_ip_in_addr.s_addr != -1)
        // get server ip directly from command
        serveraddr.sin_addr = config->www_ip_in_addr;
    else {
        // connect dns server to get server ip 
        struct addrinfo *result;
        if (resolve("video.cs.cmu.edu", "8080", NULL, &result) < 0)
            return -1;
        // save result
        serveraddr.sin_addr = ((struct sockaddr_in*)result->ai_addr)->sin_addr;
        free(result);
    }
    
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      return -1;

    if (bind(sock, (struct sockaddr*)&proxyaddr, sizeof(proxyaddr)) < 0)
      return -1;

    if (connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
      return -1;

    // init state for server connection
    server_conn_t* s_conn = malloc(sizeof(server_conn_t));
    s_conn->fd = sock;
    s_conn->file_data = NULL;
    s_conn->bufferSize = 0;
    s_conn->state = STATE_START;
    s_conn->content_length = -1;
    s_conn->bitrates = NULL;
    s_conn->addr = serveraddr.sin_addr;

    px_conn->s_conn = s_conn;

    FD_SET(sock, &config->readset);
    if (config->max_fd < sock)
      config->max_fd = sock;

    return 0;
}

/* save state of connection (available bitrates, throughput) so that when 
    connection is rebuilt, it can restore the state */
void save_history_bitrates(px_config_t * config, px_conn_t* px_conn) {
    server_conn_t * s_conn = px_conn->s_conn;
    browser_conn_t* b_conn = px_conn->b_conn;

    //save bitrates to history
    history_bitrate_t* history_bitrate = config->history_bitrates;
    int is_add = 1;
    while (b_conn && s_conn && history_bitrate){
        if (strlen(b_conn->video_path) == strlen(history_bitrate->video_path) &&
            strcmp(b_conn->video_path, history_bitrate->video_path) == 0){
            is_add = 0;
            history_bitrate->throughput = px_conn->throughput;
            break;
        }
        history_bitrate = history_bitrate->next;
    }
    if (is_add && b_conn && s_conn){
        history_bitrate = malloc(sizeof(history_bitrate_t));
        history_bitrate->bitrates = s_conn->bitrates;
        history_bitrate->throughput = px_conn->throughput;
        history_bitrate->video_path = malloc(strlen(b_conn->video_path) + 1);
        strcpy(history_bitrate->video_path, b_conn->video_path);
        history_bitrate->next = config->history_bitrates;
        config->history_bitrates = history_bitrate;
    }
}

void logmessage(char* msg1, char* msg2, int len) {
    FILE* file = fopen("mylog", "a");
    fwrite(msg1, 1, strlen(msg1), file);
    if (msg2)
        fwrite(msg2, 1, len, file);
    fclose(file);
}

