/*
 * proxy.c
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *          Junqiang Li <junqiangl@andrew.cmu.edu>
 *
 * Date: 12-13-2015
 *
 * Description: the main file of proxy. It will process request from browser
 *              and process response from server. 
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "proxy.h"
#include "px_parse.h"
#include "parser.h"
#include "http_parse.h"
#include "f4m.h"
#include "chunk.h"
#include "helper.h"

int main(int argc, char **argv) {
    px_config_t config;

    /* initialize configuration */
    px_init(&config, argc, argv);

    px_parse_command_line(&config);

    /* ignore SIGPIPE in case proxy is terminated due to broken pipe */
    signal(SIGPIPE, SIG_IGN);

    /* main routine */
    proxy_run(&config);
    return 0;
}

void proxy_run(px_config_t * config) {
    int sock;
    struct sockaddr_in myaddr;
    fd_set readyset;

    // pre setup proxy listening socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("proxy_run could not create socket");
        exit(-1);
    }

    config->sock = sock;

    bzero(&myaddr, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(config->listen_port);

    if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
        perror("proxy_run could not bind socket");
        exit(-1);
    }

    if (listen(sock, 5)) {
        perror("proxy_run could not listen socket");
        exit(-1);
    }
    // end setup proxy listening socket

    // config init
    config->conns = malloc(sizeof(px_conn_t));
    config->conns->next = NULL;
    config->history_bitrates = NULL;
    
    // do select prep
    FD_ZERO(&config->readset);
    FD_SET(sock, &config->readset);
    config->max_fd = sock;
    // end do select prep

    while (1) {
        // begin do readyset stuff
        int nfds;
        readyset = config->readset;
        nfds = select(config->max_fd + 1, &readyset, NULL, NULL, NULL);
        // end do readyset stuff

        if (nfds > 0) {
            // begin new client conn
            if (FD_ISSET(sock, &readyset)) {
                process_browser_conn(sock, config);
                nfds--;
            }
            // end new client conn

            px_conn_t * px_conn;
            browser_conn_t * b_conn;
            server_conn_t * s_conn;

            // begin process browser / server request
            for (px_conn = config->conns; px_conn && px_conn->next && nfds > 0; 
                                                    px_conn = px_conn->next) {
                b_conn = px_conn->next->b_conn;
                s_conn = px_conn->next->s_conn;

                int retval = 0;

                if (FD_ISSET(b_conn->fd, &readyset)) {
                    retval = process_browser_request(config, px_conn->next);
                    nfds--;
                }

                // something wrong, need to close connection
                if (retval) {
                    // delete b_conn and s_conn
                    close_connection(config, px_conn->next);
                    
                    // delete px_conn
                    px_conn_t* tmp = px_conn->next;
                    px_conn->next = px_conn->next->next;
                    free(tmp);
                    continue;
                }
                
                if (s_conn && FD_ISSET(s_conn->fd, &readyset)) {
                    retval = process_server_response(config, px_conn->next);
                    nfds--;
                }

                // something wrong, need to close connection
                if (retval) {
                    // delete b_conn and s_conn
                    close_connection(config, px_conn->next);
                    
                    // delete px_conn
                    px_conn_t* tmp = px_conn->next;
                    px_conn->next = px_conn->next->next;
                    free(tmp);
                }

            }
            // end process browser / server request
        }
    }
}

int process_browser_conn(int listenSock, px_config_t * config) {
    // accept(), create new connection
    struct sockaddr_in cliAddr;
    socklen_t cliSize = sizeof(cliAddr);
    int client_sock;
    
    if ((client_sock = accept(listenSock, (struct sockaddr *) &cliAddr,
                         &cliSize)) == -1) {
        perror("proxy_run could not accept socket");
        exit(-1);    
    }

    FD_SET(client_sock, &config->readset);

    if (config->max_fd < client_sock)
        config->max_fd = client_sock;

    browser_conn_t * b_conn = malloc(sizeof(browser_conn_t));
    b_conn->fd = client_sock;
    b_conn->bufferSize = 0;
    b_conn->state = STATE_START;
    b_conn->request = NULL;

    px_conn_t * conn = malloc(sizeof(px_conn_t));
    conn->b_conn = b_conn;
    conn->s_conn = NULL;
    conn->next = config->conns->next;

    config->conns->next = conn;

    logmessage("accept a new connection from browser\n", NULL, 0);

    return 0;
}


