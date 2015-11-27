extern struct Request* requestResult;
extern FILE* yyin;

void parse_browser_request(px_config_t * config, browser_conn_t * b_conn) {
    // TODO parse the browser request
    // modify at least the following attributes of b_conn:
    // b_conn->req_type
    // b_conn->is_parse_done
    // b_conn->url

    /* Assume:   1. A HTTP request always smaller than 8192 bytes
     *           2. All HTTP requests have no body (only GET request)
     *           3. Ignore invalid HTTP request without any response 
     */
    
    char buf;
    int readret = 0;
    
    // check CRLFCRLF state
    char expected = 0;
    int state = b_conn->state;
    while (state != STATE_CRLFCRLF) {
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

        // receive data from browser
        readret = recv(b_conn->fd, buf, 1, MSG_DONTWAIT);

        if (readret < 0){
            if (errno == EAGAIN || errno == EWOULDBLOCK){
                // finish recving, has not found CRLFCRLF
                b_conn->state = state;
                b_conn->is_parse_done = 0;          
            } else {
                // detect error, close connection
                b_conn->is_parse_done = -1;
            }
            return;
        }

        if (buf == expected)
            state++;
        else
            state = STATE_START;

        // if request size is greater than max size, close connection
        if (b_conn->bufferSize == MAX_HEADER_LENGTH){
            b_conn->is_parse_done = -1;
            return;
        }

        b_conn->buffer[b_conn->bufferSize++] = buf;
    }

    // has found CRLFCRLF, parse request 

    // create a new request
    b_conn->request = malloc(sizeof(struct Request));
    requestResult = b_conn->request;
    requestResult->headers = NULL;
    requestResult->requestLine.method = NULL;
    requestResult->requestLine.requestURI.absPath = NULL;
    requestResult->requestLine.requestURI.host = NULL;
    requestResult->requestLine.requestURI.query = NULL;
    requestResult->requestLine.httpVersion.http = NULL;    

    // set buffer for yacc
    set_parsing_buf(b_conn->buffer, b_conn->bufferSize);

    yyrestart(yyin);
    // parse request
    if (yyparse() == 1){
        // parse syntax error, close connection
        b_conn->is_parse_done = -1;
        return;
    }

    /* don't know why, but must call yyrestart() to ensure the next call of 
     * yyparse() could work correctly.
     */
    yyrestart(yyin);

    // check the length of url
    if (requestResult->requestLine.requestURI.absPath == NULL ||
        strlen(requestResult->requestLine.requestURI.absPath) >= MAX_URL_LENGTH){
        b_conn->is_parse_done = -1;
        return;
    }

    // copy absPath
    strcpy(b_conn->url, requestResult->requestLine.requestURI.absPath);

    // check type
    char* dot = strrchr(b_conn->url, '.');
    if (dot != NULL && strcmp(dot+1, "f4m") == 0)
        b_conn->req_type = F4M_REQ;
    else
        b_conn->req_type = CHUNK_REQ;

    b_conn->is_parse_done = 1;
}


void clean_bconn_after_parse(browser_conn_t * b_conn) {
    b_conn->bufferSize = 0;
    b_conn->state = STATE_START;

    if (b_conn->request) {
        if (b_conn->request->requestLine.method) 
            free(b_conn->request->requestLine.method);
        
        if (b_conn->request->requestLine.requestURI.absPath)
            free(b_conn->request->requestLine.requestURI.absPath);

        if (b_conn->request->requestLine.requestURI.host)
            free(b_conn->request->requestLine.requestURI.host);

        if (b_conn->request->requestLine.requestURI.query)
            free(b_conn->request->requestLine.requestURI.query);

        if (b_conn->request->requestLine.httpVersion.http)
            free(b_conn->request->requestLine.httpVersion.http);

        struct MessageHeader* p = b_conn->request->headers;
        struct MessageHeader* temp;
        while (p) {
            temp = p;
            p = p->next;
            free(temp->fieldName);
            free(temp->fieldValue);
            free(temp);    
        }
        free(b_conn->request);
        b_conn->request = NULL;
    } 
}

