#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ospf.h"

static node_t nodes;
static char** servers;
static int server_num, robin_index;

int init_ospf(char* lsa_file_path) {
    nodes.next = NULL;

    char buf[BUF_LEN];
    char id[MAX_NODE_LEN], neighbor_id[MAX_NODE_LEN];

    FILE* file = fopen(lsa_file_path, "r");
    if (!file) {
        fprintf(stderr, "no lsa file\n");
        return -1;
    }

    while(fgets(buf, BUF_LEN, file)) {
        if (buf[strlen(buf)-1] == '\n')
            buf[strlen(buf)-1] = '\0';

        char* space = strchr(buf, ' ');
        if (!space){
            fprintf(stderr, "invalid format\n");
            return -1;
        }
        
        *space = '\0';
        strcpy(id, buf);

        char* space2 = strchr(space + 1, ' ');
        if (!space2){
            fprintf(stderr, "invalid format\n");
            return -1;
        }

        *space2 = '\0';
        int seq_num = atoi(space + 1);

        node_t* cur_node = find_node(id);
        if (!cur_node)  {
            cur_node = new_node(id, seq_num);
        }

        if (cur_node->seq_num > seq_num)
            continue;
        cur_node->seq_num = seq_num;

        // realase old neighbors
        release_neighbors(cur_node->neighbors);
        cur_node->neighbors = NULL;

        space2 += 1;
        space2[strlen(space2) + 1] = '\0';
        space2[strlen(space2)] = ',';

        char* comma = strchr(space2, ',');
        while (comma) {
            *comma = '\0';
            strcpy(neighbor_id, space2);
            
            node_t* neighbor_node = find_node(neighbor_id);
            if (!neighbor_node){
                neighbor_node = new_node(neighbor_id, -1);
            }
            
            neighbor_t* neighbor = malloc(sizeof(neighbor_t));
            neighbor->node = (void*) neighbor_node;
            neighbor->next = cur_node->neighbors;
            cur_node->neighbors = neighbor;

            space2 = comma + 1;
            comma = strchr(space2, ',');
        }

    }

    return 0;
}

int mark_server(char* server_file_path) {
    FILE* file = fopen(server_file_path, "r");
    if (!file) {
        fprintf(stderr, "no server file\n");
        return -1;
    }

    server_num = 0;

    char buf[BUF_LEN];
    while(fgets(buf, BUF_LEN, file)) {
        if (buf[strlen(buf)-1] == '\n')
            buf[strlen(buf)-1] = '\0';
        
        node_t* node = find_node(buf);
        if (!node) {
            fprintf(stderr, "can not find server: %s\n", buf);
            return -1;
        }
        node->is_server = 1;
        server_num++;
    }

    fclose(file);

    // for round robin
    servers = malloc(sizeof(char*) * server_num);
    server_num = 0;
    file = fopen(server_file_path, "r");
    while(fgets(buf, BUF_LEN, file)) {
        if (buf[strlen(buf)-1] == '\n')
            buf[strlen(buf)-1] = '\0';
        servers[server_num] = malloc(strlen(buf) + 1);
        strcpy(servers[server_num++], buf);
    }
    fclose(file);

    robin_index = 0;

    return 0;
}

int find_closest_server(int is_robin, char* node_id, char* result_id) {
    if (is_robin)
        return round_robin(result_id);

    node_t* cur_node = find_node(node_id);
    if (!cur_node) {
        fprintf(stderr, "can not find node: %s\n", node_id);
        return -1;
    }

    // init distance
    node_t* tmp = nodes.next;
    while (tmp) {
        tmp->distance = -1;
        tmp->is_finish = 0;
        tmp = tmp->next;
    }
    cur_node->distance = 0;

    // begin dijkstra
    while (cur_node && !cur_node->is_server) {
        // update distance
        neighbor_t* tmp_neighbor = cur_node->neighbors;
        while(tmp_neighbor) {
            if ((((node_t*)tmp_neighbor->node)->distance == -1) || 
                (((node_t*)tmp_neighbor->node)->distance > cur_node->distance + 1))
                ((node_t*)tmp_neighbor->node)->distance = cur_node->distance + 1;
            tmp_neighbor = tmp_neighbor->next;
        }

        // mark current node as finish
        cur_node->is_finish = 1;

        // find the next cloest node
        cur_node = NULL;
        tmp = nodes.next;
        while(tmp) {
            if (!tmp->is_finish){
                if  ((tmp->distance != -1) && 
                     (!cur_node || cur_node->distance > tmp->distance))
                    cur_node = tmp;
            }
            tmp = tmp->next;
        }
    }

    if (!cur_node){
        fprintf(stderr, "can not find any closest server\n");
        return -1;
    }

    strcpy(result_id, cur_node->id);

    return 0;
}

node_t * find_node(char* id) {
    node_t* p = nodes.next;
    while (p) {
        if (strcmp(p->id, id) == 0)
            return p;
        p = p->next;
    }
    return NULL;
}


node_t* new_node(char* id, int seq_num) {
    node_t* result = malloc(sizeof(node_t));
    strcpy(result->id, id);
    result->is_server = 0;
    result->seq_num = seq_num;
    result->neighbors = NULL;
    result->next = nodes.next;
    nodes.next = result;

    return result;
}

void release_neighbors(neighbor_t* p) {
    neighbor_t* tmp;
    while (p) {
        tmp = p;
        p = p->next;
        free(tmp);
    }
}

int round_robin(char* result_id) {
    strcpy(result_id, servers[robin_index]);
    robin_index = (robin_index + 1) % server_num;
    return 0;
}

