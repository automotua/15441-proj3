void parse_browser_request(px_config_t * config, browser_conn_t * b_conn) {
    // TODO parse the browser request
    // modify at least the following attributes of b_conn:
    // b_conn->req_type
    // b_conn->is_parse_done
    // b_conn->url

}


void clean_bconn_after_parse(browser_conn_t * b_conn) {

}

void parse_server_response(px_config_t * config, server_conn_t * s_conn) {
    // TODO parse the server response
    // modify at least the following attributes of s_conn:
    // s_conn->resp_type
    // s_conn->is_parse_done
    // s_conn->file_data

}

void clean_sconn_after_parse(server_conn_t * s_conn) {

}

