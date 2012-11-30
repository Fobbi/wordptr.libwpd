/*
 * File:   wp_daemonizer.h
 * Author: Jason Short <ctor@wordptr.com>
 *
 * Created on November 28, 2012, 6:10 AM
 */

#ifndef WP_DAEMONIZER__H
#define WP_DAEMONIZER__H

#include <wp_common.h>
#include <wp_configuration.h>

struct wp_daemonizer;

/* Keep the private impementation... private. */
struct __wp_daemonizer_private_t;
typedef struct __wp_daemonizer_private_t *wp_daemonizer_private_t;

typedef void (*wp_reconfigure_method_fn)(const struct wp_daemonizer *, wp_configuration_pt);

/* Here's the public interface! */
typedef struct wp_daemonizer {
  /* The daemonize method which will fork, etc., our process */
  wp_status_t (*daemonize)(const struct wp_daemonizer *self);
  /* Start the "main loop." */
  wp_status_t (*start)(const struct wp_daemonizer *self);

  /* Return an instance of the daemon singleton. */
  struct wp_daemonizer* (*get_instance)();

  /* Handle signals from the OS. */
  void (*signal_handler)(int sig);
  void (*install_signal_handlers)();
  /* Shutdown the daemon. */
  void (*shutdown)();

  void (*set_reconfigure_method)(const struct wp_daemonizer *self, wp_reconfigure_method_fn fn);
  
  /* Our private implementation details. */
  wp_daemonizer_private_t data;
} wp_daemonizer_t, *wp_daemonizer_pt;

/* Utility method to create a new instance of the daemonizer */
/*wp_status_t wp_daemonizer_initialize(wp_daemonizer_t **self_out, wp_configuration_t *config);*/
wp_status_t wp_daemonizer_initialize(wp_daemonizer_pt *out, wp_reconfigure_method_fn fn);

#endif /* WP_DAEMONIZER__H */
