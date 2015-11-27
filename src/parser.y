/**
 * @file parser.y
 * @brief Grammar for HTTP
 * Created by: Ke Wu <kewu@andrew.cmu.edu>
 */

%{
#include <stdio.h>     /* C declarations used in actions */
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "parser.h"

#define SUCCESS 0

/* Define YACCDEBUG to enable debug messages for this lex file */
#ifdef YACCDEBUG
#include <stdio.h>
#define YPRINTF(...) printf(__VA_ARGS__)
#else
#define YPRINTF(...)
#endif


/* yyparse() calls yyerror() on error */
void yyerror (char *s);

/* yyparse() calls yylex() to get tokens */
extern int yylex();

/*
** Global variables required for parsing from buffer
** instead of stdin:
*/

/* Pointer to the buffer that contains input */
char *parsing_buf;

/* Current position in the buffer */
int parsing_offset;

/* Buffer size */
size_t parsing_buf_siz;

struct Request* requestResult;

char* deepCopyString(char* str) {
	if (str == NULL || (strcmp(str, "") == 0))
		return NULL;
	char* newStr = malloc(strlen(str) + 1);
	strcpy(newStr, str);
	return newStr;
}
%}

/* Various types values that we can get from lex */
%union {
	char str[8192];
	int i;
}

%start request

/*
 * Tokens that yacc expects from lex, essentially these are the tokens
 * declared in declaration section of lex file.
 */
%token t_crlf
%token t_backslash
%token t_digit
%token t_dot
%token t_token_char
%token t_lws
%token t_colon
%token t_separators
%token t_sym_and
%token t_sym_plus
%token t_sym_dollar
%token t_sym_percent
%token t_sym_minus
%token t_sym_dash
%token t_sym_excl
%token t_sym_prime
%token t_sym_quot
%token t_sym_star

%token t_sp
%token t_ws

/* Type of value returned for these tokens */
%type<str> t_crlf
%type<i> t_backslash
%type<i> t_digit
%type<i> t_dot
%type<i> t_token_char
%type<str> t_lws
%type<i> t_colon
%type<i> t_separators

%type<i> t_sp
%type<str> t_ws

/*
 * Followed by this, you should have types defined for all the intermediate
 * rules that you will define. These are some of the intermediate rules:
 */
%type<i> allowed_char_for_token
%type<i> allowed_char_for_text
%type<str> ows
%type<str> token
%type<str> text


%type<str> HTTP_Version
%type<str> message_header
%type<str> message_headers
%type<str> field_value
%type<str> request_line
%type<str> request_uri
%type<str> absoluteURI
%type<str> abs_path
%type<str> http_url_port
%type<str> http_url_abs_path
%type<str> http_url_abs_path_query
%type<str> op_backslash
%type<str> uric
%%

/*
** The following 2 rules define a token.
*/

/*
 * Rule 1: Allowed characters in a token
 *
 * An excerpt from RFC 2616:
 * --
 * token          = 1*<any CHAR except CTLs or separators>
 * --
 */
allowed_char_for_token:
t_token_char; |
t_digit {
	$$ = '0' + $1;
}; |
t_dot;

/*
 * Rule 2: A token is a sequence of all allowed token chars.
 */
token:
allowed_char_for_token {
	YPRINTF("token: Matched rule 1.\n");
	snprintf($$, 8192, "%c", $1);
}; |
token allowed_char_for_token {
	YPRINTF("token: Matched rule 2.\n");
	snprintf($$, 8192, "%s%c", $1, $2);
};

/*
** The following 2 rules define text.
*/
/*
 *
 * Rule 3: Allowed characters in text
 *
 * An excerpt from RFC 2616, section 2.2:
 * --
 * The TEXT rule is only used for descriptive field contents and values
 * that are not intended to be interpreted by the message parser. Words
 * of *TEXT MAY contain characters from character sets other than ISO-
 * 8859-1 [22] only when encoded according to the rules of RFC 2047
 * [14].
 *
 * TEXT = <any OCTET except CTLs, but including LWS>
 * --
 *
 */

allowed_char_for_text:
allowed_char_for_token; |
t_separators {
	$$ = $1;
}; |
t_sp {
	$$ = $1;
}; |
t_ws {
	$$ = ' ';
}; |
t_colon {
	$$ = $1;
}; |
t_backslash {
	$$ = $1;
};

/*
 * Rule 4: Text is a sequence of characters allowed in text as per RFC. May
 * 	   also contains spaces.
 */
text: allowed_char_for_text {
	YPRINTF("text: Matched rule 1.\n");
	snprintf($$, 8192, "%c", $1);
}; |
text ows allowed_char_for_text {
	YPRINTF("text: Matched rule 2.\n");
	snprintf($$, 8192, "%s%s%c", $1, $2, $3);
};

/*
 * Rule 5: Optional white spaces
 */
