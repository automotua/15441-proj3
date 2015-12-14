/*
 * dns_helper.c
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *          Junqiang Li <junqiangl@andrew.cmu.edu>
 *
 * Date: 12-13-2015
 *
 * Description: some helper functions for generate dns header, generate dns
 *              request, generate dns response, parse dns request and parse
 *              dns response. 
 */

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "dns_helper.h"

void generate_header(char* pkt, unsigned short dns_id, int type,
                     char RCODE) {
    
    // set dns ID
    unsigned short dns_id_net = htons(dns_id);
    memcpy(pkt, &dns_id_net, 2);
    pkt += 2;

    char byte = (char)0;
    char mask;

    // set QR
    mask = type << 7;
    byte |= mask;

    // OPCODE is 0000

    // set AA
    mask = type << 2;
    byte |= mask;

    // TC is 0

    // RD is 0

    // copy byte
    memcpy(pkt, &byte, 1);
    pkt += 1;

    byte = (char)0;

    // RA is 0

    // Z is 0

    // set RCODE
    byte |= RCODE;

    // copy byte
    memcpy(pkt, &byte, 1);
    pkt += 1;

    // set count numbers
    unsigned short qdcount;
    unsigned short ancount;
    unsigned short nscount = 0;
    unsigned short arcount = 0;
    if (type == 0){
        qdcount = htons(1);
        ancount = 0;
    } else {
        if (RCODE == 0){
            qdcount = htons(1);
            ancount = htons(1);
        } else {
            qdcount = 0;
            ancount = 0;
        }
    }

    memcpy(pkt, &qdcount, 2);
    pkt += 2;
    memcpy(pkt, &ancount, 2);
    pkt += 2;
    memcpy(pkt, &nscount, 2);
    pkt += 2;
    memcpy(pkt, &arcount, 2);
    pkt += 2;
}

int generate_question_section(char* pkt, const char* domain_name) {
    char* init_pkt = pkt;

    // set QNAME
    char* dot = strchr(domain_name, '.');
    while (dot != NULL) {
        char num = (char)(dot - domain_name);
        // set length of label
        memcpy(pkt, &num, 1);
        pkt += 1;
        // copy label
        memcpy(pkt, domain_name, (int)num);
        pkt += (int)num;

        domain_name = dot + 1;
        dot = strchr(domain_name, '.');    
    }

    // the last label
    char num = (char)(strlen(domain_name));
    // set length of label
    memcpy(pkt, &num, 1);
    pkt += 1;
    // copy label
    memcpy(pkt, domain_name, (int)num);
    pkt += (int)num;

    // zero length octet for the null label
    char null = 0;
    memcpy(pkt, &null, 1);
    pkt += 1;

    // set QTYPE
    unsigned short qtype = htons(1);
    memcpy(pkt, &qtype, 2);
    pkt += 2;

    // set QCLASS
    unsigned short qclass = htons(1);
    memcpy(pkt, &qclass, 2);
    pkt += 2;

    return pkt - init_pkt;
}

char* parse_dns_request(char* pkt, int size, unsigned short* dns_id) {
    // at least 12 bytes (dns request header)
    if (size < 12){
        *dns_id = 0;
        return NULL;
    }

    // parse header, get dns_id
    unsigned short dns_id_net;
    memcpy(&dns_id_net, pkt, 2);

    *dns_id = ntohs(dns_id_net);

    // get 3rd byte
    char byte;
    memcpy(&byte, pkt+2, 1);
    
    // get QR
    int qr = (byte >> 7) & 1;
    if (qr != 0)
        return NULL;

    // get qdcount
    unsigned short qdcount_net;
    memcpy(&qdcount_net, pkt+4, 2);
    unsigned short qdcount = ntohs(qdcount_net);
    if (qdcount != 1)
        return NULL;

    // parse question section
    pkt += 12;
    size -= 12;
    char* query_name = malloc(MAX_DNS_NAME);
    char* cur = query_name;
    while (size > 0) {
        char num;
        // get length of label
        memcpy(&num, pkt, 1);
        pkt += 1;
        size -= 1;
        // check if the last label
        if (num == 0){
            // add terminate character
            *(cur-1) = '\0';
            break;
        }

        // copy label
        memcpy(cur, pkt, (int)num);
        cur += (int)num;
        pkt += (int)num;
        size -= (int)num;

        // add dot
        strncpy(cur, ".", 1);
        cur += 1;
    }  

    // should remain 4 bytes, 2 for QNAME and 2 for QTYPE
    if (size != 4)
        return NULL;

    // get qtype
    unsigned short qtype_net;
    memcpy(&qtype_net, pkt, 2);
    unsigned short qtype = ntohs(qtype_net);
    if (qtype != 1)
        return NULL;

    // get qclass
    unsigned short qclass_net;
    memcpy(&qclass_net, pkt+2, 2);
    unsigned short qclass = ntohs(qclass_net);
    if (qclass != 1)
        return NULL;

    return query_name;
}

