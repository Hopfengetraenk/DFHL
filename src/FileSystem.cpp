/* FileSystem.cpp : File System specific function for DFHL

DFHL - Duplicate File Hard Linker, a small tool to gather some space
    from duplicate files on your hard disk
Copyright (C) 2004-2012 Jens Scheffler & Oliver Schneider & Hans Schmidts

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <stdarg.h>
#include "Util.h"
#include "FileSystem.h"
#include "Hardlink.h"

#ifndef INVALID_SET_FILE_POINTER
  #define INVALID_SET_FILE_POINTER  0xFFFFFFFF  /* missing in MS-VC 6.0 !? */
#endif


/** Flag if list of hard linking should be suppredded (2012-10-12  HaSchm) */
bool boNoFileNameLog = false;


bool FolderEntry::isFile() {
    return this->file;
}

bool FolderEntry::isFolder() {
    return !this->file;
}

Path::Path(LPWSTR newName, Path* newParent) {
    Statistics::getInstance()->pathObjCreated++;
    classType = CLASS_TYPE_PATH;
    file = false;
    name = new WCHAR[wcslen(newName) + 1];
    wcscpy(name, newName);
    parent = newParent;
    if (newParent != NULL) {
        parent->addReference();
    }
}

Path::~Path() {
    Statistics::getInstance()->pathObjDestroyed++;
    delete name;
    if (parent != NULL) {
        parent->removeReference();
    }
}

void Path::copyName(LPWSTR buffer) {
    // recursively copy the parent path
    if (parent != NULL) {
        parent->copyName(buffer);
        wcscat(buffer, PATH_SEPARATOR); // append path delimiter...
    }
    wcscat(buffer, name);
}

bool Path::equals(Path* otherPath) {
    if (otherPath == NULL) {
        return false;
    }
    if (wcscmp(name, otherPath->name) != 0) {
        return false;
    }
    if (parent != NULL) {
        return parent->equals(otherPath->parent);
    }
    return true;
}


File::File(WIN32_FIND_DATA fileInfo, Path* newParent) {
    Statistics::getInstance()->i64FileObjCreated++;
    classType = CLASS_TYPE_FILE;
    file = true;
    name = new WCHAR[wcslen(fileInfo.cFileName) + 1];
    wcscpy(name, fileInfo.cFileName);
    shortName = new WCHAR[wcslen(fileInfo.cAlternateFileName) + 1];
    wcscpy(shortName, fileInfo.cAlternateFileName);
    parentFolder = newParent;
    parentFolder->addReference();
    i64Size = fileInfo.nFileSizeLow + ((INT64)MAXDWORD + 1) * fileInfo.nFileSizeHigh;
    attributes = fileInfo.dwFileAttributes;
    lastModifyTime = fileInfo.ftLastWriteTime;
    u32Hash = 0u;
    boHashAvailable = false;
    boIsDup = false;
}

File::~File() {
    Statistics::getInstance()->i64FileObjDestroyed++;
    delete name;
    delete shortName;
    parentFolder->removeReference();
}

void File::copyName(LPWSTR buffer) {
    // recursively copy the parent path
    parentFolder->copyName(buffer);
    wcscat(buffer, PATH_SEPARATOR); // append path delimiter...
    wcscat(buffer, name);
}

LPWSTR File::getName() {
    return name;
}

LPWSTR File::getShortName() {
    return shortName;
}

INT64 File::i64GetSize() {
    return i64Size;
}

DWORD File::getAttributes() {
    return attributes;
}

FILETIME File::getLastModifyTime() {
    return lastModifyTime;
}

bool File::boIsHashAvailable() {
    return boHashAvailable;
}

DWORD File::u32GetHash() {
    return u32Hash;
}

void File::vSetHash(DWORD u32NewHash) {
    boHashAvailable = true;
    u32Hash = u32NewHash;
}

bool File::equals(File* otherFile) {
    if (otherFile == NULL) {
        return false;
    }
    if (wcscmp(name, otherFile->name) != 0) {
        return false;
    }
    return parentFolder->equals(otherFile->parentFolder);
}

