#ifndef __STRING_H__
#define __STRING_H__

extern size_t strlcat(char *, const char *, size_t);
extern size_t strnlen(const char *, size_t);
extern int strncmp(const char *, const char *, size_t);
extern char *strncpy(char *, const char *, size_t);

extern void *memcpy(void *, const void *, size_t);
extern void memzero(void *, size_t);
extern void *memset(void *, int, size_t);
extern int memcmp(const void *, const void *, size_t);

extern void *memchr(const void *, int c, size_t);

#endif
