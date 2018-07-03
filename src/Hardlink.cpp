/******************************************************************************
 ******************************************************************************
 ***                                                                        ***
 ***  Hardlinks. Implementation of the CreateHardLink() API introduced in   ***
 ***  Windows 2000 - BUT ALSO COMPATIBLE with Windows NT 4.0!               ***
 ***  This implementation should be fully compatible with the Windows 2000  ***
 ***  implementation (including GetLastError() code and so on).             ***
 ***                                                                        ***
 ***  Version [1.00]                                {Last mod 2005-03-06}   ***
 ***                                                                        ***
 ******************************************************************************
 ******************************************************************************

                                 _\\|//_
                                (` * * ')
 ______________________________ooO_(_)_Ooo_____________________________________
 ******************************************************************************
 ******************************************************************************
 ***                                                                        ***
 ***                Copyright (c) 1995 - 2005 by -=Assarbad=-               ***
 ***                                                                        ***
 ***   CONTACT TO THE AUTHOR(S):                                            ***
 ***    ____________________________________                                ***
 ***   |                                    |                               ***
 ***   | -=Assarbad=- aka Oliver            |                               ***
 ***   |____________________________________|                               ***
 ***   |                                    |                               ***
 ***   | Assarbad @ gmx.info|.net|.com|.de  |                               ***
 ***   | ICQ: 281645                        |                               ***
 ***   | AIM: nixlosheute                   |                               ***
 ***   |      nixahnungnicht                |                               ***
 ***   | MSN: Assarbad@ePost.de             |                               ***
 ***   | YIM: sherlock_holmes_and_dr_watson |                               ***
 ***   |____________________________________|                               ***
 ***             ___                                                        ***
 ***            /   |                     ||              ||                ***
 ***           / _  |   ________ ___  ____||__    ___   __||                ***
 ***          / /_\ |  / __/ __//   |/  _/|   \  /   | /   |                ***
 ***         / ___  |__\\__\\  / /\ || |  | /\ \/ /\ |/ /\ | DOT NET        ***
 ***        /_/   \_/___/___/ /_____\|_|  |____/_____\\__/\|                ***
 ***       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~        ***
 ***              [http://assarbad.net | http://assarbad.org]               ***
 ***                                                                        ***
 ***   Notes:                                                               ***
 ***   - my first name is Oliver, you may well use this in your e-mails     ***
 ***   - for questions and/or proposals drop me a mail or instant message   ***
 ***                                                                        ***
 ***~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~***
 ***              May the source be with you, stranger ... ;)               ***
 ***    Snizhok, eto ne tolko fruktovij kefir, snizhok, eto stil zhizni.    ***
 ***                     Vsekh Privet iz Germanii                           ***
 ***                                                                        ***
 *** Greets from -=Assarbad=- fly to YOU =)                                 ***
 *** Special greets fly 2 Nico, Casper, SA, Pizza, Navarion, Eugen, Zhenja, ***
 *** Xandros, Melkij, Strelok etc pp.                                       ***
 ***                                                                        ***
 *** Thanks to:                                                             ***
 *** W.A. Mozart, Vivaldi, Beethoven, Poeta Magica, Kurtzweyl, Manowar,     ***
 *** Blind Guardian, Weltenbrand, In Extremo, Wolfsheim, Carl Orff, Zemfira ***
 *** ... most of my work was done with their music in the background ;)     ***
 ***                                                                        ***
 ******************************************************************************
 ******************************************************************************

 LEGAL STUFF:
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 NOTE: This source is OSI-licensed. You may choose between any of the licenses
       approved by the OSI (Open Source Initiative) as long as the licensing
       of this module will not force a whole project under the chosen license.
       This basically means that the virulent character of a license (e.g. GPL)
       must no be misused to force a project to use the license you chose for
       this module!
       OSI-approved licenses can be found at this website:
	   http://www.opensource.org/licenses/

       You have the choice to make this module compatible with your own open
       source or commercial or whatever project by choosing the right license.

       However, I recommend using either of the following licenses:
       - the BSD-License (as seen below)
       - the LGPL [ http://www.opensource.org/licenses/lgpl-license.php ]
       - the MPL [ http://www.opensource.org/licenses/mozilla1.1.php ]

       !ATTENTION!: if this unit comes bundled with the files of the project
       JEDI the project license (currently MPL) is mandatory!!!
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

 Copyright (c) 1995-2005, -=Assarbad=- ["copyright holder(s)"]
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
 3. The name(s) of the copyright holder(s) may not be used to endorse or
    promote products derived from this software without specific prior written
    permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                             .oooO     Oooo.
 ____________________________(   )_____(   )___________________________________
                              \ (       ) /
                               \_)     (_/

 ******************************************************************************/


