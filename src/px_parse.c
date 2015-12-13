/*
 * px_parse.c
 *
 *
 * Authors:
 *
 * Date:
 *
 * Description:
 *
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "px_parse.h"
#include "mydns.h"

static void show_usage(){

}

void px_init(px_config_t * config, int argc, char **argv) {
  bzero(config, sizeof(px_config_t));

  config->argc = argc;
  config->argv = argv;

}

void px_parse_command_line(px_config_t * config) {
    if (config->argc != 7  && config->argc != 8){
        show_usage();
        exit(-1);
    }

    char** argv = config->argv;

    strcpy(config->log_file, argv[1]);
  
    config->alpha = (float) atof(argv[2]);
  
    config->listen_port = atoi(argv[3]);

    inet_aton(argv[4], &config->fake_ip_in_addr);

    inet_aton(argv[5], &config->dns_ip_in_addr);

    config->dns_port = atoi(argv[6]);

    if (config->argc == 8)
        inet_aton(argv[7], &config->www_ip_in_addr);
    else {
        config->www_ip_in_addr.s_addr = -1;
        init_mydns(argv[5], config->dns_port, argv[4]);
    }
}
