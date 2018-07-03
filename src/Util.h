/* Util.h : Headers for Utility functions for DFHL

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


// Global Definitions
// *******************************************
#define LOG_ERROR               1
#define LOG_INFO                0
#define LOG_VERBOSE             -1
#define LOG_DEBUG               -2

#define CLASS_TYPE_FILE         1
#define CLASS_TYPE_PATH         2

// Global Variables
// *******************************************


/** Flag to write error messages to standard output (2012-10-15  HaSchm) */
extern bool boNoStdErr;

/** Logging Level for the console output */
extern int logLevel;

/**
 * Some global Variables for Statistics stuff...
 */
class Statistics {
public:
    static Statistics* getInstance();

    Statistics();

    /** Variable for statistic collection */
    INT64 i64FileCompares;
    INT64 i64HashCompares;
    INT64 i64HashComparesHit1;
    INT64 i64HashComparesHit2;
    INT64 i64HashComparesHit3;
    /* INT64 i64FileMetaDataMismatch;  2012-10-12  HaSchm */
    INT64 i64FileNameMismatch;
    INT64 i64FileAttributeMismatch;
    INT64 i64FileMTimeMismatch;

    INT64 i64FileAlreadyLinked;
    INT64 i64FileContentDifferFirstBlock;
    INT64 i64FileContentDifferLater;
    INT64 i64FileContentSame;
    int fileCompareProblems;
    INT64 i64HashesCalculated;
    int directoryOpenProblems; /* 2012-10-16  HaSchm */
    int directoriesFound;  /* incl. junctions */
    int junctionsFound;  /* 2012-10-16  HaSchm */
    INT64 i64FilesFound;  /* incl. hidden or system files and incl small files */
    INT64 i64HSFilesFound;  /* 2012-10-16  HaSchm */
    INT64 i64BytesRead;
    INT64 i64FilesOpened;
    INT64 i64FilesClosed;
    int fileOpenProblems;
    int pathObjCreated;
    int pathObjDestroyed;
    INT64 i64FileObjCreated;
    INT64 i64FileObjDestroyed;
    INT64 i64UnbufferedFileStreamObjCreated;
    INT64 i64UnbufferedFileStreamObjDestroyed;
    INT64 i64FileSystemObjCreated;
    INT64 i64FileSystemObjDestroyed;
    INT64 i64HardLinks;  /* errors+success */
    INT64 i64HardLinksSuccess;
    INT64 i64BytesSaved;  /* 2012-10-12  HaSchm */
    INT64 i64CollectionObjCreated;
    INT64 i64CollectionObjDestroyed;
    INT64 i64ItemObjCreated;
    INT64 i64ItemObjDestroyed;
    INT64 i64ReferenceCounterObjCreated;
    INT64 i64ReferenceCounterObjDestroyed;
    INT64 i64SizeGroupObjCreated;
    INT64 i64SizeGroupObjDestroyed;
    INT64 i64SortedFileCollectionObjCreated;
    INT64 i64SortedFileCollectionObjDestroyed;
    INT64 i64DuplicateEntryObjCreated;
    INT64 i64DuplicateEntryObjDestroyed;
    INT64 i64DuplicateFileCollectionObjCreated;
    INT64 i64DuplicateFileCollectionObjDestroyed;
    INT64 i64DuplicateFileHardLinkerObjCreated;
    INT64 i64DuplicateFileHardLinkerObjDestroyed;
    INT64 i64FileTooSmall;
    INT64 i64TotalSmallFileSize;
};

// Global Functions
// *******************************************

/**
 * Method to log a message to stdout/stderr
 */
void logError(LPCWSTR message, ...);

void logError(DWORD errNumber, LPCWSTR message, ...);

void logErrorInfo(LPCWSTR message, ...);  /* 2012-10-15  HaSchm */

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
    /* append/push data to end of collection (nextItem = root) */
    void append(void* data);

    /* append/push data to end of collection (nextItem = root) */
    void push(void* data);

    /* add/move all elements from items to end of collection (pop items; push this) */
    void addAll(Collection* items);

    /* return and remove first item from collection (nextItem = root) */
    void* pop();

    /* return next item from collection (nextItem = nextItem->next) */
    void* next();

    /* return item at given index from collection (nextItem = item[index+1]) */
    void* item(int index);

    /* return number of items in collection */
    int getSize();

    Collection();

    ~Collection();
};
