/* Util.cpp : Utility function for DFHL

DFHL - Duplicate File Hard Linker, a small tool to gather some space
    from duplicate files on your hard disk
Copyright (C) 2004, 2005 Jens Scheffler & Oliver Schneider

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


#include <stdafx.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <stdarg.h>
#include "Util.h"
#include "FileSystem.h"

// Global Variables
// *******************************************
/** Logging Level for the console output */
int logLevel = LOG_INFO;

void logError(LPCWSTR message, ...) {
	va_list argp;
	fwprintf(stderr, L"ERROR: ");
	va_start(argp, message);
	vfwprintf(stderr, message, argp);
	va_end(argp);
	fwprintf(stderr, L"\n");
}

void logError(DWORD errNumber, LPCWSTR message, ...) {
	va_list argp;
	fwprintf(stderr, L"ERROR: ");
	va_start(argp, message);
	vfwprintf(stderr, message, argp);
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
	fwprintf(stderr, L" -> [%i] %s\n", errNumber, msgBuffer);
	LocalFree(msgBuffer);
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
	INT64 size = FileData.nFileSizeLow + ((INT64)MAXDWORD + 1) * FileData.nFileSizeHigh;
	logDebug(L"Found file \"%s\" (Size=%I64i,%s%s%s%s%s%s%s%s%s%s%s%s)", 
		FileData.cFileName,
		size,
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
	fileCompares = 0;
	hashCompares = 0;
	hashComparesHit1 = 0;
	hashComparesHit2 = 0;
	hashComparesHit3 = 0;
	fileMetaDataMismatch = 0;
	fileAlreadyLinked = 0;
	fileContentDifferFirstBlock = 0;
	fileContentDifferLater = 0;
	fileContentSame = 0;
	fileCompareProblems = 0;
	hashesCalculated = 0;
	directoriesFound = 0;
	filesFound = 0;
	bytesRead = 0;
	filesOpened = 0;
	filesClosed = 0;
	fileOpenProblems = 0;
	pathObjCreated = 0;
	pathObjDestroyed = 0;
	fileObjCreated = 0;
	fileObjDestroyed = 0;
	unbufferedFileStreamObjCreated = 0;
	unbufferedFileStreamObjDestroyed = 0;
	fileSystemObjCreated = 0;
	fileSystemObjDestroyed = 0;
	hardLinks = 0;
	hardLinksSuccess = 0;
	collectionObjCreated = 0;
	collectionObjDestroyed = 0;
	itemObjCreated = 0;
	itemObjDestroyed = 0;
	referenceCounterObjCreated = 0;
	referenceCounterObjDestroyed = 0;
	sizeGroupObjCreated = 0;
	sizeGroupObjDestroyed = 0;
	sortedFileCollectionObjCreated = 0;
	sortedFileCollectionObjDestroyed = 0;
	duplicateEntryObjCreated = 0;
	duplicateEntryObjDestroyed = 0;
	duplicateFileCollectionObjCreated = 0;
	duplicateFileCollectionObjDestroyed = 0;
	duplicateFileHardLinkerObjCreated = 0;
	duplicateFileHardLinkerObjDestroyed = 0;
	fileTooSmall = 0;
}


ReferenceCounter::ReferenceCounter() {
	Statistics::getInstance()->referenceCounterObjCreated++;
	numberOfReferences = 1;
}

ReferenceCounter::~ReferenceCounter() {
	Statistics::getInstance()->referenceCounterObjDestroyed++;
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
 * Item of the collection. Each entry it of type Item
 */
class Item {
public:
	void* data;

	Item* next;

	Item(void* defaultData) {
		Statistics::getInstance()->itemObjCreated++;
		data = defaultData;
		next = NULL;
	}

	~Item() {
		Statistics::getInstance()->itemObjDestroyed++;
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
	Statistics::getInstance()->collectionObjCreated++;
	itemCount = 0;
	root = last= nextItem = NULL;
}

Collection::~Collection() {
	Statistics::getInstance()->collectionObjDestroyed++;
	while (itemCount > 0)
		pop();
}
