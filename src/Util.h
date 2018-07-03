/* Util.h : Headers for Utility functions for DFHL

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


// Global Definitions
// *******************************************
#define LOG_ERROR				1
#define LOG_INFO				0
#define LOG_VERBOSE				-1
#define LOG_DEBUG				-2

#define CLASS_TYPE_FILE         1
#define CLASS_TYPE_PATH         2

// Global Variables
// *******************************************

/**
 * Some global Variables for Statistics stuff...
 */
class Statistics {
public:
	static Statistics* getInstance();

	Statistics();

	/** Variable for statistic collection */
	int fileCompares;
	int hashCompares;
	int hashComparesHit1;
	int hashComparesHit2;
	int hashComparesHit3;
	int fileMetaDataMismatch;
	int fileAlreadyLinked;
	int fileContentDifferFirstBlock;
	int fileContentDifferLater;
	int fileContentSame;
	int fileCompareProblems;
	int hashesCalculated;
	int directoriesFound;
	int filesFound;
	INT64 bytesRead;
	int filesOpened;
	int filesClosed;
	int fileOpenProblems;
	int pathObjCreated;
	int pathObjDestroyed;
	int fileObjCreated;
	int fileObjDestroyed;
	int unbufferedFileStreamObjCreated;
	int unbufferedFileStreamObjDestroyed;
	int fileSystemObjCreated;
	int fileSystemObjDestroyed;
	int hardLinks;
	int hardLinksSuccess;
	int collectionObjCreated;
	int collectionObjDestroyed;
	int itemObjCreated;
	int itemObjDestroyed;
	int referenceCounterObjCreated;
	int referenceCounterObjDestroyed;
	int sizeGroupObjCreated;
	int sizeGroupObjDestroyed;
	int sortedFileCollectionObjCreated;
	int sortedFileCollectionObjDestroyed;
	int duplicateEntryObjCreated;
	int duplicateEntryObjDestroyed;
	int duplicateFileCollectionObjCreated;
	int duplicateFileCollectionObjDestroyed;
	int duplicateFileHardLinkerObjCreated;
	int duplicateFileHardLinkerObjDestroyed;
	int fileTooSmall;
};

// Global Functions
// *******************************************

/**
 * Method to log a message to stdout/stderr
 */
void logError(LPCWSTR message, ...);

void logError(DWORD errNumber, LPCWSTR message, ...);

void logInfo(LPCWSTR message, ...);

void logVerbose(LPCWSTR message, ...);

void logDebug(LPCWSTR message, ...);

void setLogLevel(int newLevel);

/**
 * Logs a found file to debug
 */
void logFile(WIN32_FIND_DATA FileData);

/**
 * Cleans the provided String
 * @return String reference to use it inside function calls
 */
LPWSTR cleanString(LPWSTR string);


/**
 * Abstract base class to implement "garbage collection" features by counting the number of references
 */
class ReferenceCounter {
private:
	int numberOfReferences;
protected:
	int classType; // signature of the class type for object following and correct destruction
public:
	ReferenceCounter();

	~ReferenceCounter();

	/**
	 * Signals the object that a further reference is created
	 */
	void addReference();

	/**
	 * Signals the object that a reference is removed - the object needs to check whether it can be destroyed
	 */
	void removeReference();
};

/** Item implementation for the collection */
class Item;

/**
 * Simple implementation for a collection
 */
class Collection {
private:
	int itemCount;
	Item* root;
	Item* last;
	Item* nextItem;
public:
	void append(void* data);

	void push(void* data);

	void addAll(Collection* items);

	void* pop();

	void* next();

	void* item(int index);

	int getSize();

	Collection();

	~Collection();
};