UnbufferedFileStream::UnbufferedFileStream(File* newFile) {
    Statistics::getInstance()->i64UnbufferedFileStreamObjCreated++;
    file = newFile;
}

UnbufferedFileStream::~UnbufferedFileStream() {
    Statistics::getInstance()->i64UnbufferedFileStreamObjDestroyed++;
}

bool UnbufferedFileStream::open() {
    Statistics::getInstance()->i64FilesOpened++;
    LPWSTR fileName = new WCHAR[MAX_PATH_LENGTH];
    file->copyName(cleanString(fileName));
    hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    delete fileName;
    if (hFile == INVALID_HANDLE_VALUE) Statistics::getInstance()->fileOpenProblems++;
    return hFile != INVALID_HANDLE_VALUE;
}

int UnbufferedFileStream::read(LPVOID buffer, int bytesToRead) {
    DWORD dwResult;
    ReadFile(hFile, buffer, bytesToRead, &dwResult, NULL);
    /* Unbuffered ReadFile always reads complete sectors. Content of extra
       data behind current length is not specified (0x00 in many cases).
       Result is real rest of file at the end (not rounded up). */
    Statistics::getInstance()->i64BytesRead+=dwResult;
    return (int)dwResult;
}

int UnbufferedFileStream::skip(int bytesToSkip) {
    LARGE_INTEGER liSize,liPos;
    int iResult;
    DWORD dwError;

    liSize.LowPart = GetFileSize(hFile, (LPDWORD)&liSize.HighPart);
    if (liSize.LowPart == INVALID_FILE_SIZE
     && (dwError = GetLastError()) != NO_ERROR ) {
        LPWSTR buffer = new WCHAR[MAX_PATH_LENGTH];
        wsprintf(buffer, L"GetFileSize error. Error is %u\n", dwError);
        throw buffer;
    }
    liPos.HighPart = 0;
    liPos.LowPart = SetFilePointer(hFile, 0, &liPos.HighPart, FILE_CURRENT);
    if (liPos.LowPart == INVALID_SET_FILE_POINTER
     && (dwError = GetLastError()) != NO_ERROR ) {
        LPWSTR buffer = new WCHAR[MAX_PATH_LENGTH];
        wsprintf(buffer, L"SetFilePointer error. Error is %u\n", dwError);
        throw buffer;
    }
    iResult = bytesToSkip;
    if (liPos.QuadPart+bytesToSkip > liSize.QuadPart) {
      /* result is rest of file like read() */
      iResult = (int)(liSize.QuadPart - liPos.QuadPart);
      /* bytesToSkip must be rounded up */
      bytesToSkip = ((unsigned)(iResult+(MAX_SECTOR_SIZE-1))&(((unsigned)(MAX_SECTOR_SIZE-1))^(unsigned)-1));
    }
    liPos.HighPart = 0;
    liPos.LowPart = SetFilePointer(hFile, bytesToSkip, &liPos.HighPart, FILE_CURRENT);
    if (liPos.LowPart == INVALID_SET_FILE_POINTER
     && (dwError = GetLastError()) != NO_ERROR ) {
        LPWSTR buffer = new WCHAR[MAX_PATH_LENGTH];
        wsprintf(buffer, L"SetFilePointer error. Error is %u\n", dwError);
        throw buffer;
    }
    return iResult;
}

bool UnbufferedFileStream::getFileDetails(BY_HANDLE_FILE_INFORMATION* info) {
    return GetFileInformationByHandle(hFile, info) == TRUE;
}

void UnbufferedFileStream::close() {
    Statistics::getInstance()->i64FilesClosed++;
    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;
}


FileSystem::FileSystem() {
    Statistics::getInstance()->i64FileSystemObjCreated++;
    followJunctions = false;
    hiddenFiles = false;
    systemFiles = false;

    // Iinit the Hard Linking functionality in NT4.0
    if(!Hardlink_Initialize())
    {
        // Error ... probably not on NT/2000/XP or any other NT system ...
        logError(ERROR_CALL_NOT_IMPLEMENTED, L"Could not initialize required functions!\n");
        logInfo(L"NOTE: You need to run this program in Windows NT 4.0 or greater!");
    }
}

