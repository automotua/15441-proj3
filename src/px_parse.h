/*
 * px_parse.h
 *
 * Authors:     Ke Wu <kewu@andrew.cmu.edu>
 *              Junqiang Li <junqianl@andrew.cmu.edu>
 *
 *
 * Date:
 *
 * Description:
 */

#ifndef _PX_PARSE_H_
#define _PX_PARSE_H_

#define MAX_URL_LENGTH 256
#define PX_FILENAME_LEN 256
 #define MAX_HEADER_LENGTH 8192

enum REQUEST_TYPE {
    F4M_REQ,
    CHUNK_REQ
};

enum RESPONSE_EXPECT_TYPE {
    F4M_RESP,
    NOLIST_F4M_RESP,
    CHUNK_RESP
};

typedef struct bitrate_s {
    int bitrate;
    struct bitrate_s * next;
} bitrate_t;

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

    /* buffer stores the data that has been processed 
     * (has been processed by FSM to check CRLFCRLF) */
    char buffer[MAX_HEADER_LENGTH];
    int bufferSize;

    // FSM state
    int state;

    // stores the request that has been parsed by lex and yacc
    struct Request* request;

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
    

} server_conn_t;

typedef struct px_conn_s {
    browser_conn_t * b_conn;
    server_conn_t * s_conn;
    struct px_conn_s * next;
} px_conn_t;

struct px_config_s {
    char log_file[PX_FILENAME_LEN];
    float alpha;
    int listen_port;
    long fake_ip_s_addr;
    long dns_ip_s_addr;
    int dns_port;
    long www_ip_s_addr;

    int argc;
    char ** argv;

    px_conn_t * conns;

    // listen socket listening from browser
    int sock;

    fd_set readset;
    int max_fd;
};
typedef struct px_config_s px_config_t;


#endif /* _PX_PARSE_H_ */
