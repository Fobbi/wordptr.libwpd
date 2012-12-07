/*
 * File:   wp_configuration.c
 * Author: Jason Short <ctor@wordptr.com>
 *
 * Created on November 28, 2012, 6:10 AM
 */

#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>
#include <obstack.h>
#include <wp_common.h>
#include <wp_configuration.h>

#define DEFAULT_UID               "daemon"
#define DEFAULT_CONFIG_FILE_PATH  "/etc/libwpd.conf"
#define DEFAULT_LOCK_FILE_NAME    "/tmp/libwpd.lock"
#define DEFAULT_RUN_PATH          "/"
#define PACKAGE_BUGREPORT         "ctor@wordptr.com"
#define GITHUB_PROJECT_PATH       "https://github.com/jgshort/wordptr.libwpd"

typedef void(*exec_config_switch_fn)(wp_configuration_pt, char *, char);

typedef struct __wp_configuration_private_t {
  /* TODO: Incorporate additional state as needed. */
  bool enable_daemon;
  bool enable_pid_lock;
  bool enable_verbose_logging;
  bool print_arguments;
  bool print_config_options;

  char *config_file_path;
  char *run_folder_path;
  char *lock_file_path;
  char *uid;
  
  wp_daemon_on_start_method_fn daemon_on_start_method;
} __wp_configuration_private_t;

typedef const wp_configuration_t* wp_configuration_cpt;

static const char *const arg_shortoptions = "hscdl:p:vu";
static const struct option arg_options[] = {
/*{ "tests",       0, NULL, 't' }, TODO: Implement later. */
  { "help",        0, NULL, 'h' },
  { "show",        1, NULL, 's' },
  { "config",      1, NULL, 'c' },
  { "daemon",      0, NULL, 'd' },
  { "lockfile",    1, NULL, 'l' },
  { "path",        1, NULL, 'p' },
  { "verbose",     0, NULL, 'v' },
  { "uid",         0, NULL, 'u' },
  { 0,             0, NULL,  0  }
};

/**
 * Gets the value indicating whether or not the pid lock file is created.
 * @param self pointer to an instance of a configuration object.
 * @return true if the pid lock configuration option is enabled, otherwise false
 */
static bool wp_config_get_enable_pid_lock(const wp_configuration_t *self) {
  assert(self && self->data);
  return self->data->enable_pid_lock;
}

/**
 * Sets the value indicating whether or not the pid lock file is created
 * @param self pointer to an instance of a configuration object
 * @param value provide true to create a pid lock file, otherwise false
 */
static void wp_config_set_enable_pid_lock(const wp_configuration_t *self, bool value) {
  assert(self && self->data);
  self->data->enable_pid_lock = value;
}

static bool wp_config_get_enable_daemon(const wp_configuration_t *self) {
  assert(self && self->data);
  return self->data->enable_daemon;
}

static void wp_config_set_enable_daemon(const wp_configuration_t *self, bool value) {
  assert(self && self->data);
  self->data->enable_daemon = value;
}

static bool wp_config_get_enable_verbose_logging(const wp_configuration_t *self) {
 assert(self && self->data);
 return self->data->enable_verbose_logging;
}

static void wp_config_set_enable_verbose_logging(const wp_configuration_t *self, bool value) {
  assert(self && self->data);
  self->data->enable_verbose_logging = value;
}

static bool wp_config_get_print_arguments(const wp_configuration_t *self) {
 assert(self && self->data);
 return self->data->print_arguments;
}

static void wp_config_set_print_arguments(const wp_configuration_t *self, bool value) {
  assert(self && self->data);
  self->data->print_arguments = value;
}

static bool wp_config_get_print_config_options(const wp_configuration_t *self) {
 assert(self && self->data);
 return self->data->print_config_options;
}

static void wp_config_set_print_config_options(const wp_configuration_t *self, bool value) {
  assert(self && self->data);
  self->data->print_config_options = value;
}

static char *wp_config_get_config_file_path(const wp_configuration_t *self) {
  assert(self && self->data);
  return self->data->config_file_path;
}

static void wp_config_set_config_file_path(const wp_configuration_t *self, const char *value) {
  assert(self && value);
  
  if(self->data->config_file_path) {
    free(self->data->config_file_path);
  }
  
  wp_safe_strcpy(&self->data->config_file_path, value);
}

static char *wp_config_get_run_folder_path(const wp_configuration_t *self) {
  assert(self && self->data);
  return self->data->run_folder_path;
}

static char *wp_config_get_lock_file_path(const wp_configuration_t *self) {
  assert(self && self->data);
  return self->data->lock_file_path;
}