int process_browser_request(px_config_t * config, px_conn_t * px_conn) {
    browser_conn_t * b_conn = px_conn->b_conn;
    parse_browser_request(config, b_conn);
    if (b_conn->is_parse_done == 1) {
        logmessage("Receive request from browser\n", 
                                        b_conn->buffer, b_conn->bufferSize);
        if (b_conn->req_type == HTML_REQ) {
            if (process_html_request(config, px_conn) < 0) {
                fprintf(stderr, "Failed in process html request\n");
                return -1;
            }
        } else if (b_conn->req_type == F4M_REQ) {
            if (process_f4m_request(config, px_conn) < 0) {
                fprintf(stderr, "Failed in process f4m request\n");
                return -1;
            }
        } else if (b_conn->req_type == CHUNK_REQ) {
           if (process_chunk_request(config, px_conn) < 0) {
                fprintf(stderr, "Failed in process chunk request\n");
                return -1;
           }
        } else {
            fprintf(stderr, "unknown browser request type\n");
            return -1;
        }
        clean_bconn_after_parse(b_conn);
    } else if (b_conn->is_parse_done < 0) {
        fprintf(stderr, "Failed in parse browser request\n");
        return -1;
    }

    if (b_conn->is_close){
        logmessage("close connection due to browser\n", NULL, 0);
        return 1;
    }

    return 0;
}

int process_server_response(px_config_t * config, px_conn_t * px_conn) {
    server_conn_t * s_conn = px_conn->s_conn;
    parse_server_response(config, s_conn);
    if (s_conn->is_parse_done == 1) {
        logmessage("Receive response from server\n", s_conn->buffer, 
                                                            s_conn->bufferSize);
        if (s_conn->resp_type == HTML_RESP) {
            if (process_html_response(config, px_conn) < 0) {
                fprintf(stderr, "Failed in process html response\n");
                return -1;
            }
        } else if (s_conn->resp_type == F4M_RESP) {
            if (process_f4m_response(config, px_conn) < 0) {
                fprintf(stderr, "Failed in process f4m response\n");
                return -1;
            }
        } else if (s_conn->resp_type == NOLIST_F4M_RESP) {
            if (process_nolist_f4m_response(config, px_conn) < 0) {
                fprintf(stderr, "Failed in process nolist f4m response\n");
                return -1;
            }
        } else if (s_conn->resp_type == CHUNK_RESP) {
            if (process_chunk_response(config, px_conn) < 0) {
                fprintf(stderr, "Failed in process chunk response\n");
                return -1;
            }
        } else {
            fprintf(stderr, "unknown server response type\n");
            return -1;
        }
        clean_sconn_after_parse(s_conn);
    } else if (s_conn->is_parse_done < 0){
        fprintf(stderr, "Failed in parse server response\n");
        return -1;
    }

    if (s_conn->is_close){
        logmessage("close connection due to server\n", NULL, 0);
        return 1;
    }

    return 0;
}

/* close connection of browser and server, need to save state (available
    bitrates, throughput) in case connection will be rebuilt */
void close_connection(px_config_t * config, px_conn_t* px_conn) {
    server_conn_t * s_conn = px_conn->s_conn;
    browser_conn_t* b_conn = px_conn->b_conn;

    save_history_bitrates(config, px_conn);
    
    if (b_conn) {
        clean_bconn_after_parse(b_conn);
        FD_CLR(b_conn->fd, &config->readset);
        close(b_conn->fd);
        free(b_conn);
    }

    if (s_conn) {
        clean_sconn_after_parse(s_conn);
        FD_CLR(s_conn->fd, &config->readset);
        close(s_conn->fd);

        free(s_conn);
    }
    
}
