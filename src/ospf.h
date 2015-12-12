#define MAX_NODE_LEN 256
#define BUF_LEN 8192

typedef struct neighbor_s{
	void* node;
	struct neighbor_s* next;	
} neighbor_t;

typedef struct node_s{
	char id[MAX_NODE_LEN];
	int is_server;
	int seq_num;
	neighbor_t* neighbors;
	struct node_s* next;

	int distance;
	int is_finish;
} node_t;


int init_ospf(char* lsa_file_path);

int mark_server(char* server_file_path);

int find_closest_server(int is_robin, char* node_id, char* result_id);

node_t * find_node(char* id);

node_t* new_node(char* id, int seq_num);

void release_neighbors(neighbor_t* p);

int round_robin(char* result_id);