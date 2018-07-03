/* Util.cpp : Utility function for DFHL

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

// Global Variables
// *******************************************

/** Flag to write error messages to standard output (2012-10-15  HaSchm) */
bool boNoStdErr = false;


/** Logging Level for the console output */
int logLevel = LOG_INFO;


void logError(LPCWSTR message, ...) {
    va_list argp;
    FILE *fiErrOut = stderr;
    if (boNoStdErr) fiErrOut = stdout;  /* 2012-10-15  HaSchm */

    fwprintf(fiErrOut, L"ERROR: ");
    va_start(argp, message);
    vfwprintf(fiErrOut, message, argp);
    va_end(argp);
    fwprintf(fiErrOut, L"\n");
}

void logError(DWORD errNumber, LPCWSTR message, ...) {
    va_list argp;
    FILE *fiErrOut = stderr;
    if (boNoStdErr) fiErrOut = stdout;  /* 2012-10-15  HaSchm */

    fwprintf(fiErrOut, L"ERROR: ");
    va_start(argp, message);
    vfwprintf(fiErrOut, message, argp);
    va_end(argp);
    LPWSTR msgBuffer = NULL;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        errNumber,
        GetSystemDefaultLangID(),
        (LPWSTR)(&msgBuffer),
        0,
        NULL);
    fwprintf(fiErrOut, L" -> [%i] %s\n", errNumber, msgBuffer);
    LocalFree(msgBuffer);
}

void logErrorInfo(LPCWSTR message, ...) {  /* 2012-10-15  HaSchm */
    va_list argp;
    FILE *fiErrOut = stderr;
    if (boNoStdErr) fiErrOut = stdout;

    va_start(argp, message);
    vfwprintf(fiErrOut, message, argp);
    va_end(argp);
    fwprintf(fiErrOut, L"\n");
}

void logInfo(LPCWSTR message, ...) {
    if (logLevel <= LOG_INFO) {
        va_list argp;
        va_start(argp, message);
        vwprintf(message, argp);
        va_end(argp);
        wprintf(L"\n");
    }
}

void logVerbose(LPCWSTR message, ...) {
    if (logLevel <= LOG_VERBOSE) {
        va_list argp;
        wprintf(L"  ");
        va_start(argp, message);
        vwprintf(message, argp);
        va_end(argp);
        wprintf(L"\n");
    }
}
void logDebug(LPCWSTR message, ...) {
    if (logLevel <= LOG_DEBUG) {
        va_list argp;
        wprintf(L"    ");
        va_start(argp, message);
        vwprintf(message, argp);
        va_end(argp);
        wprintf(L"\n");
    }
}

void setLogLevel(int newLevel) {
    logLevel = newLevel;
}

LPWSTR cleanString(LPWSTR string) {
    string[0] = 0;
    return string;
}

void logFile(WIN32_FIND_DATA FileData) {
    INT64 i64Size = FileData.nFileSizeLow + ((INT64)MAXDWORD + 1) * FileData.nFileSizeHigh;
    logDebug(L"Found file \"%s\" (Size=%I64i,%s%s%s%s%s%s%s%s%s%s%s%s)",
        FileData.cFileName,
        i64Size,
        FILE_ATTRIBUTE_ARCHIVE      &FileData.dwFileAttributes?L"ARCHIVE ":L"",
        FILE_ATTRIBUTE_COMPRESSED   &FileData.dwFileAttributes?L"COMPRESSED ":L"",
        FILE_ATTRIBUTE_DIRECTORY    &FileData.dwFileAttributes?L"DIRECTORY ":L"",
        FILE_ATTRIBUTE_ENCRYPTED    &FileData.dwFileAttributes?L"ENCRYPTED ":L"",
        FILE_ATTRIBUTE_HIDDEN       &FileData.dwFileAttributes?L"HIDDEN ":L"",
        FILE_ATTRIBUTE_NORMAL       &FileData.dwFileAttributes?L"NORMAL ":L"",
        FILE_ATTRIBUTE_OFFLINE      &FileData.dwFileAttributes?L"OFFLINE ":L"",
        FILE_ATTRIBUTE_READONLY     &FileData.dwFileAttributes?L"READONLY ":L"",
        FILE_ATTRIBUTE_REPARSE_POINT&FileData.dwFileAttributes?L"REPARSE_POINT ":L"",
        FILE_ATTRIBUTE_SPARSE_FILE  &FileData.dwFileAttributes?L"SPARSE ":L"",
        FILE_ATTRIBUTE_SYSTEM       &FileData.dwFileAttributes?L"SYSTEM ":L"",
        FILE_ATTRIBUTE_TEMPORARY    &FileData.dwFileAttributes?L"TEMP ":L"");
}

static Statistics* instance;
static int instanceFlag;

Statistics* Statistics::getInstance() {
    if (instanceFlag != 1234) {
        instance = new Statistics();
        instanceFlag = 1234;
    }
    return instance;
}