/**
 *  This code requires <windows.h> to be included .
 *  Furthermore you should turn of usage of precompiled headers for this file!
 *
 *  Finally it's necessary to set WINVER 0x0400 to remove all conflicts with
 *  Windows 2000 (and XP, and ...) with this custom function.
 *  This is self-explanatory since this function replaces an API introduced
 *  with Windows 2000, whereas the smae functionality can be achieved on NT4
 *  already. Hence this module makes only sense in conjunction with programs
 *  that try to be backwards-compatible to Windows NT 4.0!
 *
 *  This module is thought to mask out NT4/W2K differences with respect to
 *  CreateHardlink().
 *
 *  You can configure the behavior of the module by two #define statements in
 *  the Hardlink.h header file. These statements are PREFERAPI and RTDL.
 *  Refer to the header file for details.
 **/

#include <Windows.h>
#pragma warning( disable : 4005 )
#include <ntstatus.h>
#pragma warning( default : 4005 )
#include "Hardlink.h"

//#include <stdio.h>
#define dbg_printf /##/printf

/******************************************************************************
 *
 * TYPE INFORMATION (structs and enums and typedefs, defines, constants)
 *
 ******************************************************************************/

// Type for status information in the native API
typedef LONG NTSTATUS;

// This is a counted unicode character string
typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

// Constant unicode string
typedef const UNICODE_STRING *PCUNICODE_STRING;

