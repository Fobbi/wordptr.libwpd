/* 
 * File:   wp_string.h
 * Author: Jason Short <ctor@wordptr.com>
 *
 * Created on November 28, 2012, 6:10 AM
 */

#ifndef WP_STRING__H
#define	WP_STRING__H

#include <stdbool.h>
#include <wp_pool.h>

struct __wp_string_private_t;
typedef struct __wp_string_private_t *wp_string_private_t;

typedef struct wp_string {
  wp_status_t (*copy)(struct wp_string **self_out, const struct wp_string *src);
  wp_status_t (*copy_to_pool)(struct wp_string **self_out, const wp_pool_t *pool, const struct wp_string *src);

  bool (*equals)(const struct wp_string *self, const struct wp_string *to);
  int (*compare)(const struct wp_string *self, const struct wp_string *to);

  const char *(*get_str)(const struct wp_string *self);
  size_t (*get_length)(const struct wp_string *self);
  int (*get_hash)(const struct wp_string *self);
  int (*get_ref_count)(const struct wp_string *self);
  wp_pool_t *(*get_pool)(const struct wp_string *self);

  wp_status_t (*concat)(const struct wp_string *self, struct wp_string **self_out, ...);
  wp_status_t (*substr)(const struct wp_string *self, struct wp_string **self_out, int index, size_t len);
  
  struct wp_string *(*ltrim)(const struct wp_string *self);
  struct wp_string *(*rtrim)(const struct wp_string *self);
  struct wp_string *(*trim)(const struct wp_string *self);

  wp_string_private_t data;
} wp_string_t;

wp_status_t wp_string_new(wp_string_t **self_out, const wp_pool_t *pool, const char *str);
wp_status_t wp_string_delete(wp_string_t **self_out);

int wp_string_compare(const wp_string_t *left, const wp_string_t *right);
int wp_string_get_ref_count(const wp_string_t *str);

#endif
