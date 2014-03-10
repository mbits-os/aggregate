/* 
 *  Copied to fcgi_config.h when building on WinNT without cygwin,
 *  i.e. configure is not run.  See fcgi_config.h.in for details.
 */
#ifdef _WIN32
#include "fcgi_config_win32.h"
#else
#include "fcgi_config_posix.h"
#endif