#include <stdlib.h>
#include <stdio.h>

#include <libwpd.h>

static void reconfigure_daemon(const struct wp_daemonizer *daemon, const wp_configuration_pt config) {
  daemon = daemon;
  config->set_enable_verbose_logging(config, true);
}

int main(int argc, char* argv[]) {
  wp_status_t status = WP_FAILURE;
  wp_daemonizer_pt daemon = NULL;
  
  if((status = wp_daemonizer_initialize(&daemon, &reconfigure_daemon)) == WP_SUCCESS) {;
    atexit(&(*daemon->shutdown));
    
    /* Daemonize ourselves:
     * fork; setsid; reset file mask; cd; reopen standard files
     */
    if((status = daemon->daemonize(daemon)) == WP_SUCCESS) {
      /* Assuming we successfully daemonize, here we start. */  
      status = daemon->start(daemon);
    }
  }

  argc = argc; argv = argv;
  return status;
}
