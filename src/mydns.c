#include "mydns.h"

static struct in_addr fake_ip_in_addr;
static struct in_addr dns_ip_in_addr;
static int dns_port;

int init_mydns(const char *dns_ip, unsigned int port, const char *local_ip) {
	inet_aton(local_ip, &fake_ip_in_addr);

    inet_aton(dns_ip, &dns_ip_in_addr);

    dns_port = port;
}

int resolve(const char *node, const char *service, 
            const struct addrinfo *hints, struct addrinfo **res) {
	

}