/*
 * proxy.c
 *
 */
#include "proxy.h"


int main(int argc, char **argv) {
    px_config_t config;

    /* initialize configuration */
    px_init(&config, argc, argv);

    DPRINTF(DEBUG_INIT, "proxy.c main beginning\n");

    px_parse_command_line(&config);

#ifdef DEBUG
    if (debug & DEBUG_INIT) {
        px_dump_config(&config);
    }
#endif

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

    // TODO pre setup proxy listening socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
        perror("peer_run could not create socket");
        exit(-1);
    }

    config->sock = sock;

    bzero(&myaddr, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = config->myaddr.sin_addr.s_addr;
    //inet_aton("127.0.0.1", (struct in_addr *)&myaddr.sin_addr.s_addr);
    myaddr.sin_port = htons(config->myport);

    if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
        perror("peer_run could not bind socket");
        exit(-1);
    }

    // TODO call listen() for the listener socket
    // end setup proxy listening socket

    // TODO: bind fake ip to outbound sock

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

            px_conn_t * px_conn = config->conns;
            browser_conn_t * b_conn;
            server_conn_t * s_conn;

            // begin process browser / server request
            for (; px_conn && nfds > 0; px_conn = px_conn->next) {
                b_conn = px_conn->b_conn;
                s_conn = px_conn->s_conn;

                if (FD_ISSET(b_conn->fd, &readyset)) {
                    process_browser_request(config, px_conn);
                    nfds--;
                }

                if (s_conn && FD_ISSET(s_conn->fd, &readyset)) {
                    process_server_response(config, px_conn);
                    nfds--;
                }

            }
            // end process browser / server request
        }
    }
}

void process_browser_conn(int sock, px_config_t * config) {
    // TODO process new browser connection

}


void process_browser_request(px_config_t * config, px_conn_t * px_conn) {
    browser_conn_t * b_conn = px_conn->b_conn;
    parse_browser_request(config, b_conn);
    if (b_conn->is_parse_done) {
        if (b_conn->req_type == F4M_REQ) {
            // TODO for F4M_REQ, set s_conn->resp_type = F4M_RESP
            process_f4m_request(config, px_conn);
        } else if (b_conn->req_type == CHUNK_REQ) {
            process_chunk_request(config, px_conn);
        } else {
            // TODO unknown browser request type
        }
        clean_bconn_after_parse(b_conn);
    }
}

void process_server_response(px_config_t * config, px_conn_t * px_conn) {
    // TODO process server response
    server_conn_t * s_conn = px_conn->s_conn;
    parse_server_response(config, s_conn);
    if (s_conn->is_parse_done) {
        if (s_conn->resp_type == F4M_RESP) {
            process_f4m_response(config, px_conn);
        } else if (s_conn->resp_type == NOLIST_F4M_RESP) {
            process_nolist_f4m_response(config, px_conn);
        } else if (s_conn->resp_type == CHUNK_RESP) {
            process_chunk_response(config, px_conn);
        } else {
            // TODO unknown server response type
        }
        clean_sconn_after_parse(s_conn);
    }
}
