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


void px_init(px_config_t * config, int argc, char **argv) {
  bzero(config, sizeof(px_config_t));

  config->argc = argc;
  config->argv = argv;
}

void px_parse_command_line(px_config_t * config) {

}