Statistics::Statistics() {
    i64FileCompares = 0;
    i64HashCompares = 0;
    i64HashComparesHit1 = 0;
    i64HashComparesHit2 = 0;
    i64HashComparesHit3 = 0;
    /* i64FileMetaDataMismatch = 0; 2012-10-12  HaSchm */
    i64FileNameMismatch = 0;
    i64FileAttributeMismatch = 0;
    i64FileMTimeMismatch = 0;

    i64FileAlreadyLinked = 0;
    i64FileContentDifferFirstBlock = 0;
    i64FileContentDifferLater = 0;
    i64FileContentSame = 0;
    fileCompareProblems = 0;
    i64HashesCalculated = 0;
    directoryOpenProblems = 0; /* 2012-10-16  HaSchm */
    directoriesFound = 0;
    junctionsFound = 0;  /* 2012-10-16  HaSchm */
    i64FilesFound = 0;
    i64HSFilesFound = 0;
    i64BytesRead = 0;
    i64FilesOpened = 0;
    i64FilesClosed = 0;
    fileOpenProblems = 0;
    pathObjCreated = 0;
    pathObjDestroyed = 0;
    i64FileObjCreated = 0;
    i64FileObjDestroyed = 0;
    i64UnbufferedFileStreamObjCreated = 0;
    i64UnbufferedFileStreamObjDestroyed = 0;
    i64FileSystemObjCreated = 0;
    i64FileSystemObjDestroyed = 0;
    i64HardLinks = 0;
    i64HardLinksSuccess = 0;
    i64BytesSaved = 0;  /* 2012-10-16  HaSchm */
    i64CollectionObjCreated = 0;
    i64CollectionObjDestroyed = 0;
    i64ItemObjCreated = 0;
    i64ItemObjDestroyed = 0;
    i64ReferenceCounterObjCreated = 0;
    i64ReferenceCounterObjDestroyed = 0;
    i64SizeGroupObjCreated = 0;
    i64SizeGroupObjDestroyed = 0;
    i64SortedFileCollectionObjCreated = 0;
    i64SortedFileCollectionObjDestroyed = 0;
    i64DuplicateEntryObjCreated = 0;
    i64DuplicateEntryObjDestroyed = 0;
    i64DuplicateFileCollectionObjCreated = 0;
    i64DuplicateFileCollectionObjDestroyed = 0;
    i64DuplicateFileHardLinkerObjCreated = 0;
    i64DuplicateFileHardLinkerObjDestroyed = 0;
    i64FileTooSmall = 0;
    i64TotalSmallFileSize = 0;
}


ReferenceCounter::ReferenceCounter() {
    Statistics::getInstance()->i64ReferenceCounterObjCreated++;
    numberOfReferences = 1;
}

ReferenceCounter::~ReferenceCounter() {
    Statistics::getInstance()->i64ReferenceCounterObjDestroyed++;
}

void ReferenceCounter::addReference() {
    numberOfReferences++;
}

void ReferenceCounter::removeReference() {
    numberOfReferences--;
    // Delete the object if no further reference exists
    if (numberOfReferences <= 0) {
        switch (classType) {
        case CLASS_TYPE_FILE:
            delete (File*) this;
            break;
        case CLASS_TYPE_PATH:
            delete (Path*) this;
            break;
        default:
            delete this;
            break;
        }
    }
}

/**
 * Item of the collection. Each entry is of type Item
 */
class Item {
public:
    void* data;

    Item* next;

    Item(void* defaultData) {
        Statistics::getInstance()->i64ItemObjCreated++;
        data = defaultData;
        next = NULL;
    }

    ~Item() {
        Statistics::getInstance()->i64ItemObjDestroyed++;
    }
};

void Collection::append(void* data) {
    Item* newItem = new Item(data);
    if (root != NULL) {
        last->next = newItem;
        last = newItem;
    } else {
        root = last = newItem;
    }
    itemCount++;
    nextItem = root;
}

void Collection::push(void* data) {
    append(data);
}

void Collection::addAll(Collection* items) {
    if (items == NULL) {
        return;
    }
    void* item;
    while (item = items->pop()) {
        append(item);
    }
}

void* Collection::pop() {
    if (root != NULL) {
        Item* temp = root;
        nextItem = root = root->next;
        if (last == temp) {
            last = root;
        }
        itemCount--;
        void* result = temp->data;
        delete temp;
        return result;
    } else {
        return NULL;
    }
}

void* Collection::next() {
    if (nextItem != NULL) {
        Item* temp = nextItem;
        nextItem = nextItem->next;
        return temp->data;
    } else {
        return NULL;
    }
}

void* Collection::item(int index) {
    if (itemCount -1 >= index) {
        Item* temp = root;
        for (int i=0; i<index; i++) {
            temp = temp->next;
        }
        nextItem = temp->next;
        return temp->data;
    } else {
        return NULL;
    }
}

int Collection::getSize() {
    return itemCount;
}

Collection::Collection() {
    Statistics::getInstance()->i64CollectionObjCreated++;
    itemCount = 0;
    root = last= nextItem = NULL;
}

Collection::~Collection() {
    Statistics::getInstance()->i64CollectionObjDestroyed++;
    while (itemCount > 0)
        pop();
}
