
char* generate_request_to_server(browser_conn_t * b_conn, char* request_file){
    char *request = malloc(MAX_HEADER_LENGTH);

    // generate the first line
    strcpy(request, "GET ");
    strcat(request, request_file);
    strcat(request, " HTTP/1.1\r\n");

    struct MessageHeader* header = b_conn->request->headers;
    // add header
    while (header){
        // ignore host header
        if (strcmp(header->fieldName, "Host") == 0 ||
            strcmp(header->fieldName, "host") == 0) {
            header = head->next;
            continue;
        }
        strcat(request, header->fieldName);
        strcat(request, ":");
        strcat(request, header->fieldValue);
        strcat(request, "\r\n");
        header = header->next;
    }
    
    strcat(request, "\r\n");

    return request;
}

int send_data_to_socket(int sock, char* buf, int size){
    /* send data to browser, may need send many times */
    int writeret;
    while (size > 0) {
        writeret = send(sock, buf, size, 0);
        if (writeret <= 0)
        {
            /* Interrupted by signal, will send() again */
            if (errno == EINTR) 
                writeret = 0;
            else {
                /* send data error */
                return -1;
            }
        }
        size -= writeret;
        buf += writeret;
    }
    return 0;
}