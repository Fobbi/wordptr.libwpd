#include <pwd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <wp_common.h>
#include <wp_configuration.h>
#include <wp_daemonizer.h>

const size_t DEFAULT_BUFFER_SIZE = 16384;

static wp_daemonizer_t *instance = NULL;

typedef struct __wp_daemonizer_private_t {
  /* TODO: Incorporate additional state as needed. */
  wp_configuration_t *config;
  
  wp_reconfigure_method_fn reconfigure_method;
  int created_pid_lock_file;
} __wp_daemonizer_private_t;


/**
 * Redirect standard file descriptors to /dev/null.
 * @param self pointer to an instance of the daemonizer. Not utilized at this time.
 */
static void wp_daemonizer_null_file_descriptors(const wp_daemonizer_t *self) {
  assert(self); /* make compiler happy for now. */
  struct fp { FILE *file; const char *mode; };
  struct fp files[] = {
    { .file = stdin , .mode = "r" },
    { .file = stdout, .mode = "w" },
    { .file = stderr, .mode = "w" }
  };
  for(size_t i = 0; i < sizeof(files) / sizeof(struct fp); i++) {
    FILE *f = freopen("/dev/null", files[i].mode, files[i].file);
    if(f == NULL) {
      wp_log(stderr, self->data->config, LOG_ERR, "FATAL: freopen: %m");
      exit(EXIT_FAILURE);
    }
  }
}

/**
 * Open the lock file name and save the PID of the daemon into it.
 * @param self pointer to an instance of the daemonizer.
 * @return WP_SUCCESS.
 */
static wp_status_t wp_daemonizer_set_pid_lock(const wp_daemonizer_t *self) {
  wp_status_t res = WP_FAILURE;
  wp_configuration_t *config = NULL;
  char *lock_file_name = NULL;

  config = self->data->config;
  lock_file_name = config->get_lock_file_path(config); 
  struct stat sts;
  if(stat(lock_file_name, &sts) != 0 && errno == ENOENT) {
    int lfp = open(lock_file_name, O_WRONLY | O_CREAT | O_EXCL, 0640);
    if(lfp > -1) {
      if(ftruncate(lfp, 0) == 0) {
        char pidtext[WP_MAX_LINE];
        snprintf(pidtext, sizeof(pidtext), "%ld\n", (long)getpid());
        int r = write(lfp, pidtext, strlen(pidtext));
        if(r > 0) {
          self->data->created_pid_lock_file = 1;
          res = WP_SUCCESS;
        }
      } 
    } 
  }
  
  if(res != WP_SUCCESS) {
    wp_log(stdout, config, LOG_INFO, "Could not acquire exclusive lock %s due to %i: %m", lock_file_name, errno);
  }

  return res;
}

/**
 * Set the UID to daemon.
 * @param self pointer to an instance of the daemonizer. Not utilized at this time.
 * @return WP_SUCCESS on success, otherwise WP_FAILURE.
 */
static wp_status_t wp_daemonizer_set_uid(const wp_daemonizer_t *self) {
  if(getppid() != 1) {
    if(getuid() == 0 || geteuid() == 0) {
      /* Setting uid to " DEFAULT_UID */
      struct passwd pwd;
      struct passwd *result;

      size_t bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
      if (!bufsize) {
          bufsize = DEFAULT_BUFFER_SIZE;
      }
      char *buff = malloc(bufsize);
      if (buff != NULL) {
        wp_configuration_t *config = NULL;
        config = self->data->config;
        char *uid = config->get_uid(config);
        int s = getpwnam_r(uid, &pwd, buff, bufsize, &result);

        if(s == 0 && result) {
          setuid(result->pw_uid);
          wp_log(stdout, config, LOG_INFO, "UID set to %s: %m", uid);
        }
        free(buff);
        buff = NULL;
      } else {
          exit(EXIT_FAILURE);
      }
    } else {
      wp_log(stderr, self->data->config, LOG_ERR, "WARNING: Unable to set uid to '%s'. Sudo? %m", self->data->config->get_uid(self->data->config));
    }
  } else {
    wp_log(stderr, self->data->config, LOG_ERR, "Daemon already running: %m");
    return WP_FAILURE;
  }

  return WP_SUCCESS;
}

static wp_status_t wp_daemonizer_daemonize(const wp_daemonizer_t *self) {
  wp_status_t res = WP_FAILURE;
  wp_configuration_t *config = NULL;
  pid_t pid, sid;

  config = self->data->config;

  if(!config->get_enable_daemon(config)) {
    wp_log(stdout, config, LOG_INFO, "Daemon option not enabled, starting main loop: %m");
    return WP_SUCCESS;
  }

    
  if((res = wp_daemonizer_set_uid(self)) == WP_SUCCESS) {
    /* Forking. Opening syslog for exsvcd. */
    if((pid = fork()) < 0) {
      /* fork error */
      /* FATAL: fork: %m */
      exit(EXIT_FAILURE);
    } else if(pid != 0) {
      exit(EXIT_SUCCESS);
    }

    umask(027);

    sid = setsid(); /* get a new process group. */
    if(sid < 0) {
      /* FATAL: setsid: %m */
      exit(EXIT_FAILURE);
    }

    /* log to syslog prefixed with exsvcd */
    openlog("exsvcd", LOG_PID, LOG_USER);

    wp_log(stderr, self->data->config, LOG_INFO, "syslog file opened: %m");
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if(sigaction(SIGHUP, &sa, NULL) < 0) {
      wp_log(stderr, self->data->config, LOG_ERR, "FATAL: sigaction: %m");
      exit(EXIT_FAILURE);
    }

    /* double-fork pattern to prevent zombie children */
    if((pid = fork()) < 0) {
      wp_log(stderr, self->data->config, LOG_ERR, "FATAL: fork: %m");
      exit(EXIT_FAILURE);
    } else if(pid != 0) {
      /* parent */
      exit(EXIT_SUCCESS);
    }

    char *run_path = config->get_run_folder_path(config);
    if(chdir(run_path) == 0) {
      if(wp_daemonizer_set_pid_lock(self) != WP_SUCCESS) {
        exit(EXIT_FAILURE);
      }
    } else { 
      wp_log(stderr, self->data->config, LOG_ERR, "FATAL: chdir: %m");
      exit(EXIT_FAILURE);
    }
    
    /* redirect stdin, out, err to NULL */
    wp_daemonizer_null_file_descriptors(self);

    return WP_SUCCESS;
  } else {
    /* couldn't set UID. */
    wp_log(stderr, self->data->config, LOG_ERR, "FATAL: Couldn't set UID: %m");
  }
  
  exit(EXIT_FAILURE);
}