char* generate_dns_response(char* qname, char* ip, unsigned short dns_id, 
                                                    char rcode, int* size) {
    char* packet = malloc(MAX_DNS_PKT);

    generate_header(packet, dns_id, 1, rcode);
    if (rcode == 0) {
        int question_size = generate_question_section(packet + 12, qname);
        int record_size = generate_record_section(packet + 12 + question_size, 
                                                                qname, ip);
    
        *size = 12 + question_size + record_size;
    }
    else
        *size = 12;
    

    return packet;
}

int generate_record_section(char* pkt, char* qname, char* ip) {
    char* init_pkt = pkt;

    // set NAME
    char* dot = strchr(qname, '.');
    while (dot != NULL) {
        char num = (char)(dot - qname);
        // set length of label
        memcpy(pkt, &num, 1);
        pkt += 1;
        // copy label
        memcpy(pkt, qname, (int)num);
        pkt += (int)num;

        qname = dot + 1;
        dot = strchr(qname, '.');    
    }

    // the last label
    char num = (char)(strlen(qname));
    // set length of label
    memcpy(pkt, &num, 1);
    pkt += 1;
    // copy label
    memcpy(pkt, qname, (int)num);
    pkt += (int)num;

    // zero length octet for the null label
    char null = 0;
    memcpy(pkt, &null, 1);
    pkt += 1;

    // set TYPE
    unsigned short type = htons(1);
    memcpy(pkt, &type, 2);
    pkt += 2;

    // set CLASS
    unsigned short dns_class = htons(1);
    memcpy(pkt, &dns_class, 2);
    pkt += 2;

    // set TTL
    unsigned int ttl = 0;
    memcpy(pkt, &ttl, 4);
    pkt += 4;

    // set RDLENGTH
    unsigned short len = htons(4);
    memcpy(pkt, &len, 2);
    pkt += 2;

    // set RDATA
    struct in_addr ip_addr;
    inet_aton(ip, &ip_addr);
    memcpy(pkt, &ip_addr.s_addr, 4);
    pkt += 4;

    return pkt - init_pkt;
}

int parse_dns_response(struct in_addr* ip_addr, char* response_name, char* pkt, 
                                                                    int size) {
    // at least 12 bytes (dns response header)
    if (size < 12)
        return -1;

    // get 3rd byte
    char byte;
    memcpy(&byte, pkt+2, 1);
    
    // get QR
    int qr = (byte >> 7) & 1;
    if (qr != 1)
        return -1;

    // get 4th byte
    memcpy(&byte, pkt+3, 1);

    // get rcode
    char mask = 0x0F;
    int rcode = byte & mask;
    if (rcode != 0)
        return -1;

    // get qdcount
    unsigned short qdcount_net;
    memcpy(&qdcount_net, pkt+4, 2);
    unsigned short qdcount = ntohs(qdcount_net);
    if (qdcount != 1)
        return -1;

    // get ancount
    unsigned short ancount_net;
    memcpy(&ancount_net, pkt+6, 2);
    unsigned short ancount = ntohs(ancount_net);
    if (ancount != 1)
        return -1;

    // parse question section
    pkt += 12;
    size -= 12;
    while (size > 0) {
        char num;
        // get length of label
        memcpy(&num, pkt, 1);
        pkt += 1;
        size -= 1;
        // check if the last label
        if (num == 0){
            break;
        }

        // skip label
        pkt += (int)num;
        size -= (int)num;
    }  

    // skip 4 bytes, 2 for QNAME and 2 for QTYPE
    pkt += 4;
    size -= 4;

    // parse resource record
    // parse NAME
    char* cur = response_name;
    while (size > 0) {
        char num;
        // get length of label
        memcpy(&num, pkt, 1);
        pkt += 1;
        size -= 1;
        // check if the last label
        if (num == 0){
            // add terminate character
            *(cur-1) = '\0';
            break;
        }

        // copy label
        memcpy(cur, pkt, (int)num);
        cur += (int)num;
        pkt += (int)num;
        size -= (int)num;

        // add dot
        strncpy(cur, ".", 1);
        cur += 1;
    }  

    pkt += 8;
    size -= 8;

    // should at least have 2 bytes for RDLENGTH
    if (size < 2)
        return -1;

    // parse RDLENGTH
    unsigned short len_net;
    memcpy(&len_net, pkt, 2);
    pkt += 2;
    size -= 2;
    unsigned short length = ntohs(len_net);
    if (length != 4)
        return -1;

    if (size != length)
    return -1; 

    memcpy(&(ip_addr->s_addr), pkt, 4);
    return 0;
}
