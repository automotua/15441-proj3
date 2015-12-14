/*
 * mydns.c
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *          Junqiang Li <junqiangl@andrew.cmu.edu>
 *
 * Date: 12-13-2015
 *
 * Description: dns resolution library
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <arpa/inet.h>
#include "mydns.h"
#include "dns_helper.h"

static int sock, slen;
static struct sockaddr_in dnsserveraddr;

int init_mydns(const char *dns_ip, unsigned int dns_port, const char *local_ip){
    struct in_addr fake_ip_in_addr;
    struct in_addr dns_ip_in_addr;

    inet_aton(local_ip, &fake_ip_in_addr);
    inet_aton(dns_ip, &dns_ip_in_addr);

    struct sockaddr_in proxyaddr;
    bzero(&proxyaddr, sizeof(proxyaddr));
    proxyaddr.sin_family = AF_INET;
    proxyaddr.sin_port = htons(0);
    proxyaddr.sin_addr = fake_ip_in_addr;

    bzero(&dnsserveraddr, sizeof(dnsserveraddr));
    dnsserveraddr.sin_family = AF_INET;
    dnsserveraddr.sin_port = htons(dns_port);
    dnsserveraddr.sin_addr = dns_ip_in_addr;

    slen = sizeof(dnsserveraddr);

    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
      return -1;

    if (bind(sock, (struct sockaddr*)&proxyaddr, sizeof(proxyaddr)) < 0)
      return -1;

    return 0;
}

int resolve(const char *node, const char *service, 
            const struct addrinfo *hints, struct addrinfo **res) {
    
    char packet[MAX_DNS_PKT];

    // generate dns request
    generate_header(packet, 1, 0, 0);
    int size = generate_question_section(packet + 12, node);
    size += 12;

    // send dns request
    if (sendto(sock, packet, size, 0, (struct sockaddr*)&dnsserveraddr, slen)<0)
        return -1;
    // wait to receive dns response
    if ((size = recvfrom(sock, packet, MAX_DNS_PKT, 0, 
                (struct sockaddr *) &dnsserveraddr, (socklen_t *)&slen)) < 0)
        return -1;

    // parse dns response
    char response_name[MAX_DNS_NAME];
    struct in_addr ip_addr;
    if (parse_dns_response(&ip_addr, response_name, packet, size) < 0)
        return -1;

    // check response name
    if (strcmp(response_name, node) != 0)
        return -1;

    struct sockaddr_in* addr = malloc(sizeof(struct sockaddr_in));
    addr->sin_addr = ip_addr;
    
    *res = malloc(sizeof(struct addrinfo));
    (*res)->ai_addr = (struct sockaddr *) addr;

    return 0;
}

