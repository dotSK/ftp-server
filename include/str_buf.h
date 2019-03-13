#ifndef STR_BUF
#define STR_BUF

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

__attribute__((designated_init)) typedef struct StrBuf
{
  char *ptr;
  size_t size;
  size_t len;
} StrBuf;

extern StrBuf cwd;

void strbuf_free(StrBuf *buf);
void strbuf_destroy(StrBuf *buf);
StrBuf *strbuf_new(void);
bool *strbuf_update(StrBuf *restrict buf, const char *restrict str,
                    const size_t buf_offset, const size_t str_size);
StrBuf *strbuf_from_char(const char *str);
bool strbuf_change_size(StrBuf *buf, const size_t size);
#endif