FileSystem::~FileSystem() {
    Statistics::getInstance()->i64FileSystemObjDestroyed++;
}

Path* FileSystem::parsePath(LPCWSTR pathStr) {
    // first erase tailing path separators
    while (wcslen(pathStr) > 0 && pathStr[wcslen(pathStr) - 1] == L'\\') {
        (&pathStr)[wcslen(pathStr) - 1] = L'\0';
    }

    // make the path absolute
    LPWSTR absolutePath = new WCHAR[MAX_PATH_LENGTH];
    LPWSTR somePointer;
    GetFullPathName(pathStr, MAX_PATH_LENGTH, absolutePath, &somePointer);

    // add the \\?\ prefix to support extra long paths
    LPWSTR extendedPath = new WCHAR[MAX_PATH_LENGTH];
    wcscpy(extendedPath, LONG_FILENAME_PREFIX);
    wcscat(extendedPath, absolutePath);

    // get the long file name of the path
    LPWSTR longPath = new WCHAR[MAX_PATH_LENGTH];
    GetLongPathName(extendedPath, longPath, MAX_PATH_LENGTH);

    // check if we got something valid...
    Path* result = NULL;
    if (wcslen(longPath) == 0) {
        logError(L"Path \"%s\" is not existing or accessible!", pathStr);
        Statistics::getInstance()->directoryOpenProblems++; /* 2012-10-16  HaSchm */
    } else {
        // convert the string into a Path object
        result = new Path(longPath, NULL);
        Statistics::getInstance()->directoriesFound++;
    }

    // clean up memory
    delete absolutePath;
    delete extendedPath;
    delete longPath;
    return result;
}

Collection* FileSystem::getFolderContent(Path* folder) {

    WIN32_FIND_DATA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WCHAR dirSpec[MAX_PATH_LENGTH];
    DWORD dwError;
    Collection* result = new Collection();

    // prepare the folder name and append "\*"
    folder->copyName(cleanString(dirSpec));
    wcscat(dirSpec, L"\\*");

    hFind = FindFirstFile(dirSpec, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        // Accessing "<drive>:\System Volume Information\*" gives an
        // ERROR_ACCESS_DENIED. So this has to be fixed to scan whole
        // volumes! Also can happen on folders with no access permissions.
        /* 2012-10-15  HaSchm  logError(GetLastError(), L"Unable to read folder content."); */
        logError(GetLastError(), L"Unable to read content of folder \"%s\".",dirSpec);  /* 2012-10-15  HaSchm */
        Statistics::getInstance()->directoryOpenProblems++; /* 2012-10-16  HaSchm */

    } else {
        do {
            // check if this is a valid file and not a dummy like "." or ".."
            if (wcscmp(findFileData.cFileName, L".") != 0 &&  wcscmp(findFileData.cFileName, L"..") != 0) {

                // check if we have found a file
                if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    Statistics::getInstance()->i64FilesFound++;

                    // check for hidden or system file
                    if ((findFileData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM)) != 0u) {
                        Statistics::getInstance()->i64HSFilesFound++; /* 2012-10-16  HaSchm */

                        // check for hidden file
                        if (!hiddenFiles && (findFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {
                            logDebug(L"ignoring hidden file \"%s\"", findFileData.cFileName);
                            continue;
                        }
                        // check for system file
                        if (!systemFiles && (findFileData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)) {
                            logDebug(L"ignoring system file \"%s\"", findFileData.cFileName);
                            continue;
                        }

                    } /* hidden or system */

                    // add the path to the collection
                    logFile(findFileData);

                    File* item = new File(findFileData, folder);
                    result->append(item);
                    continue;
                }
                // Okay we found a directory!
                Statistics::getInstance()->directoriesFound++;

                // check for junction
                if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)!=0u) {
                    Statistics::getInstance()->junctionsFound++;
                    if (!followJunctions) {
                        logDebug(L"ignoring junction \"%s\"", findFileData.cFileName);
                        continue;
                    }
                }

                // add the path to the collection
                logFile(findFileData);
                result->append(new Path(findFileData.cFileName, folder));
            } /* not ".", not ".." */
        } while (FindNextFile(hFind, &findFileData) != 0);

        dwError = GetLastError();
        FindClose(hFind);
        if (dwError != ERROR_NO_MORE_FILES) {
            LPWSTR buffer = new WCHAR[MAX_PATH_LENGTH];
            wsprintf(buffer, L"FindNextFile error. Error is %u\n", dwError);
            // clean up memory
            delete result;
            throw buffer;
        }
    }

    return result;
}

