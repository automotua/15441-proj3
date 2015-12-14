/*
 * px_parse.h
 *
 * Authors:     Ke Wu <kewu@andrew.cmu.edu>
 *              Junqiang Li <junqianl@andrew.cmu.edu>
 *
 * Date: 12-13-2015
 *
 * Description: header file of px_parse.c
 */

#ifndef _PX_PARSE_H_
#define _PX_PARSE_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "parser.h"


#define MAX_URL_LENGTH 256
#define PX_FILENAME_LEN 256
#define MAX_HEADER_LENGTH 8192
#define SERVER_LISTEN_PORT 8080
#define SELECT_TIMEOUT 10

enum REQUEST_TYPE {
    HTML_REQ,
    F4M_REQ,
    CHUNK_REQ
};

enum RESPONSE_EXPECT_TYPE {
    HTML_RESP,
    F4M_RESP,
    NOLIST_F4M_RESP,
    CHUNK_RESP
};

typedef struct bitrate_s {
    int bitrate;
    struct bitrate_s * next;
} bitrate_t;

typedef struct history_bitrate_s {
    bitrate_t * bitrates;
    char* video_path;
    struct history_bitrate_s* next;
    int throughput;
} history_bitrate_t;

typedef struct browser_conn_s {
    //TODO depends on how browser conn
    //     is parsed
    enum REQUEST_TYPE req_type;

    // browser client socket fd
    int fd;

    // is parsing done(1), not(0), or close connection due to error(-1) 
    int is_parse_done;

    // the url string after GET and before HTTP/1.1
    char url[MAX_URL_LENGTH];

    char video_path[MAX_URL_LENGTH];

    /* buffer stores the data that has been processed 
     * (has been processed by FSM to check CRLFCRLF) */
    char buffer[MAX_HEADER_LENGTH];
    int bufferSize;

    // FSM state
    int state;

    // stores the request that has been parsed by lex and yacc
    struct Request* request;

    char* tmp_nolist_request;

    int is_close;

} browser_conn_t;

typedef struct server_conn_s {
    //TODO depends on how server conn
    //     is parsed

    enum RESPONSE_EXPECT_TYPE resp_type;

    // binded socket btw proxy and server
    int fd;

    // is parsing done(1), not(0), or close connection due to error(-1) 
    int is_parse_done;

    // the file data (either .f4m or chunk files)
    // returned as response body
    char * file_data;
    int cur_size;

    /* buffer stores the header data that has been processed 
     * (has been processed by FSM to check CRLFCRLF) */
    char buffer[MAX_HEADER_LENGTH];
    int bufferSize;

    // FSM state
    int state;

    int content_length;

    // available bitrates
    bitrate_t * bitrates;

    struct in_addr addr;

    int is_close;

} server_conn_t;

typedef struct px_conn_s {
    browser_conn_t * b_conn;
    server_conn_t * s_conn;
    double throughput;
    struct timeval timer;
    int bitrate;
    struct px_conn_s * next;
} px_conn_t;

struct px_config_s {
    char log_file[PX_FILENAME_LEN];
    float alpha;
    int listen_port;
    struct in_addr fake_ip_in_addr;
    struct in_addr dns_ip_in_addr;
    int dns_port;
    struct in_addr www_ip_in_addr;

    int argc;
    char ** argv;

    px_conn_t * conns;

    // listen socket listening from browser
    int sock;

    fd_set readset;
    int max_fd;

    history_bitrate_t* history_bitrates;
};
typedef struct px_config_s px_config_t;

void px_init(px_config_t * config, int argc, char **argv);

void px_parse_command_line(px_config_t * config);

#endif /* _PX_PARSE_H_ */
