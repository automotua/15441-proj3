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

static void show_usage(){

}

void px_init(px_config_t * config, int argc, char **argv) {
  bzero(config, sizeof(px_config_t));

  config->argc = argc;
  config->argv = argv;

}

int px_parse_command_line(px_config_t * config) {
    if (config->argc != 7 || config->argc != 8){
        show_usage();
        exit(-1;)
    }

    char** argv = config->argv;

    strcpy(config->log_file, argv[1]);
  
    config->alpha = (float) atof(argv[2]);
  
    cofing->listen_port = atoi(argv[3]);

    inet_aton(argv[4], &config->fake_ip_s_addr);

    inet_aton(argv[5], &config->dns_ip_s_addr);

    cofing->dns_port = atoi(argv[6]);

    if (config->argc == 8)
        inet_aton(argv[7], &config->www_ip_s_addr);
    else
        config->www_ip_s_addr = -1;
}
