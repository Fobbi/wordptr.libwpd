/* 
 * File:   wp_pool.h
 * Author: Jason Short <ctor@wordptr.com>
 *
 * Created on November 28, 2012, 6:10 AM
 */

#ifndef WP_POOL__H
#define	WP_POOL__H

#include <stdlib.h>
#include <wp_common.h>

struct __wp_pool_private_t;
typedef struct __wp_pool_private_t *wp_pool_private_t;

typedef struct wp_pool {
  /* TODO: Create the pool. */
  void *(*palloc)(size_t size);
  void (*pfree)(void *what);

  wp_pool_private_t data;
} wp_pool_t;

wp_status_t wp_pool_new(wp_pool_t **self_out, size_t size);

#endif