static void wp_config_set_lock_file_path(const wp_configuration_t *self, const char *value) {
  assert(self && value);
  
  if(self->data->lock_file_path) {
    free(self->data->lock_file_path);
  }
  
  wp_safe_strcpy(&self->data->lock_file_path, value);
}

static char *wp_config_get_uid(const wp_configuration_t *self) {
  assert(self && self->data);
  return self->data->uid;
}

/* TODO: Remove, keeping while I make some configuration changes */
/*
static void wp_config_print_usage(wp_configuration_pt self, FILE *stream, int ec) {
  fprintf(stream,
          "Usage: libwpd arg_options\n"
          "   --help            -h   Display this help screen.\n"
          "   --show=arg        -s   Show diagnostic information.\n"
          "                             arg is one of: \n"
          "                             config   - Output options passed to the configure script.\n"
          "                             args     - Output parsed command line arguments.\n"
          "   --daemon          -d   Run as a daemon.\n"
          "   --config=arg      -c   Explicitly set the config file path. Default is " DEFAULT_CONFIG_FILE_PATH ".\n"
          "                             arg is the fully-qualified path to the config file.\n"
          "   --lockfile=arg    -l   Explicitly set the lock file path. Default is " DEFAULT_LOCK_FILE_NAME "\n"
          "                             arg is the file name of the lock file to use for the daemon instance.\n"
          "   --path=arg        -p   Explicitly set the run path. Default is " DEFAULT_RUN_PATH "\n"
          "                             arg is the fully-qualified path to the execution folder.\n"
          "\n"
          "   --verbose         -v   Output verbose information. Errors and warnings are logged to syslog.\n"
          "\n"
          "Send bug reports to " PACKAGE_BUGREPORT " or file an issue on github at\n"
          "\t" GITHUB_PROJECT_PATH "\n"
          );

  wp_configuration_delete(self);
  exit(ec);
}
*/

static void wp_config_load_helper(const wp_configuration_pt config, char *pch, char w) {
  switch(w) {
    case 'v':
      config->set_enable_verbose_logging(config, tolower(pch[0]) == 't');
      break;
    case 'd':
      config->set_enable_daemon(config, tolower(pch[0]) == 't');
      break;
    case 'o':
      config->set_print_config_options(config, tolower(pch[0]) == 't');
      break;
    case 'a':
      config->set_print_arguments(config, tolower(pch[0]) == 't');
      break;
    case 'c':
      wp_safe_strcpy(&config->data->config_file_path, pch);
      break;
    case 'r':
      wp_safe_strcpy(&config->data->run_folder_path, pch);
      break;
    case 'l':
      wp_safe_strcpy(&config->data->lock_file_path, pch);
      break;
  }
}

/**
 *  Reads a configuration file from the default configuration path and populates
 * the provided config object.
 * Configuration file format:
 * # - comment
 * name=value;
 *
 * NOTES: Extraneous white space not permitted. Comments must appear at the
 * beginning of the line. Only \n supported for newlines.
 * @param config self
 * @param fn
 * @return
 */
static wp_status_t wp_config_update_from_configuration_file(wp_configuration_pt config, exec_config_switch_fn fn, const char *file_path) {

  const char tok[2] = { '=', ';' };
  wp_status_t ret = WP_FAILURE;
  char line[WP_MAX_LINE];
  char *pch;
  char w;

  /* let's try the command line arguments first: */
  /* TODO: Fix, this is currently broken as we will not (yet) have a path populated
   * from the command line... */
  FILE *file = NULL;
  if(file_path != NULL) {
    file = fopen(file_path, "r");
  } else if(config->get_config_file_path(config) != NULL) {
    file = fopen(config->get_config_file_path(config), "r");
  }
  if(file == NULL) {
    /* maybe we weren't provided a file, let's try the default location: */
    file = fopen(DEFAULT_CONFIG_FILE_PATH, "r");
  }
  /* If we have a file, we'll try to load/read it: */
  if(file != NULL) {
    while(fgets(line, WP_MAX_LINE, file) != NULL) {
      /* TODO: Ignore spaces. */
      if(line[0] == '#') {
        continue;
      }
      pch = strtok(line, tok);
      while(pch != NULL) {
        w = pch[0];
        if(((pch = strtok(NULL, tok)) == NULL) || pch[0] == '\n') {
          break;
        }
        /* helper function to populate the config. */
        fn(config, pch, w);
      }
    }

    fclose(file);
    ret = WP_SUCCESS;
  }

  return ret;
}

static wp_status_t wp_config_populate_from_file(wp_configuration_pt self, const char *file_path) {
  return wp_config_update_from_configuration_file(self, &wp_config_load_helper, file_path);
}

