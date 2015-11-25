void parse_browser_request(px_config_t * config, browser_conn_t * b_conn);
void clean_bconn_after_parse(browser_conn_t * b_conn);
void parse_server_response(px_config_t * config, server_conn_t * s_conn);
void clean_sconn_after_parse(server_conn_t * s_conn);