bool FileSystem::hardLinkFiles(File* file1, File* file2) {
    Statistics::getInstance()->i64HardLinks++;
    LPWSTR file1Name = new WCHAR[MAX_PATH_LENGTH];
    LPWSTR file2Name = new WCHAR[MAX_PATH_LENGTH];
    LPWSTR file2Backup = new WCHAR[MAX_PATH_LENGTH];

    file1->copyName(cleanString(file1Name));
    file2->copyName(cleanString(file2Name));
    if (!boNoFileNameLog) logInfo(L"Linking %s and %s", file1Name, file2Name);

    // Step 1: precaution - rename original file
    wsprintf(file2Backup, L"%s.DfhlBackup", file2Name);
    if (!MoveFile(file2Name, file2Backup)) {
        if (logLevel>LOG_INFO || boNoFileNameLog || !boNoStdErr) {
            logErrorInfo(L"Linking %s and %s", file1Name, file2Name);  /* 2012-10-15  HaSchm */
        }
        logError(L"Unable to rename to backup (*.DfhlBackup): %i", GetLastError());
        delete file1Name;
        delete file2Name;
        delete file2Backup;
        return false;
    }

    // Step 2: create hard link
    if (!CreateHardLink(file2Name, file1Name, NULL)) {
        if (logLevel>LOG_INFO || boNoFileNameLog || !boNoStdErr) {
            logErrorInfo(L"Linking %s and %s", file1Name, file2Name);  /* 2012-10-15  HaSchm */
        }
        logError(L"Unable to create hard link: %i", GetLastError());

        if (!MoveFile(file2Backup, file2Name)) {
            logError(L"Unable to restore (rename) from backup (*.DfhlBackup): %i", GetLastError());
        }

        delete file1Name;
        delete file2Name;
        delete file2Backup;
        return false;
    }

    // Step 3: remove backup file (orphan)
    if (!DeleteFile(file2Backup)) {
        // 2012-02-06  HaSchm  logError(L"Unable to delete file: %i, trying to change attribute...", GetLastError());

        if (!SetFileAttributes(file2Backup, FILE_ATTRIBUTE_NORMAL) ||
            !DeleteFile(file2Backup)) {

            if (logLevel>LOG_INFO || boNoFileNameLog || !boNoStdErr) {
                logErrorInfo(L"Linking %s and %s", file1Name, file2Name); /* 2012-10-15  HaSchm */
            }
            logError(L"Finally unable to delete file (*.DfhlBackup): %i", GetLastError());
            delete file1Name;
            delete file2Name;
            delete file2Backup;
            return false;
        }
    }

    // Step 4: re-assign the old short file name
    // NOTE: This seems to be NOT possible! THis seems to be a NTFS Limitation, no hard link alternative file names are possible!
    //if (file2->getShortName() != NULL) {
    //
    //  HANDLE hFile = CreateFile(file2Name, GENERIC_ALL, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    //  if (!SetFileShortName(hFile, file2->getShortName())) {
    //      logError(L"Unable to set the files short name to: \"%s\" Error: %i", file2->getShortName(), GetLastError());
    //  }
    //  CloseHandle(hFile);
    //}

    delete file1Name;
    delete file2Name;
    delete file2Backup;
    Statistics::getInstance()->i64HardLinksSuccess++;
    return true;
}

void FileSystem::setFollowJunctions(bool newValue) {
    followJunctions = newValue;
}

void FileSystem::setHiddenFiles(bool newValue) {
    hiddenFiles = newValue;
}

void FileSystem::setSystemFiles(bool newValue) {
    systemFiles = newValue;
}
