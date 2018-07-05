#pragma once

/******************************************************************************

  RTDL:
    turns on Runtime Dynamic Linking.

  PREFERAPI:
    turns on the behavior that the module will use the real CreateHardLink()
    function preferably over the one defined here. This is useful for maximum
    compatibility with future versions of Windows 2000/XP/2003 and their
    successors.
    For maximum compatibility with both, future versions and Windows 9x/Me
    respectively you should turn own both of these directives!

 ******************************************************************************/
#define RTDL      // Use runtime dynamic linking
#define PREFERAPI // Prefer the "real" Windows API on systems on which it exists
                  // If this is defined STDCALL is automatically needed and defined!

#if (_WIN32_WINNT >= 0x0400)
//
// API call(s) to create hard links.
//

BOOL
WINAPI
CreateHardLinkA (
    IN LPCSTR lpFileName,
    IN LPCSTR lpExistingFileName,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

BOOL
WINAPI
CreateHardLinkW (
    IN LPCWSTR lpFileName,
    IN LPCWSTR lpExistingFileName,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

#ifdef UNICODE
#define CreateHardLink  CreateHardLinkW
#else
#define CreateHardLink  CreateHardLinkA
#endif // !UNICODE

#endif // (_WIN32_WINNT >= 0x0400)

#ifdef PREFERAPI
    typedef BOOL (WINAPI *TFNCreateHardLinkA) (
        IN LPCSTR lpFileName,
        IN LPCSTR lpExistingFileName,
        IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
        );

    typedef BOOL (WINAPI *TFNCreateHardLinkW) (
        IN LPCWSTR lpFileName,
        IN LPCWSTR lpExistingFileName,
        IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
        );

    static TFNCreateHardLinkW lpfnCreateHardLinkW = NULL;
    static TFNCreateHardLinkA lpfnCreateHardLinkA = NULL;
#endif // PREFERAPI

HANDLE NtpGetProcessHeap();
BOOL Hardlink_Initialize();