static void wp_config_print_configuration(const wp_configuration_t *config) {
  fprintf(stdout, "Started with:\n");
  fprintf(stdout, "    enable verbose logging       : \"%s\"\n", (config->get_enable_verbose_logging(config) ? "true" : "false"));
  fprintf(stdout, "    enable pid lock              : \"%s\"\n", (config->get_enable_pid_lock(config) ? "true" : "false"));
  fprintf(stdout, "    enable print args            : \"%s\"\n", (config->get_print_arguments(config) ? "true" : "false"));
  fprintf(stdout, "    enable print config options  : \"%s\"\n", (config->get_print_config_options(config) ? "true" : "false"));
  fprintf(stdout, "    enable daemon                : \"%s\"\n", (config->get_enable_daemon(config) ? "true" : "false"));
  fprintf(stdout, "    uid                          : \"%s\"\n", config->get_uid(config));
  fprintf(stdout, "    run path                     : \"%s\"\n", config->get_run_folder_path(config));
  fprintf(stdout, "    lock file                    : \"%s\"\n", config->get_lock_file_path(config));
  fprintf(stdout, "    config file path             : \"%s\"\n", config->get_config_file_path(config));
}

static void wp_config_set_daemon_on_start_method(const struct wp_configuration *self, wp_daemon_on_start_method_fn fn) {
  assert(self && self->data);
  self->data->daemon_on_start_method = fn;
}

static wp_daemon_on_start_method_fn wp_config_get_daemon_on_start_method(const struct wp_configuration *self) {
  assert(self && self->data && self->data->daemon_on_start_method);
  return self->data->daemon_on_start_method;
}

/**
 * Create a new instance of a configuration object from the provided parameters
 * @param self_out the created instance of the configuration object
 * @return WP_SUCCESS on success, otherwise WP_FAILURE
 */
wp_status_t wp_configuration_new(wp_configuration_pt *self_out) {
  wp_status_t ret = WP_FAILURE;
  wp_configuration_t *self = NULL;

  if((self = malloc(sizeof(*self)))) {
    if((self->data = malloc(sizeof(*(self->data))))) {
      self->set_daemon_on_start_method = &wp_config_set_daemon_on_start_method;
      self->get_daemon_on_start_method = &wp_config_get_daemon_on_start_method;
      self->get_enable_pid_lock = &wp_config_get_enable_pid_lock;
      self->set_enable_pid_lock = &wp_config_set_enable_pid_lock;
      self->populate_from_file = &wp_config_populate_from_file;
      self->set_enable_pid_lock = &wp_config_set_enable_pid_lock;
      self->get_enable_pid_lock = &wp_config_get_enable_pid_lock;
      self->get_enable_verbose_logging = &wp_config_get_enable_verbose_logging;
      self->set_enable_verbose_logging = &wp_config_set_enable_verbose_logging;
      self->get_print_arguments = &wp_config_get_print_arguments;
      self->set_print_arguments = &wp_config_set_print_arguments;
      self->get_print_config_options = &wp_config_get_print_config_options;
      self->set_print_config_options = &wp_config_set_print_config_options;
      self->get_enable_daemon = &wp_config_get_enable_daemon;
      self->set_enable_daemon = &wp_config_set_enable_daemon;
      self->get_config_file_path = &wp_config_get_config_file_path;
      self->set_config_file_path = &wp_config_set_config_file_path;
      self->get_run_folder_path = &wp_config_get_run_folder_path;
      self->get_lock_file_path = &wp_config_get_lock_file_path;
      self->set_lock_file_path = &wp_config_set_lock_file_path;
      self->get_uid = &wp_config_get_uid;

      self->configuration_print = &wp_config_print_configuration;

      /* set some defaults: */
      self->data->daemon_on_start_method = NULL;
      self->data->enable_pid_lock = true;
      self->data->enable_daemon = false; /* no deamon by default.*/
      self->data->enable_verbose_logging = true;

      self->data->config_file_path = NULL;
      self->data->lock_file_path = NULL;
      self->data->run_folder_path = NULL;
      self->data->uid = NULL;

      ret = WP_SUCCESS;
    } else {
      free(self);
    }
  }
  
  *self_out = self;
  return ret;
}

/**
 * Deletes (frees) an instance of a configuration object
 * @param config the object instance to destroy
 * @return WP_SUCCESS if successful, otherwise WP_FAILURE
 */
void wp_configuration_delete(wp_configuration_pt self) {
  assert(self);
  if(self->data) {

    /* Assuming we're using wp_safe_strcpy... */
    if(self->data->config_file_path) { 
      free(self->data->config_file_path);
    }
    if(self->data->lock_file_path) { 
      free(self->data->lock_file_path);
    }
    if(self->data->run_folder_path) { 
      free(self->data->run_folder_path);
    }
    if(self->data->uid) { 
      free(self->data->uid);
    }

    free(self->data);
    self->data = NULL;
  }
  free(self);
  self = NULL;
}
