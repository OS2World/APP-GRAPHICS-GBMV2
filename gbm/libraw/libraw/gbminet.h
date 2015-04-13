/* HN: Addon for GBM
 * Replacement macros for htonl/ntohl and htons/ntohs
 * to get rid of the dependency to TCPIP32 on OS/2
 * (OpenWatcom) which would break backward compatibility
 * to Warp 3 or Warp 4 without 32bit TCPIP stack installed.
 */

#ifdef __OS2__

#ifdef htonl
 #undef htonl
#endif
#ifdef ntohl
 #undef ntohl
#endif

#ifdef htons
 #undef htons
#endif
#ifdef ntohs
 #undef ntohs
#endif

#define htonl(x) ((x<<24)|(x>>24)|((x&0xff00)<<8)|((x&0xff0000)>>8))
#define ntohl(x) ((x<<24)|(x>>24)|((x&0xff00)<<8)|((x&0xff0000)>>8))
#define htons(x) ((unsigned short)(((unsigned)x<<8)|((unsigned)x>>8)))
#define ntohs(x) ((unsigned short)(((unsigned)x<<8)|((unsigned)x>>8)))

#else /* Other platforms */

#ifndef WIN32
#if (__GNUC__ < 4)
#include <io.h>
#else
#include <netinet/in.h>
#endif
#else
#include <winsock2.h>
#endif

/* use the platform implementations instead */

#endif