void parse_server_response(px_config_t * config, server_conn_t * s_conn) {
    // TODO parse the server response
    // modify at least the following attributes of s_conn:
    // s_conn->resp_type
    // s_conn->is_parse_done
    // s_conn->file_data

    /* Assume:   1. A HTTP response header always smaller than 8192 bytes
     *           2. No invalid HTTP response
     */

    char buf;
    int readret = 0;

    // check if receiving response body (content)
    while (s_conn->content_length >= 0) {

        if (s_conn->content_length == 0){
            s_conn->is_parse_done = 1;
            return;
        }

        // receive data from server
        readret = recv(s_conn->fd, buf, 1, MSG_DONTWAIT);

        if (readret < 0){
            if (errno == EAGAIN || errno == EWOULDBLOCK){
                // finish recving, has not finish receiving body
                s_conn->is_parse_done = 0;          
            } else {
                // detect error, close connection
                s_conn->is_parse_done = -1;
            }
            return;
        }

        s_conn->file_data[s_conn->cur_size++] = buf;
        s_conn->content_length--;
    }

    // a new response, check CRLFCRLF state
    char expected = 0;
    int state = s_conn->state;
    while (state != STATE_CRLFCRLF) {
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

        // receive data from server
        readret = recv(s_conn->fd, buf, 1, MSG_DONTWAIT);

        if (readret < 0){
            if (errno == EAGAIN || errno == EWOULDBLOCK){
                // finish recving, has not found CRLFCRLF
                s_conn->state = state;
                s_conn->is_parse_done = 0;          
            } else {
                // detect error, close connection
                s_conn->is_parse_done = -1;
            }
            return;
        }

        if (buf == expected)
            state++;
        else
            state = STATE_START;

        // if request size is greater than max size, close connection
        if (s_conn->bufferSize == MAX_HEADER_LENGTH){
            s_conn->is_parse_done = -1;
            return;
        }

        s_conn->buffer[s_conn->bufferSize++] = buf;
    }

    // has found CRLFCRLF, parse response
    char one_line[MAX_HEADER_LENGTH]
    char* line_begin = s_conn->buffer;
    char* line_end = strstr(line_begin, "\r\n");
    while (line_end != line_begin){
        strncpy(one_line, line_begin, line_end - line_begin);
        if ((s_conn->content_length = parse_content_length(one_line)) >= 0)
            break;
        line_begin = line_end + 2;
        line_end = strstr(line_begin, "\r\n");
    }

    if (s_conn->content_length < 0){
        // finish parsing without content-length, detect error
        s_conn->is_parse_done = -1;
        return;
    }

    // finish parsing response head, will parse body (content) in the next time
    s_conn->is_parse_done = 0;
    if (s_conn->resp_type == F4M_RESP){
        // f4m file, only need body
        s_conn->file_data = malloc(s_conn->content_length);
        s_conn->cur_size = 0;
    } else {
        // f4m nolist or chunk data, need HTTP header + body
        s_conn->file_data = malloc(s_conn->bufferSize + s_conn->content_length);
        strcpy(s_conn->file_data, s_conn->buffer);
        s_conn->cur_size = s_conn->bufferSize;
    }
}

void clean_sconn_after_parse(server_conn_t * s_conn) {
    s_conn->state = STATE_START;
    s_conn->bufferSize = 0;
    if (s_conn->file_data){
        free(s_conn->file_data);
        s_conn->file_data = NULL;
    }
    s_conn->content_length = -1;
    
    if (s_conn->resp_type == F4M_RESP){
        // next time, expect NOLIST_F4M_RESP
        s_conn->resp_type = NOLIST_F4M_RESP;
    } else if (s_conn->resp_type == NOLIST_F4M_RESP){
        // next time, expect CHUNK_RESP
        s_conn->resp_type = CHUNK_RESP;
    } else {
        // next time, expect CHUNK_RESP
        s_conn->resp_type = CHUNK_RESP;
    }
}

int parse_content_length(char* line){
    
    char* colon = strchr(line, ':');

    if (colon == NULL)
        return -1;

    char* key_begin = line;
    char* key_end = colon - 1;
    char* value_begin - colon + 1;

    // trim
    while ((key_begin == ' ' || key_begin == '\t') && (key_begin != key_end))
        key_begin++
    while ((key_end == ' ' || key_end == '\t') && (key_begin != key_end))
        key_end--;

    // convert to lower case
    char* tmp;
    for (tmp = key_begin; tmp <= key_end; tmp++)
        tolower(*tmp);

    // check if it is content_length
    if (strncmp(key_begin, "content-length", key_end - key_begin + 1) != 0)
        return -1;

    // get length
    return atoi(value_begin);
}
