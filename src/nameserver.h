/*
 * nameserver.h
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *          Junqiang Li <junqiangl@andrew.cmu.edu>
 *
 * Date: 12-13-2015
 *
 * Description: header file of nameserver.c
 *
 */

#ifndef _NAMESERVER_H_
#define _NAMESERVER_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 

#define NS_FILENAME_LEN 256
#define VALID_QUERY_NAME "video.cs.cmu.edu"

typedef struct ns_config_s {
	int is_robin;
	char log_file_path[NS_FILENAME_LEN];
	struct in_addr ip_in_addr;
	int port;
	char servers_file_path[NS_FILENAME_LEN];
	char LSAs_file_path[NS_FILENAME_LEN];
} ns_config_t;

int parse_command(int argc, char** argv, ns_config_t* config);

void nameserver_run(ns_config_t* config);

void send_dns_pkt(int sock, struct sockaddr_in* from, char* ip, 
											unsigned short dns_id, int rcode);

void send_packet(int socket, char* data, size_t packet_len, int flag,
                 struct sockaddr *dest_addr, socklen_t addr_len);

void logging(char* log_file, struct in_addr* client_ip, char* query_name, 
															char* response_ip);

#endif