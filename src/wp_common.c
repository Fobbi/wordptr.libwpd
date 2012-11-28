/*
 * File:   wp_common.c
 * Author: Jason Short <ctor@wordptr.com>
 *
 * Created on November 28, 2012, 6:10 AM
 */

#include <stdlib.h>
#include <string.h>
#include <wp_common.h>

const size_t WP_MAX_LINE = 1024;
const wp_status_t WP_SUCCESS = 0;
const wp_status_t WP_FAILURE = -1;

/**
 * Safely copy source string to destination. Caller responsible for freeing.
 * @param dest The destination address
 * @param src The source string to copy to dest.
 * @return The destination pointer or NULL on failure.
 */
char *wp_safe_strcpy(char **dest, const char *src) {
  /* TODO: This isn't exactly "safe." If the caller doesn't free, we have a
           dangling pointer. Review in the future to utilize a memory pool
           or similar device.
   */
  char *tmp = NULL;
  char *ret = NULL;

  register size_t len = strlen(src) + 1;

  /* Caller responsible for freeing... */
  if((tmp = malloc(len)) != NULL) {
    ret = strncpy(tmp, src, len);

    (tmp)[len - 1] = '\0';

    *dest = tmp;
    return *dest;
  }

  /* Error: */
  return NULL;
}
