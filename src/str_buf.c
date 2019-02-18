#include "str_buf.h"

void strbuf_free(StrBuf *buf) {
  if (buf != NULL) {
    free(buf->ptr);
    buf->size = 0;
    buf->len = 0;
  }
}

void strbuf_destroy(StrBuf *buf) {
  if (buf != NULL) {
    strbuf_free(buf->ptr);
    free(buf);
  }
}

/**
 * Creates new StrBuf.
 */
StrBuf *strbuf_new(void) {
  StrBuf *temp = malloc(sizeof(StrBuf));

  if (temp != NULL) {
    temp->ptr = NULL;
    temp->len = 0;
    temp->size = 0;
  }
  return temp;
}

bool *strbuf_update(StrBuf *restrict buf, const char *restrict str,
                    const size_t buf_offset, const size_t str_size) {
  if (strbuf_change_size(buf, str_size + buf_offset)) {
    memcpy(buf->ptr + buf_offset, str, str_size);
    buf->len = str_size + buf_offset - 1;
    buf->size = str_size + buf_offset;
    return true;
  } else {
    return false;
  }
}

/**
 * Create new StrBuf from existing string.
 */
StrBuf *strbuf_from_char(const char *str) {
  StrBuf *temp = malloc(sizeof(StrBuf));

  if (temp != NULL) {
    temp->ptr = str;
    temp->len = strlen(str);
    temp->size = temp->len + 1;
  }
  return temp;
}

/**
 * Tries to reallocate buf if smaller than size,
 * or if it fails, tries to allocate new buffer using malloc.
 *
 * Does not do NULL check.
 */
bool strbuf_change_size(StrBuf *buf, const size_t size) {
  char *tmp_ptr = NULL;

  if (buf->size < size) {
    if ((tmp_ptr = realloc(buf, size)) != NULL) {
      buf->size = size;
    } else if ((tmp_ptr = malloc(size)) != NULL) {
      strbuf_free(buf);
      buf->ptr = tmp_ptr;
      buf->size = size;
    } else {
      return false;
    }
  }
  return true;
}