#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include <libwpd.h>

/* sed-begin-wait-loop */
static void daemon_on_start(const wp_daemonizer_t *self) {
  assert(self); /* make compiler happy */
  sigset_t mask, oldmask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);
  sigprocmask(SIG_BLOCK, &mask, &oldmask);
  
  while(true) {
    sigsuspend(&oldmask);
  }
  
  sigprocmask(SIG_UNBLOCK, &mask, NULL);
}
/* sed-end-wait-loop */

static void reconfigure_daemon(const struct wp_daemonizer *daemon, const wp_configuration_pt config) {
  daemon = daemon;
  config->set_enable_verbose_logging(config, true);
  config->set_daemon_on_start_method(config, &daemon_on_start);
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
