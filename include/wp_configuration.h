/* 
 * File:   wp_configuration.h
 * Author: Jason Short <ctor@wordptr.com>
 *
 * Created on November 28, 2012, 6:10 AM
 */

#ifndef WP_CONFIGURATION__H
#define	WP_CONFIGURATION__H

#include <stdbool.h>

struct wp_daemonizer;
struct __wp_configuration_private_t;
typedef struct __wp_configuration_private_t *wp_configuration_private_t;

typedef void (*wp_daemon_start_method_fn)(const struct wp_daemonizer *);

typedef struct wp_configuration {
  wp_status_t (*populate_from_file)(struct wp_configuration *self);
  
  /* TODO: revise: wp_status_t (*reload)(const struct wp_configuration *self); */
  
  bool (*get_enable_pid_lock)(const struct wp_configuration *self);
  void (*set_enable_pid_lock)(const struct wp_configuration *self, bool value);
 
  bool (*get_enable_daemon)(const struct wp_configuration *self);
  void (*set_enable_daemon)(const struct wp_configuration *self, bool value);
  bool (*get_enable_verbose_logging)(const struct wp_configuration *self);
  void (*set_enable_verbose_logging)(const struct wp_configuration *self, bool value);
  bool (*get_print_arguments)(const struct wp_configuration *self);
  void (*set_print_arguments)(const struct wp_configuration *self, bool value);
  bool (*get_print_config_options)(const struct wp_configuration *self);
  void (*set_print_config_options)(const struct wp_configuration *self, bool value);

  void (*configuration_print)(const struct wp_configuration *self);

  char *(*get_config_file_path)(const struct wp_configuration *self);
  char *(*get_run_folder_path)(const struct wp_configuration *self);
  char *(*get_lock_file_path)(const struct wp_configuration *self);
  char *(*get_uid)(const struct wp_configuration *self);

  wp_daemon_start_method_fn (*get_daemon_start_method)(const struct wp_configuration *self);
  void (*set_daemon_start_method)(const struct wp_configuration *self, wp_daemon_start_method_fn fn);
  
  wp_configuration_private_t data;
} wp_configuration_t, *wp_configuration_pt;

/**
 * Create a new wp_configuration_t instance.
 * @param config will point to the newly created configuration instance, or NULL
 *        on failure.
 * @param argc argc from the command line
 * @param argv argv from the command line
 * @return returns WP_SUCCESS on success, otherwise WP_FAILURE.
 */
wp_status_t wp_configuration_new(wp_configuration_pt *config);

/**
 * Deletes an wp_configuration_t instance.
 * @param config The configuration instance to delete. Will contain NULL upon
 *        success.
 * @return returns WP_SUCCESS.
 */
void wp_configuration_delete(wp_configuration_pt config);

#endif	/* WP_CONFIGURATION_H */
