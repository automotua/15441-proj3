/*
 *  parser.h
 *  
 *  Created by: Ke Wu <kewu@andrew.cmu.edu>
 *
 *  Description: header file for yacc
 *  
 */
#ifndef _PARSER_H_
#define _PARSER_H_

/* define struct Request, when yyparse() parse the request, it will fill 
 * each parts of struct Request, so the mainRoutine could use each fiedld 
 * directly
 */

enum {
    STATE_START = 0,
    STATE_CR,
    STATE_CRLF,
    STATE_CRLFCR,
    STATE_CRLFCRLF
};

struct HTTPVersion{
    char* http;
    int version[2];
};

struct RequestURI{
    int type;
    char* absPath;
    int port;
    char* host;
    char* query;
};

struct RequestLine{
    char* method;
    struct RequestURI requestURI;
    struct HTTPVersion httpVersion;
};

struct MessageHeader{
    char* fieldName;
    char* fieldValue;
    struct MessageHeader* next;
};

struct Request{
    struct RequestLine requestLine;
    struct MessageHeader* headers;
};

/* copy a string with malloc */
char* deepCopyString(char* str);

/* parse function of yacc */
int yyparse ();

/* set buffer for parsing */
void set_parsing_buf(char *buf, size_t siz);

/* used for restart yacc */
extern void yyrestart( FILE *new_file);

#endif