// I/O status information
typedef struct _IO_STATUS_BLOCK {
	union {
		NTSTATUS Status;
		PVOID    Pointer;
	};
	ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

// Enum type to declare all file information classes (NT, W2K, WXP)
typedef enum _FILE_INFORMATION_CLASS {
	FileDirectoryInformation       = 1,
	FileFullDirectoryInformation,   // 2
	FileBothDirectoryInformation,   // 3
	FileBasicInformation,           // 4  wdm
	FileStandardInformation,        // 5  wdm
	FileInternalInformation,        // 6
	FileEaInformation,              // 7
	FileAccessInformation,          // 8
	FileNameInformation,            // 9
	FileRenameInformation,          // 10
	FileLinkInformation,            // 11
	FileNamesInformation,           // 12
	FileDispositionInformation,     // 13
	FilePositionInformation,        // 14 wdm
	FileFullEaInformation,          // 15
	FileModeInformation,            // 16
	FileAlignmentInformation,       // 17
	FileAllInformation,             // 18
	FileAllocationInformation,      // 19
	FileEndOfFileInformation,       // 20 wdm
	FileAlternateNameInformation,   // 21
	FileStreamInformation,          // 22
	FilePipeInformation,            // 23
	FilePipeLocalInformation,       // 24
	FilePipeRemoteInformation,      // 25
	FileMailslotQueryInformation,   // 26
	FileMailslotSetInformation,     // 27
	FileCompressionInformation,     // 28
	FileObjectIdInformation,        // 29
	FileCompletionInformation,      // 30
	FileMoveClusterInformation,     // 31
	FileQuotaInformation,           // 32
	FileReparsePointInformation,    // 33
	FileNetworkOpenInformation,     // 34
	FileAttributeTagInformation,    // 35
	FileTrackingInformation,        // 36
	FileIdBothDirectoryInformation, // 37
	FileIdFullDirectoryInformation, // 38
	FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

// Valid values for the Attributes field
#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000010L
#define OBJ_EXCLUSIVE           0x00000020L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_OPENLINK            0x00000100L
#define OBJ_KERNEL_HANDLE       0x00000200L
#define OBJ_VALID_ATTRIBUTES    0x000003F2L

// Object attributes structure
typedef struct _OBJECT_ATTRIBUTES {
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _FILE_LINK_RENAME_INFORMATION { // Info Classes 10 and 11
	BOOLEAN ReplaceIfExists;
	HANDLE RootDirectory;
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_LINK_INFORMATION, *PFILE_LINK_INFORMATION, FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;

typedef enum
{
	INVALID_PATH = 0,
	UNC_PATH,
	ABSOLUTE_DRIVE_PATH,
	RELATIVE_DRIVE_PATH,
	ABSOLUTE_PATH,
	RELATIVE_PATH,
	DEVICE_PATH,
	UNC_DOT_PATH
} DOS_PATHNAME_TYPE;

#define SYMBOLIC_LINK_QUERY (0x0001)

#define SYMBOLIC_LINK_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)

#define FILE_SYNCHRONOUS_IO_NONALERT 0x00000020
	// All operations on the file are
	// performed synchronously. Waits
	// in the system to synchronize I/O
	// queuing and completion are not
	// subject to alerts. This flag
	// also causes the I/O system to
	// maintain the file position context.
	// If this flag is set, the
	// DesiredAccess SYNCHRONIZE flag also
	// must be set.
#define FILE_OPEN_FOR_BACKUP_INTENT 0x00004000
	// The file is being opened for backup
	// intent, hence, the system should
	// check for certain access rights
	// and grant the caller the appropriate
	// accesses to the file before checking
	// the input DesiredAccess against the
	// file's security descriptor.

/******************************************************************************
 *
 * FUNCTION PROTOTYPES
 *
 ******************************************************************************/

#ifndef RTDL
#define NTDLLEXTERN extern "C"

NTDLLEXTERN
BOOLEAN
WINAPI
RtlCreateUnicodeStringFromAsciiz (
	OUT PUNICODE_STRING Destination,
	IN PCSTR Source
	);

NTDLLEXTERN
NTSTATUS
WINAPI
ZwClose (
	IN HANDLE Handle
	);

NTDLLEXTERN
NTSTATUS
WINAPI
ZwSetInformationFile (
	IN HANDLE Filehandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID FileInformation,
	IN ULONG FileInformationLength,
	IN FILE_INFORMATION_CLASS FileInformationClass
	);

NTDLLEXTERN
BOOLEAN
WINAPI
RtlPrefixUnicodeString (
	PUNICODE_STRING Prefix,
	PUNICODE_STRING ContainingString,
	BOOLEAN CaseInsensitive
	);

NTDLLEXTERN
NTSTATUS
WINAPI
ZwOpenSymbolicLinkObject (
	OUT PHANDLE SymbolicLinkHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTDLLEXTERN
NTSTATUS
WINAPI
ZwQuerySymbolicLinkObject (
	IN HANDLE SymbolicLinkHandle,
	IN OUT PUNICODE_STRING TargetName,
	OUT PULONG ReturnLength OPTIONAL
	);

NTDLLEXTERN
NTSTATUS
WINAPI
ZwOpenFile (
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG ShareAccess,
	IN ULONG OpenOptions
	);

NTDLLEXTERN
PVOID
WINAPI
RtlAllocateHeap (
	HANDLE	Heap,
	ULONG	Flags,
	ULONG	Size
	);

NTDLLEXTERN
BOOLEAN
WINAPI
RtlFreeHeap (
	HANDLE	Heap,
	ULONG	Flags,
	PVOID	Address
	);

NTDLLEXTERN
BOOLEAN
WINAPI
RtlDosPathNameToNtPathName_U (
	PCWSTR DosName,
	PUNICODE_STRING NtName,
	PWSTR *DosFilePath,
	PVOID NtFilePath // Some special structure, first member being UNICODE_STRING
	);

NTDLLEXTERN
VOID
WINAPI
RtlInitUnicodeString (
	PUNICODE_STRING	DestinationString,
	PCWSTR SourceString
	);

NTDLLEXTERN
ULONG
WINAPI
RtlDetermineDosPathNameType_U (
	PCWSTR Path
	);

NTDLLEXTERN
DWORD
WINAPI
RtlNtStatusToDosError (
	NTSTATUS StatusCode
	);

NTDLLEXTERN
BOOLEAN
WINAPI
RtlCreateUnicodeString (
	OUT PUNICODE_STRING Destination,
	IN PWSTR Source
	);

NTDLLEXTERN
VOID
WINAPI
RtlFreeUnicodeString (
	IN PUNICODE_STRING UnicodeString
	);

#else

typedef BOOLEAN (WINAPI *TFNRtlCreateUnicodeStringFromAsciiz) (
	OUT PUNICODE_STRING Destination,
	IN PCSTR Source
	);

typedef NTSTATUS (WINAPI *TFNZwClose) (
	IN HANDLE Handle
	);

typedef NTSTATUS (WINAPI *TFNZwSetInformationFile) (
	IN HANDLE Filehandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID FileInformation,
	IN ULONG FileInformationLength,
	IN FILE_INFORMATION_CLASS FileInformationClass
	);

typedef BOOLEAN (WINAPI *TFNRtlPrefixUnicodeString) (
	PUNICODE_STRING Prefix,
	PUNICODE_STRING ContainingString,
	BOOLEAN CaseInsensitive
	);

typedef NTSTATUS (WINAPI *TFNZwOpenSymbolicLinkObject) (
	OUT PHANDLE SymbolicLinkHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

typedef NTSTATUS (WINAPI *TFNZwQuerySymbolicLinkObject) (
	IN HANDLE SymbolicLinkHandle,
	IN OUT PUNICODE_STRING TargetName,
	OUT PULONG ReturnLength OPTIONAL
	);

typedef NTSTATUS (WINAPI *TFNZwOpenFile) (
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG ShareAccess,
	IN ULONG OpenOptions
	);

typedef PVOID (WINAPI *TFNRtlAllocateHeap) (
	HANDLE	Heap,
	ULONG	Flags,
	ULONG	Size
	);

typedef BOOLEAN (WINAPI *TFNRtlFreeHeap) (
	HANDLE	Heap,
	ULONG	Flags,
	PVOID	Address
	);

typedef BOOLEAN (WINAPI *TFNRtlDosPathNameToNtPathName_U) (
	PCWSTR DosName,
	PUNICODE_STRING NtName,
	PWSTR *DosFilePath,
	PVOID NtFilePath // Some special structure, first member being UNICODE_STRING
	);

typedef VOID (WINAPI *TFNRtlInitUnicodeString) (
	PUNICODE_STRING	DestinationString,
	PCWSTR SourceString
	);

typedef ULONG (WINAPI *TFNRtlDetermineDosPathNameType_U) (
	PCWSTR Path
	);

typedef DWORD (WINAPI *TFNRtlNtStatusToDosError) (
	NTSTATUS StatusCode
	);

typedef BOOLEAN (WINAPI *TFNRtlCreateUnicodeString) (
	OUT PUNICODE_STRING Destination,
	IN PWSTR Source
	);

typedef VOID (WINAPI *TFNRtlFreeUnicodeString) (
	IN PUNICODE_STRING UnicodeString
	);

///////////////////////////////////
	static TFNRtlCreateUnicodeStringFromAsciiz RtlCreateUnicodeStringFromAsciiz = NULL;
	static TFNZwClose                          ZwClose                          = NULL;
	static TFNZwSetInformationFile             ZwSetInformationFile             = NULL;
	static TFNRtlPrefixUnicodeString           RtlPrefixUnicodeString           = NULL;
	static TFNZwOpenSymbolicLinkObject         ZwOpenSymbolicLinkObject         = NULL;
	static TFNZwQuerySymbolicLinkObject        ZwQuerySymbolicLinkObject        = NULL;
	static TFNZwOpenFile                       ZwOpenFile                       = NULL;
	static TFNRtlAllocateHeap                  RtlAllocateHeap                  = NULL;
	static TFNRtlFreeHeap                      RtlFreeHeap                      = NULL;
	static TFNRtlDosPathNameToNtPathName_U     RtlDosPathNameToNtPathName_U     = NULL;
	static TFNRtlInitUnicodeString             RtlInitUnicodeString             = NULL;
	static TFNRtlDetermineDosPathNameType_U    RtlDetermineDosPathNameType_U    = NULL;
	static TFNRtlNtStatusToDosError            RtlNtStatusToDosError            = NULL;
	static TFNRtlCreateUnicodeString           RtlCreateUnicodeString           = NULL;
	static TFNRtlFreeUnicodeString             RtlFreeUnicodeString             = NULL;
	
	// Whether they have been loaded or not
	static BOOL bRtdlFunctionsLoaded = FALSE;

#endif // RTDL

#define DeclareUnicodeStringC(name, string) UNICODE_STRING name = {sizeof(string), sizeof(string), string}; RtlInitUnicodeString(&name, name.Buffer)
#define DeclareUnicodeString(name) UNICODE_STRING name = {0, 0, NULL}
#define InitUnicodeStringFromPWSTR(name, string) UNICODE_STRING name = {0, 0, NULL}; RtlInitUnicodeString(&name, string)
#define InitAnsiStringFromPSTR(name, string) UNICODE_STRING name = {0, 0, NULL}; RtlInitAnsiString(&name, string)
#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );        \
    (p)->RootDirectory = r;                           \
    (p)->Attributes = a;                              \
    (p)->ObjectName = n;                              \
    (p)->SecurityDescriptor = s;                      \
    (p)->SecurityQualityOfService = NULL;             \
    }
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

#ifdef PREFERAPI
/**
 *  These are "trampolines" into the real functions..
 *
 *  These little helpers are needed to circumvent the fact that static
 *  variables are only available in the module they are declared in.
 **/
__declspec(naked) BOOL
WINAPI
CreateHardLinkA (
	IN LPCSTR lpFileName,
	IN LPCSTR lpExistingFileName,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
	)
{
	__asm {
		jmp lpfnCreateHardLinkA
	}
}

__declspec(naked) BOOL
WINAPI
CreateHardLinkW (
	IN LPCWSTR lpFileName,
	IN LPCWSTR lpExistingFileName,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
	)
{
	__asm {
		jmp lpfnCreateHardLinkW
	}
}

#endif PREFERAPI

/******************************************************************************

 Syntax:
 -------
  C-Prototype! (if STDCALL enabled)

  BOOL WINAPI CreateHardLink(
    LPCTSTR lpFileName,
    LPCTSTR lpExistingFileName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes // Reserved; Must be NULL!

 Compatibility:
 --------------
  The function can only work on file systems that support hardlinks through the
  underlying FS driver layer. Currently this only includes NTFS on the NT
  platform (as far as I know).
  The function works fine on Windows NT4/2000/XP and is considered to work on
  future Operating System versions derived from NT (including Windows 2003).

 Remarks:
 --------
  This function tries to resemble the original CreateHardLinkW() call from
  Windows 2000/XP/2003 Kernel32.DLL as close as possible. This is why many
  functions used are NT Native API, whereas one could use Delphi or Win32 API
  functions (e.g. memory management). BUT I included much more SEH code and
  omitted extra code to free buffers and close handles. This all is done during
  the FINALLY block (so there are no memory leaks anyway ;).

  Note, that neither Microsoft's code nor mine ignore the Security Descriptor
  from the SECURITY_ATTRIBUTES structure. In both cases the security descriptor
  is passed on to ZwOpenFile()!

  The limit of 1023 hardlinks to one file is probably related to the system or
  NTFS respectively. At least I saw no special hint, why there would be such a
  limit - the original CreateHardLink() does not check the number of links!
  Thus I consider the limit being the same for the original and my rewrite.

  For the ANSI version of this function see below ...

 Remarks from the  Platform SDK:
 -------------------------------
  Any directory entry for a file, whether created with CreateFile or
  CreateHardLink, is a hard link to the associated file. Additional hard links,
  created with the CreateHardLink function, allow you to have multiple directory
  entries for a file, that is, multiple hard links to the same file. These may
  be different names in the same directory, or they may be the same (or
  different) names in different directories. However, all hard links to a file
  must be on the same volume.
  Because hard links are just directory entries for a file, whenever an
  application modifies a file through any hard link, all applications using any
  other hard link to the file see the changes. Also, all of the directory
  entries are updated if the file changes. For example, if the file's size
  changes, all of the hard links to the file will show the new size.
  The security descriptor belongs to the file to which the hard link points.
  The link itself, being merely a directory entry, has no security descriptor.
  Thus, if you change the security descriptor of any hard link, you're actually
  changing the underlying file's security descriptor. All hard links that point
  to the file will thus allow the newly specified access. There is no way to
  give a file different security descriptors on a per-hard-link basis.
  This function does not modify the security descriptor of the file to be linked
  to, even if security descriptor information is passed in the
  lpSecurityAttributes parameter.
  Use DeleteFile to delete hard links. You can delete them in any order
  regardless of the order in which they were created.
  Flags, attributes, access, and sharing as specified in CreateFile operate on
  a per-file basis. That is, if you open a file with no sharing allowed, another
  application cannot share the file by creating a new hard link to the file.

  CreateHardLink does not work over the network redirector.

  Note that when you create a hard link on NTFS, the file attribute information
  in the directory entry is refreshed only when the file is opened or when
  GetFileInformationByHandle is called with the handle of the file of interest.

 ******************************************************************************/
BOOL WINAPI
#ifndef PREFERAPI
CreateHardLinkW
#else
MyCreateHardLinkW
#endif
	(
	LPCWSTR szLinkName,
	LPCWSTR szLinkTarget,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes
	)
{
	// Declare required variables
	DeclareUnicodeString(usNtName_LinkTarget);
	DeclareUnicodeString(usNtName_LinkName);
	DeclareUnicodeString(usSymLinkDrive);
	DeclareUnicodeStringC(usCheckDrive, L"\\??\\C:\0");
	DeclareUnicodeStringC(usLanMan, L"\\Device\\LanmanRedirector\\\0");
	HANDLE hHeap = NtpGetProcessHeap();
	DWORD dwFullPath;
	NTSTATUS Status;
	PWSTR lpwszFullPath = NULL, lpwszFullPathFilePart = NULL;
	HANDLE hLinkTarget = NULL, hDrive = NULL;
	OBJECT_ATTRIBUTES oaMisc;
	IO_STATUS_BLOCK iostats;
	BOOL result = FALSE;

#ifdef RTDL
	/****************************************************************
	  Check for dynamically loaded functions
	 ****************************************************************/
	if(!bRtdlFunctionsLoaded)
	{
		dbg_printf("ERROR: ERROR_CALL_NOT_IMPLEMENTED\n");
		SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		return FALSE;
	}
#endif // RTDL
	
	/****************************************************************
	  Preliminary parameter checks which return with error code set
	 ****************************************************************/
	if((!szLinkName) || (!szLinkTarget))
	{
		// If neither is Assigned -> return with appropriate error code
		dbg_printf("ERROR: ERROR_INVALID_PARAMETER\n");
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	
	dbg_printf("szLinkName   == %ws\n", szLinkName);
	dbg_printf("szLinkTarget == %ws\n", szLinkTarget);
	
	// Determine DOS path type for both link name and target
	if((RtlDetermineDosPathNameType_U(szLinkName) == UNC_PATH) ||
		(RtlDetermineDosPathNameType_U(szLinkTarget) == UNC_PATH))
	{
		// If UNC (i.e. network) path -> return with appropriate error code
		dbg_printf("ERROR: ERROR_INVALID_NAME\n");
		SetLastError(ERROR_INVALID_NAME);
		return FALSE;
	}
	
	/****************************************************************
	  Convert the link target into a UNICODE_STRING while checking
	  path. This is the main part ...
	 ****************************************************************/
	if(RtlDosPathNameToNtPathName_U(szLinkTarget, &usNtName_LinkTarget, NULL, NULL))
	__try {
		dbg_printf("usNtName_LinkTarget = {%d, %d, %p}\n", usNtName_LinkTarget.Length, usNtName_LinkTarget.MaximumLength, usNtName_LinkTarget.Buffer);
		dbg_printf("hHeap == %.8X\n", hHeap);

		// Determine size needed to convert path to full path
		if(dwFullPath = GetFullPathNameW(szLinkTarget, 0, NULL, NULL))
		{
			dbg_printf("dwFullPath == %d\n", dwFullPath);
			// Calculate needed size (in TCHARs)
			dwFullPath += 2;
			dbg_printf("dwFullPath == %d\n", dwFullPath);
			// Allocate enough memory
			if(lpwszFullPath = (PWSTR)RtlAllocateHeap(hHeap, HEAP_ZERO_MEMORY, dwFullPath * sizeof(WCHAR)))
			__try {
				// Convert the path
				if(GetFullPathNameW(szLinkTarget, dwFullPath, lpwszFullPath, &lpwszFullPathFilePart))
				{
					/****************************************************************
					  Preparation of the checking for mapped network drives
					 ****************************************************************/
					dbg_printf("usCheckDrive = {%d, %d, %p}\n", usCheckDrive.Length, usCheckDrive.MaximumLength, usCheckDrive.Buffer);
					dbg_printf("Before usCheckDrive == %ws\n", usCheckDrive.Buffer);
					// Replace drive letter by the drive letter we want
					usCheckDrive.Buffer[4] = lpwszFullPath[0];
					dbg_printf("After  usCheckDrive == %ws\n", usCheckDrive.Buffer);
					// Init OBJECT_ATTRIBUTES
					InitializeObjectAttributes(&oaMisc, &usCheckDrive, OBJ_CASE_INSENSITIVE, NULL, NULL);
					/****************************************************************
					  Checking for (illegal!) mapped network drives
					  Open symbolic link object for the drive and check whether the
					  symbolic link is prefixed with "\Device\LanmanRedirector\".
					 ****************************************************************/
					if(NT_SUCCESS(Status = ZwOpenSymbolicLinkObject(&hDrive, SYMBOLIC_LINK_QUERY, &oaMisc)))
					__try {
						dbg_printf("ZwOpenSymbolicLinkObject() == %.8X\n", Status);
						usSymLinkDrive.MaximumLength = MAX_PATH * sizeof(WCHAR);
						// Allocate memory to hold the name of the symlink target
						if(usSymLinkDrive.Buffer = (PWSTR)RtlAllocateHeap(hHeap, HEAP_ZERO_MEMORY, usSymLinkDrive.MaximumLength))
						__try {
							dbg_printf("Going to query %ws\n", usCheckDrive.Buffer);
							dbg_printf("usSymLinkDrive = {%d, %d, %p}\n", usSymLinkDrive.Length, usSymLinkDrive.MaximumLength, usSymLinkDrive.Buffer);
							// Query the path the symbolic link points to ...
							if(NT_SUCCESS(Status = ZwQuerySymbolicLinkObject(hDrive, &usSymLinkDrive, NULL)))
							{
								dbg_printf("ZwQuerySymbolicLinkObject() == %.8X\n", Status);
								dbg_printf("lpwszFullPath == %ws\n", lpwszFullPath);
								// Initialise the length members
								dbg_printf("usLanMan = {%d, %d, %p}\n", usLanMan.Length, usLanMan.MaximumLength, usLanMan.Buffer);
								dbg_printf("usLanMan == %ws\n", usLanMan.Buffer);
								
								dbg_printf("usSymLinkDrive = {%d, %d, %p}\n", usSymLinkDrive.Length, usSymLinkDrive.MaximumLength, usSymLinkDrive.Buffer);
								dbg_printf("usSymLinkDrive == %ws\n", usSymLinkDrive.Buffer);
								// The path must not be a mapped drive ... check this!
								// NB: This is done by comparison of prefixes
								if(!RtlPrefixUnicodeString(&usLanMan, &usSymLinkDrive, TRUE))
								{
									dbg_printf("Going to open that file ...\n");
									// Initialise OBJECT_ATTRIBUTES
									InitializeObjectAttributes(&oaMisc, &usNtName_LinkTarget, OBJ_CASE_INSENSITIVE, NULL, (lpSecurityAttributes) ? lpSecurityAttributes->lpSecurityDescriptor : NULL);
									/****************************************************************
									  Open existing target file and prepare for processing
									 ****************************************************************/
									if(NT_SUCCESS(ZwOpenFile(&hLinkTarget,
										DELETE | SYNCHRONIZE,
										&oaMisc,
										&iostats,
										FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
										FILE_FLAG_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
										)))
									__try {
										dbg_printf("%.8X - %.8X - %.8X\n", DELETE | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_FLAG_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);
										dbg_printf("OPENED SUCCESSFULLY\n");
										// Wow ... target opened ... let's try to convert the
										// name of the hardlink to be created into NT style
										if(RtlDosPathNameToNtPathName_U(szLinkName, &usNtName_LinkName, NULL, NULL))
										__try {
											// Now almost everything is done to create a link!
											dbg_printf("sizeof(FILE_LINK_INFORMATION) == %d\n", sizeof(FILE_LINK_INFORMATION));
											DWORD dwSize = sizeof(FILE_LINK_INFORMATION) + usNtName_LinkName.Length + sizeof(WCHAR);
											dbg_printf("usNtName_LinkName   == %ws (%d)\n", usNtName_LinkName.Buffer, usNtName_LinkName.Length);
											dbg_printf("usNtName_LinkTarget == %ws (%d)\n", usNtName_LinkTarget.Buffer, usNtName_LinkTarget.Length);
											// Allocate buffer for FILE_LINK_INFORMATION+Filename
											if(
											PFILE_LINK_INFORMATION lpFileLinkInfo =
												(PFILE_LINK_INFORMATION)RtlAllocateHeap(hHeap, HEAP_ZERO_MEMORY, dwSize)
												)
											__try {
												/****************************************************************
												  Core to create actual hardlink
												 ****************************************************************/
												dbg_printf("%p\n", lpFileLinkInfo);
												// Fill the FILE_LINK_INFORMATION structure
												lpFileLinkInfo->ReplaceIfExists = FALSE;
												lpFileLinkInfo->RootDirectory = NULL;
												lpFileLinkInfo->FileNameLength = usNtName_LinkName.Length;
												memcpy(lpFileLinkInfo->FileName, usNtName_LinkName.Buffer, lpFileLinkInfo->FileNameLength);
												dbg_printf("%ws -> %d\n", lpFileLinkInfo->FileName, wcslen(usNtName_LinkName.Buffer));
												dbg_printf("%X\n", dwSize);
												memset(&iostats, 0, sizeof(iostats));
												// Set this information class
												result = NT_SUCCESS(Status = ZwSetInformationFile(hLinkTarget, &iostats, lpFileLinkInfo, dwSize, FileLinkInformation));
												dbg_printf("ZwSetInformationFile() == %.8X\n", Status);
											}
											__finally {
												// Free the buffer
												RtlFreeHeap(hHeap, 0, lpFileLinkInfo);
												// Set last error code
												SetLastError(RtlNtStatusToDosError(Status));
											}
										}
										__finally {
											RtlFreeHeap(hHeap, 0, usNtName_LinkName.Buffer);
										}
										else
										{
											SetLastError(ERROR_INVALID_NAME);
											return FALSE;
										}
									}
									__finally {
										ZwClose(hLinkTarget);
									}
								}
								else
								{
									dbg_printf("ERROR: ERROR_INVALID_NAME\n");
									SetLastError(ERROR_INVALID_NAME);
									return FALSE;
								}
							}
							else
							{
								dbg_printf("COULD NOT QUERY SYMBOLIC LINK!!! %.8X\n", Status);
								SetLastError(RtlNtStatusToDosError(Status));
								return FALSE;
							}
						}
						__finally {
							RtlFreeHeap(hHeap, 0, usSymLinkDrive.Buffer);
						}
						else
						{
							dbg_printf("ERROR: ERROR_NOT_ENOUGH_MEMORY\n");
							SetLastError(ERROR_NOT_ENOUGH_MEMORY);
							return FALSE;
						}
					}
					__finally {
						ZwClose(hDrive);
					}
					else
					{
						dbg_printf("COULD NOT OPEN SYMBOLIC LINK!!! %.8X\n", Status);
						SetLastError(RtlNtStatusToDosError(Status));
						return FALSE;
					}
				}
			}
			__finally {
				RtlFreeHeap(hHeap, 0, lpwszFullPath);
			}
			else
			{
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				return FALSE;
			}
		}			
	}
	__finally {
		RtlFreeHeap(hHeap, 0, usNtName_LinkTarget.Buffer);
	}
	else
	{
		// If path is invalid or does not exist -> return with appropriate error code
		SetLastError(ERROR_PATH_NOT_FOUND);
		return FALSE;
	}

	return result;
}

/******************************************************************************
 Hint:
 -----
  For all closer information see the CreateHardLinkW function above.

 Specific to the ANSI-version:
 -----------------------------
  The ANSI-Version can be used as if it was used on Windows 2000. This holds
  for all supported systems for now.

 ******************************************************************************/
BOOL WINAPI
#ifndef PREFERAPI
CreateHardLinkA
#else
MyCreateHardLinkA
#endif
	(
	LPCSTR szLinkName,
	LPCSTR szLinkTarget,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes
	)
{
#ifdef RTDL
	if(!bRtdlFunctionsLoaded)
		return FALSE;
#endif // RTDL

	DeclareUnicodeString(usLinkName);
	DeclareUnicodeString(usLinkTarget);

	// Create and allocate a UNICODE_STRING from the zero-terminated parameters
	if(szLinkName)
		RtlCreateUnicodeStringFromAsciiz(&usLinkName, szLinkName);
	if(szLinkTarget)
		RtlCreateUnicodeStringFromAsciiz(&usLinkTarget, szLinkTarget);

	BOOL result = CreateHardLinkW(usLinkName.Buffer, usLinkTarget.Buffer, lpSecurityAttributes);

	// Free the allocated buffer
	if(usLinkTarget.Buffer)
		RtlFreeHeap(NtpGetProcessHeap(), 0, usLinkTarget.Buffer);
	// Free the allocated buffer
	if(usLinkName.Buffer)
		RtlFreeHeap(NtpGetProcessHeap(), 0, usLinkName.Buffer);

	// Return the result of CreateHardLinkW()
	return result;
}

BOOL Hardlink_Initialize()
{
#define InitApi(h, x) x = (TFN##x)GetProcAddress(h, #x)
#define InitApi2(h, x) lpfn##x = (TFN##x)GetProcAddress(h, #x)

#ifdef PREFERAPI
	HMODULE hKernel32 = GetModuleHandleW(TEXT("kernel32.dll"));
	if(hKernel32)
	{
		InitApi2(hKernel32, CreateHardLinkA);
		InitApi2(hKernel32, CreateHardLinkW);
	}
	if(lpfnCreateHardLinkA && lpfnCreateHardLinkW)
	{
		dbg_printf("Retrieved fct. addresses:\n  CreateHardLinkA == %p; CreateHardLinkW == %p\n", lpfnCreateHardLinkA, lpfnCreateHardLinkW);
		return TRUE;
	}
#endif // PREFERAPI

#ifdef RTDL
	HMODULE hNtDll = GetModuleHandleW(TEXT("ntdll.dll"));
	if(hNtDll)
	{
		InitApi(hNtDll, RtlCreateUnicodeStringFromAsciiz);
		InitApi(hNtDll, ZwClose);
		InitApi(hNtDll, ZwSetInformationFile);
		InitApi(hNtDll, RtlPrefixUnicodeString);
		InitApi(hNtDll, ZwOpenSymbolicLinkObject);
		InitApi(hNtDll, ZwQuerySymbolicLinkObject);
		InitApi(hNtDll, ZwOpenFile);
		InitApi(hNtDll, RtlAllocateHeap);
		InitApi(hNtDll, RtlFreeHeap);
		InitApi(hNtDll, RtlDosPathNameToNtPathName_U);
		InitApi(hNtDll, RtlInitUnicodeString);
		InitApi(hNtDll, RtlDetermineDosPathNameType_U);
		InitApi(hNtDll, RtlNtStatusToDosError);
		InitApi(hNtDll, RtlCreateUnicodeString);
		InitApi(hNtDll, RtlFreeUnicodeString);
	}
	bRtdlFunctionsLoaded = (
		(RtlCreateUnicodeStringFromAsciiz != NULL) &
		(ZwClose != NULL) &
		(ZwSetInformationFile != NULL) &
		(RtlPrefixUnicodeString != NULL) &
		(ZwOpenSymbolicLinkObject != NULL) &
		(ZwQuerySymbolicLinkObject != NULL) &
		(ZwOpenFile != NULL) &
		(RtlAllocateHeap != NULL) &
		(RtlFreeHeap != NULL) &
		(RtlDosPathNameToNtPathName_U != NULL) &
		(RtlInitUnicodeString != NULL) &
		(RtlDetermineDosPathNameType_U != NULL) &
		(RtlNtStatusToDosError != NULL) &
		(RtlCreateUnicodeString != NULL) &
		(RtlFreeUnicodeString != NULL)
		);
#endif // RTDL

#ifdef RTDL
#ifdef PREFERAPI
	if(bRtdlFunctionsLoaded)
	{
		lpfnCreateHardLinkA = MyCreateHardLinkA;
		lpfnCreateHardLinkW = MyCreateHardLinkW;
	}
#endif // PREFERAPI
	return bRtdlFunctionsLoaded;
#else // RTDL
		lpfnCreateHardLinkA = MyCreateHardLinkA;
		lpfnCreateHardLinkW = MyCreateHardLinkW;

		return TRUE;
#endif // RTDL
}

__declspec(naked) inline HANDLE NtpGetProcessHeap()
{
	// The actual declarations of the PEB and TIB/TEB structures has been
	// left out since they are not necessary for the functionality.
	__asm{
		// MOV  EAX, FS:[0]._TEB.Peb // FS points to TEB/TIB which has a pointer to the PEB
		mov     eax, fs:0x30
		// MOV  EAX, [EAX]._PEB.ProcessHeap // Get the process heap's handle
		mov     eax, [eax+0x18]
		retn
	}
}

/******************************************************************************

  Some warnings the user should see when compiling under certain conditions!

 ******************************************************************************/

#if defined(RTDL) || defined(PREFERAPI)
	#pragma message("_________________________________________________________________________________________")
	#pragma message("ATTENTION: Did call Hardlink_Initialize() before calling CreateHardLink() in your code???")
#endif // defined(RTDL) || defined(PREFERAPI)

#ifndef RTDL
	#pragma message("_________________________________________________________________________________________")
	#pragma message("WARNING:   You'll need to get ntdll.lib (from the DDK) to link to ntdll.dll statically!!!")
	#pragma comment(lib,"ntdll.lib")
#endif // RTDL

#if (_WIN32_WINNT > 0x0400)
	#pragma message("_________________________________________________________________________________________")
	#pragma message("WARNING:   This module exists for compatibility purposes with NT 4.0!")
	#pragma message("           You defined a WINVER and _WIN32_WINNT higher than 0x0400, so there will be")
	#pragma message("           conflicts with the header files from the SDK which define CreateHardLink().")

#endif
