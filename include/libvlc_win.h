#ifndef LIBVL_WIN_H_INCLUDED
#define LIBVL_WIN_H_INCLUDED

#ifdef _MSC_VER
	#if (_MSC_VER >= 1300)
		#define _MSVC
	#endif
#endif

#ifdef _MSVC
#pragma warning(push, 0)
#endif

#if defined(_WIN32) | defined(_WIN64) 

#include <WinSock.h>

struct pollfd {

	SOCKET  fd;
	SHORT   events;
	SHORT   revents;

};

__declspec(dllimport)
int
__stdcall
WSAPoll(
	_Inout_ struct pollfd* fdArray,
	_In_ ULONG fds,
	_In_ INT timeout
);

#define poll WSAPoll

typedef unsigned int ssize_t;

#endif

#include <vlc_common.h>
#include <vlc_meta.h>
#include <vlc_variables.h>
#include <vlc_picture.h>
#include <vlc_picture_pool.h>
#include <vlc_threads.h>
#include <vlc/vlc.h>
#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_player.h>

#ifdef _MSVC
#pragma warning(pop)
#endif

#endif //!LIBVL_WIN_H_INCLUDED