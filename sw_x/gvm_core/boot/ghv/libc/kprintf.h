#ifndef __KPRINTF_H__
#define __KPRINTF_H__

#include <stdarg.h>

extern void kvaprintf(unsigned long flags, const char *f, va_list args);
extern void kprintf(const char *f, ...) __attribute__ ((format (printf, 1, 2)));

extern ret_t kprintf_init(void);
extern void kprintf_init2(un, un);
extern void kprintf_fin(void);

extern void set_kp_intvec(un, un);

extern void kp_bt(void);

extern int sprintf(char *buf, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
extern int vsprintf(char *buf, const char *fmt, va_list ap);
extern int vsnprintf(char *str, size_t size, const char *format, va_list ap);
extern int snprintf(char *str, size_t size, const char *format, ...) __attribute__ ((format (printf, 3, 4)));

#endif
