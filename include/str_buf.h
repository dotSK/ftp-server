#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

typedef struct StrBuf_t StrBuf_t;

// Static functions
StrBuf_t* StrBuf_New(void);
StrBuf_t* StrBuf_FromCharPtr(const char* str);

// Instance functions
void StrBuf_clear(StrBuf_t* buf);
bool StrBuf_update(StrBuf_t* restrict buf, const char* restrict str,
                   const size_t buf_offset, const size_t str_size);
bool StrBuf_changeSize(StrBuf_t* buf, const size_t new_size);
char* StrBuf_getCharPtr(const StrBuf_t* buf);
void StrBuf_destroy(StrBuf_t* buf);
