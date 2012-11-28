/*
 * File:   wp_string.c
 * Author: Jason Short <ctor@wordptr.com>
 *
 * Created on November 28, 2012, 6:10 AM
 */

#include <assert.h>
#include <string.h>

#include <wp_pool.h>
#include <wp_string.h>

typedef struct __wp_string_private_t {
  /* TODO: Incorporate additional state as needed. */
  size_t len;
  int ref_count;
  char *str;
  const wp_pool_t *pool;
} __wp_string_private_t;

wp_status_t wp_string_new(wp_string_t **self_out, const wp_pool_t *pool, const char *str) {
  assert(pool);
  wp_status_t ret = WP_FAILURE;
  wp_string_t *self = NULL;
  size_t len = strlen(str) + 1;
  if((self = pool->palloc(sizeof(*self)))) {
    if((self->data = pool->palloc(sizeof(*(self->data))))) {
      if((self->data->str = pool->palloc(len))) {
        strncpy(self->data->str, str, len);
        (self->data->str)[len - 1] = '\0';
        self->data->ref_count = 1;
        self->data->len = len;
        self->data->pool = pool;
        *self_out = self;
        ret = WP_SUCCESS;
      }
    }
  }

  return ret;
}
