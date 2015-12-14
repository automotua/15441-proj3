/*
 * f4m.c
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *          Junqiang Li <junqiangl@andrew.cmu.edu>
 *
 * Date: 12-13-2015
 *
 * Description: functions to process HTML/f4m/nolist.f4m reqeust from browser 
 *              and HTML/f4m/nolist.f4m response from server. It also contains 
 *              function of parse f4m XML file.  
 */

#include <expat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f4m.h"
#include "helper.h"
#include "px_parse.h"
#include "parser.h"

#if defined(__amigaos__) && defined(__USE_INLINE__)
#include <proto/expat.h>
#endif

#ifdef XML_LARGE_SIZE
#if defined(XML_USE_MSC_EXTENSIONS) && _MSC_VER < 1400
#define XML_FMT_INT_MOD "I64"
#else
#define XML_FMT_INT_MOD "ll"
#endif
#else
#define XML_FMT_INT_MOD "l"
#endif

// used for XML file parse
static void XMLCALL
startElement(void *userData, const char *name, const char **atts)
{
  server_conn_t * s_conn= (server_conn_t *)userData;

  if(strcmp(name, "media") == 0){
    const char** p = atts;
    while(*p){
        if (strcmp(*p, "bitrate") == 0){
            p++;
            bitrate_t * tmp = malloc(sizeof(bitrate_t));
            tmp->bitrate = atoi(*p);
            tmp->next = s_conn->bitrates;
            s_conn->bitrates = tmp;
            break;
        }
        p++;
    }
  }
  
}

// used for XML file parse
static void XMLCALL
endElement(void *userData, const char *name){
    return;
}

int process_html_request(px_config_t * config, px_conn_t * px_conn) {
  browser_conn_t* b_conn = px_conn->b_conn;
  server_conn_t * s_conn = px_conn->s_conn;
  
  // if not connect with server, then build connection
  if (!s_conn) {
    init_server_connection(config, px_conn);
    s_conn = px_conn->s_conn;
  }

  // expect to receive HTML response from server
  s_conn->resp_type = HTML_RESP;
  
  char* request = generate_request_to_server(b_conn, b_conn->url, 
                                                            "localhost:8080");

  if (send_data_to_socket(s_conn->fd, request, strlen(request)) < 0){
    free(request);
    return -1;
  }

  free(request);

  return 0;
}

int process_html_response(px_config_t * config, px_conn_t * px_conn) {
    server_conn_t * s_conn = px_conn->s_conn;
    browser_conn_t* b_conn = px_conn->b_conn;
    
    if (send_data_to_socket(b_conn->fd, s_conn->file_data,s_conn->cur_size) < 0)
        return -1;

    return 0;
}

int process_f4m_request(px_config_t * config, px_conn_t * px_conn) {
  browser_conn_t* b_conn = px_conn->b_conn;
  server_conn_t* s_conn = px_conn->s_conn;

  if (!s_conn) {
    init_server_connection(config, px_conn);
    s_conn = px_conn->s_conn;
  }

  s_conn->resp_type = F4M_RESP;

  char* request = generate_request_to_server(b_conn, b_conn->url, 
                                                            "localhost:8080");

  //logmessage("Send f4m request to server\n", request, strlen(request));

  if (send_data_to_socket(s_conn->fd, request, strlen(request)) < 0){
    free(request);
    return -1;
  }

  free(request);

  /* also generate nolist.f4m request, and save it in proxy memory so that when
   * proxy get the response of f4m from server, it can send nolist.f4m reqeust
   */
  char nolist_url[MAX_URL_LENGTH];
  strcpy(nolist_url, b_conn->url);
  char* dot = strrchr(nolist_url, '.');
  *dot = '\0';
  strcat(nolist_url, "_nolist.f4m");

  b_conn->tmp_nolist_request = generate_request_to_server(b_conn, nolist_url, 
                                                              "localhost:8080");

  char* slash = strrchr(b_conn->url, '/');
  strncpy(b_conn->video_path, b_conn->url, slash - b_conn->url + 1);
  b_conn->video_path[slash - b_conn->url + 1] = '\0';

  return 0;
}

int process_f4m_response(px_config_t * config, px_conn_t * px_conn) {
    server_conn_t * s_conn = px_conn->s_conn;
    browser_conn_t* b_conn = px_conn->b_conn;

    // parse XML file
    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, s_conn);
    XML_SetElementHandler(parser, startElement, endElement);
    if (XML_Parse(parser, s_conn->file_data, 
                  s_conn->cur_size, 1) == XML_STATUS_ERROR) {
      fprintf(stderr,
              "%s at line %" XML_FMT_INT_MOD "u\n",
              XML_ErrorString(XML_GetErrorCode(parser)),
              XML_GetCurrentLineNumber(parser));
      return -1;
    }
    XML_ParserFree(parser);

    // set initial throughput
    bitrate_t* p = s_conn->bitrates;
    if (!p)
      return -1;
    px_conn->throughput = p->bitrate;
    while(p) {
      if (px_conn->throughput > p->bitrate)
        px_conn->throughput = p->bitrate;
      p = p->next;
    }

    save_history_bitrates(config, px_conn);

    s_conn->resp_type = NOLIST_F4M_RESP;

    // send nolist.f4m request
    if (send_data_to_socket(s_conn->fd, b_conn->tmp_nolist_request, 
                                      strlen(b_conn->tmp_nolist_request)) < 0){
      free(b_conn->tmp_nolist_request);
      return -1;
    }

    free(b_conn->tmp_nolist_request);

    return 0;
}


int process_nolist_f4m_response(px_config_t * config, px_conn_t * px_conn) {
  server_conn_t * s_conn = px_conn->s_conn;
  browser_conn_t* b_conn = px_conn->b_conn;

  if (send_data_to_socket(b_conn->fd, s_conn->file_data, s_conn->cur_size) < 0)
    return -1;
  return 0;
}