./src/
├── chunk.c                  Some functions to process chunk reqeust from browser and chunk response from server. It also contains function of estimating throughput and logging. 
├── chunk.h
├── dns_helper.c             Some helper functions for generate dns header, generate dns request, generate dns response, parse dns request and parse dns response.
├── dns_helper.h
├── f4m.c                    Some functions to process HTML/f4m/nolist.f4m reqeust from browser and HTML/f4m/nolist.f4m response from server. It also contains function of parse f4m XML file.
├── f4m.h
├── helper.c                 Some helper functions.
├── helper.h
├── http_parse.c             Parse HTTP request from browser and HTTP response from server
├── http_parse.h
├── mydns.c                  DNS resolution library
├── mydns.h
├── nameserver.c             The main function of nameserver, it will listen dns request, parse the request, choose a CDN server ip based on load balancing and send a dns response back.
├── nameserver.h
├── ospf.c                   It will read a lsa file and build a graph. Then it will apply dijkstra algorithm (or round-robin...) to find a cloest server for a client. 
├── ospf.h
├── proxy.c                  The main function of proxy. It will process request from browser and process response from server. 
├── proxy.h
├── px_parse.c               Parse command line of proxy
├── px_parse.h
├── lexer.l                  Lex file of parsing HTTP
├── parse.y                  Yacc file of parsing HTTP
├── parse.h                  Header file for yacc
├── Makefile                 The main function for CloudFS that parses
└── README.txt               This file                

For proxy, proxy.c, px_parse.c, chunk.c, dns_helper.c, f4m.c, helper.c, http_parse.c, mydns.c will be used

For nameserver, nameserver.c, ospf.c, dns_helper.c will be used