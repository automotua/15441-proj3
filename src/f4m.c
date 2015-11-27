#include <expat.h>

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

static void XMLCALL
endElement(void *userData, const char *name){
    return;
}

void process_f4m_request(px_config_t * config, px_conn_t * px_conn) {

}

void process_f4m_response(px_config_t * config, px_conn_t * px_conn) {
    server_conn_t * s_conn = px_conn->s_conn;
    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, s_conn);
    XML_SetElementHandler(parser, startElement, endElement);
    if (XML_Parse(parser, s_conn->file_data, 
                  s_conn->cur_size, 1) == XML_STATUS_ERROR) {
      fprintf(stderr,
              "%s at line %" XML_FMT_INT_MOD "u\n",
              XML_ErrorString(XML_GetErrorCode(parser)),
              XML_GetCurrentLineNumber(parser));
      return 1;
    }
    XML_ParserFree(parser);
}


void process_nolist_f4m_response(px_config_t * config, px_conn_t * px_conn) {

}