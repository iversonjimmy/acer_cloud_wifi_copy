#ifndef __EC_LOG_H__
#define __EC_LOG_H__

// EC_DEBUG is set in individual files because otherwise the logging
// can be overwhelming... It's a crappy way to control debug output.

#ifdef __KERNEL__
#  ifdef EC_DEBUG
#    define DBLOG(...) printk(__VA_ARGS__)
#  endif
#endif

#if !defined(GVM_FEAT_GHV_DBG) && defined(EC_DEBUG)
#  undef EC_DEBUG
#endif // !defined(GVM_FEAT_GHV_DBG) && defined(EC_DEBUG)

#ifdef EC_DEBUG
#  ifdef GHV 
#    define DBLOG(...)  {kprintf("%s: ", __FUNCTION__); kprintf("#" __VA_ARGS__);}
#  else
#    include "log.h"
#    define DBLOG(...)  LOG_INFO(__VA_ARGS__)
#  endif
#endif // EC_DEBUG

#ifndef DBLOG
#  define DBLOG(...)	(void)(0)
#endif // !DBLOG

#if defined(GVM_FEAT_GHV_DBG)
#  ifdef GHV 
#    define LOG(...)  {kprintf("%s: ", __FUNCTION__); kprintf("#" __VA_ARGS__);}
#  else
#    include "log.h"
#    define LOG(...)  LOG_INFO(__VA_ARGS__)
#  endif
#else // !defined(GVM_FEAT_GHV_DBG)
#  define LOG(...)	(void)(0)
#endif // !defined(GVM_FEAT_GHV_DBG)

// GVM Core's logger always appends a newline, so use these to avoid putting newlines in the
// string constants and still have the "\n" appended when calling kprintf.
#ifdef GHV
#  define DBLOG_N(fmt, ...)  DBLOG((fmt)"\n", ##__VA_ARGS__)
#  define LOG_N(fmt, ...)    LOG((fmt)"\n", ##__VA_ARGS__)
#  define LOG_DEBUG(fmt, ...)  LOG((fmt)"\n", ##__VA_ARGS__)
#  define LOG_INFO(fmt, ...)   LOG((fmt)"\n", ##__VA_ARGS__)
#  define LOG_ERROR(fmt, ...)  LOG((fmt)"\n", ##__VA_ARGS__)
#else
#  include "log.h"
   /// @deprecated Instead, use LOG_INFO, LOG_ERROR, etc
#  define DBLOG_N(fmt, ...)  LOG_INFO((fmt), ##__VA_ARGS__)
   /// @deprecated Instead, use LOG_INFO, LOG_ERROR, etc
#  define LOG_N(fmt, ...)    LOG_INFO((fmt), ##__VA_ARGS__)
#endif

#ifdef EC_DEBUG
static inline void
dbg_bytes_dump(const char *msg, u8 *bp, u32 len)
{
	u32 i, j;

	kprintf("#hex memory dump %s: %p[%d]\n", msg, bp, len);
	for( i = 0 ; i < len ; i += j ) {
		kprintf("#%p: ", bp);
		for ( j = 0 ; j < 16 ; j++ ) {
			if ( (i + j) >= len ) {
				break;
			}
			kprintf("#%02x ", *bp++);
		}
		kprintf("#\n");
	}
}
#else
#define dbg_bytes_dump(...)		(void)(0)
#endif // EC_DEBUG

#endif // __EC_LOG_H__