ows: {
	YPRINTF("OWS: Matched rule 1\n");
	snprintf($$, 8192, "");
}; |
t_sp {
	YPRINTF("OWS: Matched rule 2\n");
	snprintf($$, 8192, "%c", $1);
}; |
t_ws {
	YPRINTF("OWS: Matched rule 3\n");
	snprintf($$, 8192, "%s", $1);
};

/*
 * You need to fill this rule, and you are done! You have all the assembly
 * needed. You may wish to define your own rules. Please read RFC 2616
 * and the annotated excerpted text on the course website. All the best!
 *
 * The bogus rule that we have now, matches any text! Remove it and fill it
 * with your own rule.
 */
request: request_line message_headers t_crlf{
	YPRINTF("request: Matched rule 1\n");
	return SUCCESS;
}; 

/* HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT */
HTTP_Version: token t_backslash t_digit t_dot t_digit {	
	YPRINTF("HTTP_Version: Matched rule 1\n");
	snprintf($$, 8192, "%s%c%c%c%c", $1, $2, $3, $4, $5);

	requestResult->requestLine.httpVersion.http = deepCopyString($1);
	requestResult->requestLine.httpVersion.version[0] = $3;
	requestResult->requestLine.httpVersion.version[1] = $5;
};

/* message-header = field-name ":" [ field-value ] */
/* field-name     = token */
message_header: token t_colon field_value {
	YPRINTF("message_header: Matched rule 1\n");
	snprintf($$, 8192, "%s%c%s", $1, $2, $3);
	struct MessageHeader* newHeader = malloc(sizeof(struct MessageHeader));
	newHeader->fieldName = deepCopyString($1);

	char* start = $3;
	while(*start == ' ')
		start++;
	
	newHeader->fieldValue = deepCopyString(start);
	newHeader->next = requestResult->headers;
	requestResult->headers = newHeader;
};


message_headers: message_header t_crlf {
	YPRINTF("message_headers: Matched rule 1\n");
}; |
message_headers message_header t_crlf{
	YPRINTF("message_headers: Matched rule 2\n");
};


/* field-value    = *( field-content | LWS ) */
/* field-content  = *TEXT 	*/
field_value: text {
	YPRINTF("field_value: Matched rule 1\n");
	snprintf($$, 8192, "%s", $1);
}; |
t_lws {
	YPRINTF("field_value: Matched rule 2\n");
	snprintf($$, 8192, "%c", ' ');
}; |
field_value text {
	YPRINTF("field_value: Matched rule 3\n");
	snprintf($$, 8192, "%s%s", $1, $2);
}; |
field_value t_lws {
	YPRINTF("field_value: Matched rule 4\n");
	snprintf($$, 8192, "%s%c", $1, ' ');
}; |
 {
 	YPRINTF("field_value: Matched rule 5\n");
	snprintf($$, 8192, "");
};

/* Request-Line   = Method SP Request-URI SP HTTP-Version CRLF */
request_line: token t_sp request_uri t_sp HTTP_Version t_crlf {
	YPRINTF("request_line: Matched rule 1\n");
	snprintf($$, 8192, "%s%c%s%c%s%s", $1, $2, $3, $4, $5, $6);
	requestResult->requestLine.method = deepCopyString($1);
};


/* Request-URI    = "*" | absoluteURI | abs_path | authority */
request_uri: '*' {
	YPRINTF("request_uri: Matched rule 1\n");
	snprintf($$, 8192, "%s", "*");
	requestResult->requestLine.requestURI.type = 1;
	requestResult->requestLine.requestURI.absPath = "*";
}; |
absoluteURI op_backslash{
	YPRINTF("request_uri: Matched rule 2\n");
	snprintf($$, 8192, "%s", $1);
	requestResult->requestLine.requestURI.type = 2;
}; |
//abs_path op_backslash {
http_url_abs_path{
	YPRINTF("request_uri: Matched rule 3\n");
	snprintf($$, 8192, "%s", $1);
	requestResult->requestLine.requestURI.type = 3;
	//requestResult->requestLine.requestURI.absPath = deepCopyString($1);
}; |
t_backslash{
	YPRINTF("request_uri: Matched rule 4\n");
	requestResult->requestLine.requestURI.absPath = NULL;
};

/* http_URL = "http:" "//" host [ ":" port ] [ abs_path [ "?" query ]] */
absoluteURI: token t_colon t_backslash t_backslash token http_url_port http_url_abs_path {
	YPRINTF("absoluteURI: Matched rule 1\n");
	snprintf($$, 8192, "%s%c%c%c%s%s%s", $1, $2, $3, $4, $5, $6, $7);
	
	if (strcmp($6, "") != 0)
		requestResult->requestLine.requestURI.port = atoi($6);
	else
		requestResult->requestLine.requestURI.port = -1;

	requestResult->requestLine.requestURI.host = deepCopyString($5);
};

