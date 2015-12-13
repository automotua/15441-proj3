#ifndef _DNS_HELPER_H
#define _DNS_HELPER_H

#define MAX_DNS_PKT 65535
#define MAX_DNS_NAME 256

void generate_header(char* pkt, unsigned short dns_id, int type, char RCODE);

int generate_question_section(char* pkt, const char* domain_name);

char* parse_dns_request(char* pkt, int size, unsigned short* dns_id);

char* generate_dns_response(char* qname, char* ip, unsigned short dns_id, char rcode, int* size);

int generate_record_section(char* pkt, char* qname, char* ip);

int parse_dns_response(struct in_addr* ip_addr, char* response_name, char* pkt, int size);

#endif
