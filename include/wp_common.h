/*
 * File:   wp_common.h
 * Author: Jason Short <ctor@wordptr.com>
 *
 * Created on November 28, 2012, 6:10 AM
 */

#ifndef WP_COMMON__H
#define WP_COMMON__H

#include <stddef.h>
#include <syslog.h>

typedef int wp_status_t;

const size_t WP_MAX_LINE;
const wp_status_t WP_SUCCESS;
const wp_status_t WP_FAILURE;

char *wp_safe_strcpy(char **dest, const char *src);

#ifdef NDEBUG
  #define wp_log(fileptr, priority, ...) ((void)0)
#else
  #define wp_log(fileptr, config, priority, ...) \
    do { \
      if(config) { \
        if(config->get_enable_verbose_logging(config)) { \
          if(config->get_enable_daemon(config)) {\
            syslog((priority), __VA_ARGS__); \
          } \
          if(!config->get_enable_daemon(config)) { \
            fprintf((fileptr), __VA_ARGS__ ); \
            fprintf((fileptr), "\n"); \
          } \
        } \
      } \
    } while(0)
#endif

#endif

