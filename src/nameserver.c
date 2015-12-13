#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <arpa/inet.h>
#include <time.h>
#include "nameserver.h"
#include "ospf.h"
#include "dns_helper.h"

int main(int argc, char** argv) {
    ns_config_t config;

    if (parse_command(argc, argv, &config) < 0){
        fprintf(stderr, "Can not parse the command\n");
        exit(1);
    }

    nameserver_run(&config);
}

int parse_command(int argc, char** argv, ns_config_t* config) {
    if (argc != 6 && argc != 7)
        return -1;
    
    int start_index = 0;
    
    if (strcmp(argv[1], "-r") == 0){
        start_index = 1;
        config->is_robin = 1;
    } else
        config->is_robin = 0;

    strcpy(config->log_file_path, argv[start_index + 1]);

    inet_aton(argv[start_index + 2], &config->ip_in_addr);

    config->port = atoi(argv[start_index + 3]);

    strcpy(config->servers_file_path, argv[start_index + 4]);

    strcpy(config->LSAs_file_path, argv[start_index + 5]);

    return 0;
}

void nameserver_run(ns_config_t* config){
    int sock;
    struct sockaddr_in myaddr, from;
    fd_set readset;
    socklen_t fromlen = sizeof(from);
    char buf[BUF_LEN];

    if (init_ospf(config->LSAs_file_path) < 0){
        fprintf(stderr, "initialize ospf error\n");
        exit(1);
    }

    if (mark_server(config->servers_file_path) < 0){
        fprintf(stderr, "mark server error\n");
        exit(1);
    }

    bzero(&myaddr, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr = config->ip_in_addr;
    myaddr.sin_port = htons(config->port);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("nameserver_run could not create socket");
        exit(-1);
    }

    if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
        perror("nameserver_run could not bind socket");
        exit(1);
    }

    /* init readset */
    FD_ZERO(&readset);
    FD_SET(sock, &readset);

    while (1) {
        select(sock+1, &readset, NULL, NULL, NULL);

        /* receive a UDP packet */
        int size = recvfrom(sock, buf, BUF_LEN, 0, (struct sockaddr *) &from, &fromlen);
        if (size < 0) {
            perror("error in recvfrom()");
            continue;
        }

        unsigned short dns_id = 0;
        char* query_name = parse_dns_request(buf, size, &dns_id);
        if (query_name != NULL) {
            if (strcmp(query_name, VALID_QUERY_NAME) != 0) {
                // Name Error
                send_dns_pkt(sock, &from, NULL, dns_id, 3);
            } else {
                char client_ip[MAX_NODE_LEN];
                sprintf(client_ip, "%s", inet_ntoa(from.sin_addr));

                char result_ip[MAX_NODE_LEN];
                if (find_closest_server(config->is_robin, client_ip, result_ip) < 0){
                    // Server failure
                    send_dns_pkt(sock, &from, NULL, dns_id, 2);
                } else {
                    // No error condition
                    logging(config->log_file_path, &from.sin_addr, query_name, result_ip);
                    send_dns_pkt(sock, &from, result_ip, dns_id, 0);
                }
            }
            free(query_name);
        } else 
            // Query format error
            send_dns_pkt(sock, &from, NULL, dns_id, 1);
    }

}

void send_dns_pkt(int sock, struct sockaddr_in* from, char* ip, unsigned short dns_id, int rcode) {
    int pkt_size;
    char* pkt = generate_dns_response(VALID_QUERY_NAME, ip, dns_id, rcode, &pkt_size);
    send_packet(sock, pkt, pkt_size, 0, (struct sockaddr *)from, sizeof(*from));
    free(pkt);
}

/* description: send a packet using sendto */
void send_packet(int socket, char* data, size_t packet_len, int flag,
                 struct sockaddr *dest_addr, socklen_t addr_len) {

    int has_send = 0, ret;
    while (has_send < packet_len){
        ret = sendto(socket, data + has_send, packet_len - has_send, 0,
                            dest_addr, addr_len);
        if (ret < 0) {
            perror("send packet error");
            exit(-1);
        } else
            has_send += ret;
    }
}

void logging(char* log_file, struct in_addr* client_ip, char* query_name, char* response_ip) {
    time_t now;
    time(&now);

    char log[1024];
    sprintf(log, "%ld %s %s %s\n", now, inet_ntoa(*client_ip), query_name, response_ip);

    FILE* file = fopen(log_file, "a");
    fwrite(log, 1, strlen(log), file);
    fclose(file);
}