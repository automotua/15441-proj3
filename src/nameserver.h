#define NS_FILENAME_LEN 256

typedef struct ns_config_s {
	int is_robin;
	char log_file_path[NS_FILENAME_LEN];
	struct in_addr ip_in_addr;
	int port;
	char servers_file_path[NS_FILENAME_LEN];
	char LSAs_file_path[NS_FILENAME_LEN];
} ns_config_t;