#include "km_types.h"
#include "string.h"

int
strncmp(const char *a, const char *b, size_t len)
{
	int r = 0;

	while (len--) {
		r = *a - *b;
		if (r || *a == 0 || *b == 0 ) {
			break;
		}
		a++;
		b++;
	}
	return r;
}

int
memcmp(const void *s1, const void *s2, size_t len)
{
	int r = 0;
	const s8 *a = s1;
	const s8 *b = s2;

	while (len-- && r == 0) {
		r = *a++ - *b++;
	}
	return r;
}

size_t
strnlen(const char *s, size_t len)
{
	int r = 0;

	while (len--) {
		if (*s++ == 0) {
			break;
		}
		r++;
	}
	return r;
}

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t
strlcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strnlen(s, ~0UL));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}

char*
strncpy(char *dest, const char *src, size_t n)
{
	size_t i;

	for (i = 0 ; i < n && src[i] != '\0' ; i++) {
		dest[i] = src[i];
	}
	for ( ; i < n ; i++) {
		dest[i] = '\0';
	}

	return dest;
}

void *
memchr(const void *s, int c, size_t n)
{
	size_t i;
	s8    *sp = (s8 *)s;

	for( i = 0 ; i < n ; i++ ) {
		if ( *sp == c ) {
			return sp;
		}
		sp++;
	}

	return NULL;
}
