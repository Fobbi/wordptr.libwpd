/*
 * File:   wp_pool.c
 * Author: Jason Short <ctor@wordptr.com>
 *
 * Created on November 28, 2012, 6:10 AM
 */
#include <stdlib.h>
#include <wp_pool.h>

typedef struct __wp_pool_private_t {
  void *pool;
  const wp_pool_t *parent;
} __wp_pool_private_t;

wp_status_t wp_pool_new(wp_pool_t **self_out, size_t size) {
  wp_status_t ret = WP_FAILURE;
  wp_pool_t *self = NULL;

  if((self = malloc(sizeof(*self)))) {
    if((self->data = malloc(sizeof(*(self->data))))) {
      if((self->data->pool = malloc(size))) {
        *self_out = self;
        ret = WP_SUCCESS;
      } else {
        free(self->data);
        self->data = NULL;
        free(self);
        self = NULL;
      }
    } else {
      free(self);
      self = NULL;
    }
  }

  return ret;
}