/* http url pprt = : port */
http_url_port: t_colon t_digit {
	YPRINTF("http_url_port: Matched rule 1\n");
	snprintf($$, 8192, "%d", $2);	
}; |
http_url_port t_digit {
	YPRINTF("http_url_port: Matched rule 2\n");
	snprintf($$, 8192, "%s%d", $1, $2);
}; |
{
	YPRINTF("http_url_port: Matched rule 3\n");
	snprintf($$, 8192, "");
};

http_url_abs_path : op_backslash{
	YPRINTF("http_url_abs_path: Matched rule 1\n");
	snprintf($$, 8192, "%s", $1);
	requestResult->requestLine.requestURI.absPath = NULL;
	requestResult->requestLine.requestURI.query = NULL;
}; |
abs_path op_backslash http_url_abs_path_query{
	YPRINTF("http_url_abs_path: Matched rule 2\n");
	snprintf($$, 8192, "%s%c%s", $1, '/', $3);
	requestResult->requestLine.requestURI.absPath = deepCopyString($1);
	requestResult->requestLine.requestURI.query = deepCopyString($3);
};

/* query-string = *uric */
/* uric         = reserved | unreserved | escaped */
uric : allowed_char_for_token {
	YPRINTF("uric: Matched rule 1\n");
	snprintf($$, 8192, "%c", $1);
}; |
t_separators {
	YPRINTF("uric: Matched rule 2\n");
	snprintf($$, 8192, "%c", $1);
}; |
uric allowed_char_for_token{
	YPRINTF("uric: Matched rule 4\n");
	snprintf($$, 8192, "%s%c", $1, $2);
}; |
uric t_separators{
	YPRINTF("uric: Matched rule 5\n");
	snprintf($$, 8192, "%s%c", $1, $2);
};

/* http_url_abs_path_query = query-string */
/* query-string = *uric */
http_url_abs_path_query: uric {
	YPRINTF("http_url_abs_path_query: Matched rule 1\n");
	snprintf($$, 8192, "%s", $1);
}; |
{
	YPRINTF("http_url_abs_path_query: Matched rule 3\n");
	snprintf($$, 8192, "");
}

/*  abs_path == /pub/WWW/TheProject.html */
abs_path: t_backslash token{
	YPRINTF("abs_path: Matched rule 1\n");
	snprintf($$, 8192, "%c%s", $1, $2);
}; |
abs_path t_backslash token {
	YPRINTF("abs_path: Matched rule 2\n");
	snprintf($$, 8192, "%s%c%s", $1, '/', $3);
};

/* optional backslash */
op_backslash: t_backslash {
	YPRINTF("op_backslash: Matched rule 1\n");
	snprintf($$, 8192, "%c", '/');
}; |
{
	YPRINTF("op_backslash: Matched rule 2\n");
	snprintf($$, 8192, "");
};

%%

/* C code */

void set_parsing_buf(char *buf, size_t siz)
{
	parsing_buf = buf;
	parsing_offset = 0;
	parsing_buf_siz = siz;
}

/*
int main(int argc, char *argv[])
{
	int fd_in;

	enum {
		STATE_START = 0,
		STATE_CR,
		STATE_CRLF,
		STATE_CRLFCR,
		STATE_CRLFCRLF
	};

	int i, state;
	char buf[8192];
	size_t offset = 0;
	char ch;

	fd_in = open(argv[1], O_RDONLY);

	if(fd_in < 0) {
		printf("Failed to open the file\n");
		return 0;
	}

	state = STATE_START;
	while(state != STATE_CRLFCRLF) {
		char expected = 0;

		if(read(fd_in, &ch, 1) != 1)
			break;

		buf[offset++] = ch;
		switch(state) {
		case STATE_START:
		case STATE_CRLF:
			expected = '\r';
			break;
		case STATE_CR:
		case STATE_CRLFCR:
			expected = '\n';
			break;
		default:
			state = STATE_START;
			continue;
		}

		if(ch == expected)
			state++;
		else
			state = STATE_START;

	}

	requestResult.headers = NULL;

	if(state == STATE_CRLFCRLF) {
		printf("Found CRLF-CRLF\n");
		set_parsing_buf(buf, offset);
		printf("yyparse returned %d\n", yyparse());
	} else {
		printf("Failed: Could not find CRLF-CRLF\n");
	}

	
	
	printf("%s\n", requestResult.requestLine.httpVersion.http);
	printf("%s\n", requestResult.requestLine.method);
	
	printf("%s\n", requestResult.requestLine.requestURI.absPath);
	printf("%s\n", requestResult.requestLine.requestURI.host);
	printf("%s\n", requestResult.requestLine.requestURI.query);
	printf("%d\n", requestResult.requestLine.requestURI.port);

	printf("%d%d\n", requestResult.requestLine.httpVersion.version[0],
						requestResult.requestLine.httpVersion.version[1]);
	
	struct MessageHeader* p = requestResult.headers;
	while (p) {
		printf("%s %s\n", p->fieldName, p->fieldValue);
		p = p->next;
	}
	
	
	close(fd_in);

	return 0;
}
*/

void yyerror (char *s) {fprintf (stderr, "%s\n", s);}
