#include "str_buf.h"
#include <stdio.h>

typedef struct StrBuf_t {
    char *ptr;
    size_t size;
    size_t len;
} StrBuf_t;

void StrBuf_clear(StrBuf_t *buf) {
    if (buf != NULL) {
        free(buf->ptr);
        buf->size = 0;
        buf->len = 0;
    }
}

void StrBuf_destroy(StrBuf_t *buf) {
    if (buf != NULL) {
        free(buf->ptr);
    }
    free(buf);
}

/**
 * Creates new StrBuf_t.
 */
StrBuf_t *StrBuf_New(void) {
    StrBuf_t *temp = malloc(sizeof(StrBuf_t));

    if (temp != NULL) {
        temp->ptr = NULL;
        temp->len = 0;
        temp->size = 0;
    }
    return temp;
}

StrBuf_t *StrBuf_FromCharPtr(const char *str) {
    StrBuf_t *temp = malloc(sizeof(StrBuf_t));
    size_t str_len = strlen(str);

    if (temp != NULL) {
        temp->ptr = str;
        temp->len = str_len;
        temp->size = str_len;
    }

    return temp;
}

const char *StrBuf_getCharPtr(const StrBuf_t *buf) {
    return buf->ptr;
}

/**
 * Modify string and update metadata
 *
 * no null check (str, buf)
 */
bool StrBuf_update(StrBuf_t *restrict buf, const char *restrict str,
                   const size_t buf_offset, const size_t str_size) {
    if (StrBuf_changeSize(buf, str_size + buf_offset)) {
        memcpy(buf->ptr + buf_offset, str, str_size);
        buf->len = str_size + buf_offset - 1;
        buf->size = str_size + buf_offset;
        return true;
    } else {
        return false;
    }
}

/**
 * Tries to reallocate buf if smaller than size,
 * or if it fails, tries to allocate new buffer using malloc.
 *
 * no null check
 */
bool StrBuf_changeSize(StrBuf_t *buf, const size_t new_size) {
    char *tmp_ptr = NULL;

    if (buf->size < new_size) {
        tmp_ptr = realloc(buf, new_size);

        if (tmp_ptr != NULL) {
            buf->ptr = tmp_ptr;
            buf->size = new_size;
        } else {
            fprintf(stderr, "StrBuf: realloc() failed");
            return false;
        }
    }
    return true;
}