static void wp_daemonizer_shutdown() {
  /* Perform cleanup here */
  if(instance) {
    if(instance->data) {
      /* Naive removal of the pid lock. */
      wp_configuration_t *config = NULL;
      config = instance->data->config;
      if(config && config->get_enable_daemon(config) && instance->data->created_pid_lock_file > 0) {
        /* only need to remove the lock file if we're daemonized... */
        char *lock_file_name = NULL;
        lock_file_name = config->get_lock_file_path(config);
        remove(lock_file_name);
        wp_log(stderr, instance->data->config, LOG_ERR, "Removed lock file: %s: %m", lock_file_name);

        /* TODO: Revise the removal of the configuration instance... */
        wp_configuration_delete(instance->data->config);
      }
      free(instance->data);
      instance->data = NULL;
    }
    free(instance);
    instance = NULL;
  }
  exit(EXIT_SUCCESS);
}

/**
 * Handle signals from the OS.
 * @param sig The signal to process.
 */
static void wp_daemonizer_signal_handler(int sig) {
  switch(sig) {
    case SIGHUP:
      /* reload configuration, for example */
      break;
    case SIGINT:
    case SIGTERM:
      /* perform shutdown work */
      wp_daemonizer_shutdown();
      break;
  }
}

/**
 * Return the reference (or NULL) of the single instance of the daemonizer class.
 * @return The reference to the daemonizer instance or NULL if uninitialized.
 */
static wp_daemonizer_t *wp_daemonizer_get_instance() {
  return instance;
}

/**
 * Signup for signal events. Right now, we are interested in child, hang up,
 * terminate and interrupt.
 */
static void wp_daemonizer_install_signal_handlers() {
  signal(SIGCHLD, wp_daemonizer_signal_handler);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGHUP,  wp_daemonizer_signal_handler);
  signal(SIGTERM, wp_daemonizer_signal_handler);
  signal(SIGINT, wp_daemonizer_signal_handler);
}

/**
 * The main daemon loop. Eventually we will delegate responsibility to a method
 * for processing messages. For now, we just loop.
 * @param self pointer to an instance of the daemonizer. Not utilized at this time.
 * @return WP_SUCCESS
 */
static wp_status_t wp_daemonizer_start(const wp_daemonizer_t *self) {
  assert(self); /* make compiler happy */
  sigset_t mask, oldmask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);
  sigprocmask(SIG_BLOCK, &mask, &oldmask);
  
  while(true) {
    sigsuspend(&oldmask);
  }
  
  sigprocmask(SIG_UNBLOCK, &mask, NULL);

  return WP_SUCCESS;
}

static void wp_daemonizer_set_reconfigure_method(const struct wp_daemonizer *self, wp_reconfigure_method_fn fn) {
  assert(self);
  
  self->data->reconfigure_method = fn;
}

/**
 * Initialize the singleton instance of the daemon.
 * @param self_out The reference to the singleton.
 * @return WP_SUCCESS if everything is great, otherwise NULL.
 */
wp_status_t wp_daemonizer_initialize(wp_daemonizer_t **out, wp_reconfigure_method_fn fn) {
  wp_status_t ret = WP_FAILURE;
  wp_configuration_pt config = NULL;
  wp_daemonizer_pt self = NULL;
  
  if(instance) {
    ret = WP_SUCCESS;
  } else {
    
    if((ret = wp_configuration_new(&config)) != WP_SUCCESS) {
      /* TODO: Print out some help. */
      wp_configuration_delete(config);
    } else {
      config->populate_from_file(config);
      /* config->populate_from_args(config, argc, argv); 
      if(config->get_print_arguments(config) || config->get_print_config_options(config)) {
        config->configuration_print(config);
      }
      */
      
      if((self = malloc(sizeof(*self)))) {
        if((self->data = malloc(sizeof(*(self->data))))) {
          /* TODO: Load from the command line or config file. */
          self->data->config = config;
          self->data->created_pid_lock_file = 0;

          /* Setup some static and instance methods... */
          self->daemonize = &wp_daemonizer_daemonize;
          self->signal_handler = &wp_daemonizer_signal_handler;
          self->shutdown = &wp_daemonizer_shutdown;
          self->install_signal_handlers = &wp_daemonizer_install_signal_handlers;
          self->get_instance = &wp_daemonizer_get_instance;
          self->start = &wp_daemonizer_start;
          self->set_reconfigure_method = &wp_daemonizer_set_reconfigure_method;
          self->data->reconfigure_method = fn;

          /* Let's try to reconfigure ourselves.*/
          fn(self, config);

          /* By default, install the signal handlers. Will probably change. */
          self->install_signal_handlers();
          instance = self;
          ret = WP_SUCCESS;
        } else {
          free(self);
          self = NULL;
        }
      }
    }
  }

  *out = instance;
  return ret; 
}
