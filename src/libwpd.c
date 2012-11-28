#include <stdlib.h>
#include <stdio.h>
#include <libwpd.h>
#include <wp_daemonizer.h>

int main(int argc, char* argv[]) {
  /* TODO: Utilize arguments. */

  wp_status_t status = WP_FAILURE;
  wp_configuration_t *config = NULL;
  wp_daemonizer_t *daemon = NULL;

  if((status = wp_configuration_new(&config)) == WP_SUCCESS) {
    config->populate_from_file(config);
    config->populate_from_args(config, argc, argv);

    if(config->get_print_arguments(config) || config->get_print_config_options(config)) {
      config->configuration_print(config);
    }
  } else {
    /* TODO: Print out some help. */
    wp_configuration_delete(&config);
  }
  if(config && (status = wp_daemonizer_initialize(&daemon, config)) == WP_SUCCESS) {;
    /* Daemonize ourselves:
     * fork; setsid; reset file mask; cd; reopen standard files
     */
    atexit(&(*daemon->shutdown));
    if((status = daemon->daemonize(daemon)) == WP_SUCCESS) {
      /* Assuming we successfully daemonize, here we start. */  
      status = daemon->start(daemon);
    }
  }

  return status;
}
