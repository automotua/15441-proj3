
char* parse_dns_request(char* pkt, int size);

char* generate_dns_response(char* ip, int rcode, int* size);