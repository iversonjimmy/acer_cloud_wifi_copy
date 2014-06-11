#ifndef __VPL__IMPERSONATE_HPP__
#define __VPL__IMPERSONATE_HPP__

#ifdef VPL_PLAT_IS_WINRT
//WinRT app runs under sandbox, so it can not obtain permission from others.
#define WIN32_IMPERSONATION   
#else
#include "scopeguard.hpp"
int vpl_impersonatedLoggedOnUser();
int vpl_revertToSelf();

#define WIN32_IMPERSONATION  vpl_impersonatedLoggedOnUser();  \
                            ON_BLOCK_EXIT(vpl_revertToSelf);
#endif

#endif // __VPL__IMPERSONATE_HPP__

