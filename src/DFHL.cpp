/* DFHL.cpp : Defines the entry point for the console application.

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
#include "Util.h"
#include "FileSystem.h"
#include "crc32.h"


// Global Definitions
// *******************************************
#define FIRST_BLOCK_SIZE        (16*1024) // Smaller block size (n*MAX_SECTOR_SIZE)
#define BLOCK_SIZE              (4*1024*1024) // 4MiB seems to be a good value for performance without too much memory load
#define MIN_FILE_SIZE           1024 // Minimum file size so that hard linking will be checked...

#define PROGRAM_NAME        L"Duplicate File Hard Linker"
#define PROGRAM_VERSION     L"Version 2.5"
#define PROGRAM_AUTHOR      L"Jens Scheffler, Oliver Schneider, Hans Schmidts"

enum CompareResult {
    EQUAL,          // File compare was successful and content is matching
    SAME,           // Files are already hard linked
    SKIP,           // File 1 should not be processed (e.g. open error)
    DIFFERENT       // Files differ
};


// Global Variables
// *******************************************
/** Flag if list should be displayed */
bool outputList = false;
/** Flag if running in real or test mode */
bool reallyLink = false;
/** Flag if statistics should be displayed */
bool showStatistics = false;
/** Program exit code (2012-10-12  HaSchm) */
int iExitCode = 0;



// Global Classes
// *******************************************


/**
 * Class implementation for a group of files having the same size
 */
class SizeGroup {
private:
    INT64 i64FileSize;
    Collection* items;
    SizeGroup* next;
public:
    /**
     * Creates a new SizeGroup with a single file as start
     */
    SizeGroup(File* newFile, SizeGroup* newNext) {
        Statistics::getInstance()->i64SizeGroupObjCreated++;
        i64FileSize = newFile->i64GetSize();
        items = new Collection();
        items->append(newFile);
        next = newNext;
    }

    ~SizeGroup() {
        Statistics::getInstance()->i64SizeGroupObjDestroyed++;
        delete items;
    }

    /**
     * Adds the provided file to the file group
     */
    void addFile(File* newFile) {
        items->append(newFile);
    }

    /**
     * @return next group in the linked chain of files, NULL if no further item is in the list
     */
    SizeGroup* getNext() {
        return next;
    }

    /**
     * Sets the next size group to a new item
     */
    void setNext(SizeGroup* newNext) {
        next = newNext;
    }

    /**
     * Gets the size of all files in the group
     */
    INT64 i64GetFileSize() {
        return i64FileSize;
    }

    /**
     * Gets the number of files in the group
     */
    int getFileCount() {
        return items->getSize();
    }

    /**
     * Pops the next file from the list of files to be processed
     */
    File* popFile() {
        return (File*) items->pop();
    }

    /**
     * Peeks the next file to be processed
     */
    File* getNextFile() {
        return (File*) items->next();
    }
};


/**
 * Class implementation for collecting files based on the file size
 */
class SortedFileCollection {
private:
    int files;
    int groups;
    INT64 i64TotalSize;
    SizeGroup* first;
public:
    SortedFileCollection() {
        Statistics::getInstance()->i64SortedFileCollectionObjCreated++;
        files = 0;
        groups = 0;
        i64TotalSize = 0;
        first = NULL;
    }

    ~SortedFileCollection() {
        Statistics::getInstance()->i64SortedFileCollectionObjDestroyed++;
        if (first != NULL) {
            // delete all descendant items in the linked list of size groups
            SizeGroup* next = first->getNext();
            while (next != NULL) {
                SizeGroup* nextToBeDeleted = next;
                next = next->getNext();
                delete nextToBeDeleted;
            }
            delete first;
        }
    }

    /**
     * Adds a file to the sorted file collection
     */
    void addFile(File* newFile) {
        // try to find the best group where to add the file to
        SizeGroup* g = first;
        SizeGroup* lastG = NULL;
        while (g != NULL && g->i64GetFileSize() < newFile->i64GetSize()) {
            lastG = g;
            g = g->getNext();
        }
        // if no group was found, create a new file size group
        if (g == NULL) {
            // create the new group
            g = new SizeGroup(newFile, NULL);
            groups++;
            // if a previous group was in the chain, set the link to extend the chain
            if (lastG != NULL) {
                lastG->setNext(g);
            }
            // check if this is maybe the first group?
            if (first == NULL) {
                first = g;
            }
        } else {
            // check if the correct group was found
            if (g->i64GetFileSize() == newFile->i64GetSize()) {
                // add the file to the existing group...
                g->addFile(newFile);
            } else {
                // insert a new group at the correct position
                SizeGroup* newGroup = new SizeGroup(newFile, g);
                groups++;
                // modify the chain to insert the new group
                if (lastG != NULL) {
                    lastG->setNext(newGroup);
                }
                // check if the item was inserted as first...
                if (first == g) {
                    first = newGroup;
                }
            }
        }
        files++;
        i64TotalSize += newFile->i64GetSize();
    }

    /**
     * Retrieves the next group from the collection for processing.
     * Note that the caller needs to destroy the retrieved object!
     */
    SizeGroup* popGroup() {
        SizeGroup* result = first;
        if (first != NULL) {
            first = first->getNext();
        }
        groups--;
        return result;
    }

    int getFileCount() {
        return files;
    }

    INT64 i64GetTotalFileSize() {
        return i64TotalSize;
    }
};


/**
 * class for representing a file duplicate, container for two file references
 */
class DuplicateEntry {
private:
    File* file1;
    File* file2;
public:
    DuplicateEntry(File* newFile1, File* newFile2) {
        Statistics::getInstance()->i64DuplicateEntryObjCreated++;
        file1 = newFile1;
        file1->addReference();
        file2 = newFile2;
        file2->addReference();
    }

    ~DuplicateEntry() {
        Statistics::getInstance()->i64DuplicateEntryObjDestroyed++;
        file1->removeReference();
        file2->removeReference();
    }

    /**
     * Gets reference of file 1
     */
    File* getFile1() {
        return file1;
    }

    /**
     * Gets reference of file 2
     */
    File* getFile2() {
        return file2;
    }
};



/**
 * class for encasulating the collection of duplicates
 */
class DuplicateFileCollection {
private:
    INT64 i64DuplicateSize;
    Collection* items;
public:
    DuplicateFileCollection() {
        Statistics::getInstance()->i64DuplicateFileCollectionObjCreated++;
        items = new Collection();
        i64DuplicateSize = 0;
    }

    ~DuplicateFileCollection() {
        Statistics::getInstance()->i64DuplicateFileCollectionObjDestroyed++;
        // remove orphan references of duplicates if NOT linking...
        deleteAll();
        delete items;
    }

    void deleteAll() {
        i64DuplicateSize = 0;
        DuplicateEntry* leftover;
        while ((leftover = popDuplicate()) != NULL) {
            delete leftover;
        }
    }

    /**
     * Retrieves the next item to process
     */
    DuplicateEntry* popDuplicate() {
        return (DuplicateEntry*) items->pop();
    }

    /**
     * Adds a new duplicate
     */
    void addDuplicate(DuplicateEntry* newDuplicate) {
        i64DuplicateSize += newDuplicate->getFile1()->i64GetSize();
        items->append(newDuplicate);
    }

    DuplicateEntry* next() {
        return (DuplicateEntry*) items->next();
    }

    int getDuplicateCount() {
        return items->getSize();
    }
    INT64 i64GetTotalDuplicateSize() {
        return i64DuplicateSize;
    }
};


/**
 * Duplicate File Linker Class
 */
class DuplicateFileHardLinker {

private:
    /** File System Instance which handles all IO logic */
    FileSystem* fs;
    /** Collection of paths to process */
    Collection* paths;
    /** Collection of paths of next level for /i */
    Collection* pathsNextLevel;
    int iLevel;  /* 2012-10-12  HaSchm */
    /** Sorted File Collection of files to check and process */
    SortedFileCollection* files;
    /** List of duplicates to process */
    DuplicateFileCollection* duplicates;
    /** buffer variables for file compare */
    LPBYTE block;
    LPBYTE block1f;
    LPBYTE block2f;
    LPBYTE block1;
    LPBYTE block2;
    /** temporary file name buffers */
    LPWSTR file1Name;
    LPWSTR file2Name;
    /** Temporary file information storages */
    BY_HANDLE_FILE_INFORMATION info1;
    BY_HANDLE_FILE_INFORMATION info2;
    /** Flags that block1* already read */
    bool boBlock1fRead,boBlock1Read;
    /** Flag if name of file need to match */
    bool boNameMustMatch;  /* 2012-10-22  HaSchm */
    /** Flag if attributes of file need to match */
    bool attributeMustMatch;
    /** Flag if small files (<1024 bytes) should be processed */
    bool smallFiles;
    /** Flag if recursive processing should be enabled */
    bool recursive;
    /** Flag if timestamps of file need to match */
    bool dateTimeMustMatch;
public:
    /** Flag taht all levels should be processed individually */
    bool boLevelIndividually;


private:
    /**
     * Calculates a hash from byte data
     * @param block Array of bytes to analyze
     * @param length number of bytes to use for calculation
     * @return calculated hash value
     */
    DWORD u32GenerateHash(LPBYTE block, int length) {
      Statistics::getInstance()->i64HashesCalculated++;
      /* 2012-02-23  HaSchm: With crc32 more hashes of different files differ,
         even with only 16KiB crc instead of 64KiB xor. */
      return crc32(0, block, length);
    }

    /**
     * Compares the given 2 files content
     */
    CompareResult compareFiles(File* file1, File* file2, bool boNewFile1) {
        if (boNewFile1) {
            boBlock1fRead = boBlock1Read = false;
            file1->copyName(cleanString(file1Name));
        }
        file2->copyName(cleanString(file2Name));
        logVerbose(L"Comparing files \"%s\" and \"%s\".", file1Name, file2Name);
        Statistics::getInstance()->i64FileCompares++;

        // optimization: check the file hash codes (if existing already)
        if (file1->boIsHashAvailable() && file2->boIsHashAvailable() && file1->u32GetHash() != file2->u32GetHash()) {
            // okay, hashes are different, files seem to differ!
            logVerbose(L"Files differ in content (hash).");
            Statistics::getInstance()->i64HashCompares++;
            Statistics::getInstance()->i64HashComparesHit1++;
            return DIFFERENT;
        }

        // check for attributes matching
        if (boNameMustMatch && (wcscmp(file1->getName(),file2->getName())!=0)) {
            logVerbose(L"Names of files do not match (\"%s\"!=\"%s\"), skipping.",file1->getName(),file2->getName());
            Statistics::getInstance()->i64FileNameMismatch++;
            return DIFFERENT;
        }

        // check for attributes matching
        if (attributeMustMatch) {
            /* 2012-10-12  HaSchm */
            DWORD dwAttr1, dwAttr2;
            dwAttr1 = file1->getAttributes() & (DWORD)(~(DWORD)FILE_ATTRIBUTE_ARCHIVE);
            dwAttr2 = file2->getAttributes() & (DWORD)(~(DWORD)FILE_ATTRIBUTE_ARCHIVE);
            if (dwAttr1!=dwAttr2) {
                logVerbose(L"Attributes of files do not match (0x%lX!=0x%lX), skipping.",dwAttr1,dwAttr2);
                Statistics::getInstance()->i64FileAttributeMismatch++;
                return DIFFERENT;
            }
        }

        // check for time stamp matching
        if (dateTimeMustMatch && (
                file1->getLastModifyTime().dwHighDateTime != file2->getLastModifyTime().dwHighDateTime ||
                file1->getLastModifyTime().dwLowDateTime  != file2->getLastModifyTime().dwLowDateTime
                )) {
            logVerbose(L"Modification timestamps of files do not match, skipping.");
            Statistics::getInstance()->i64FileMTimeMismatch++;
            return DIFFERENT;
        }

        // doublecheck data consistency!
        if (file1->equals(file2)) {
            logError(L"Same file \"%s\"found as duplicate, ignoring!", file1Name);
            Statistics::getInstance()->fileCompareProblems++;
            return SKIP;
        }

        // Open File 2
        UnbufferedFileStream* fs2 = new UnbufferedFileStream(file2);
        if (!fs2->open()) {
            logError(L"Unable to open file \"%s\"", file2Name);
            delete fs2;
            Statistics::getInstance()->fileCompareProblems++;
            return DIFFERENT; // assume that only file2 is not to be opened, signal different file
        }

        // Open File 1
        UnbufferedFileStream* fs1 = new UnbufferedFileStream(file1);
        if (!fs1->open()) {
            logError(L"Unable to open file \"%s\"", file1Name);
            fs2->close();
            delete fs1;
            delete fs2;
            Statistics::getInstance()->fileCompareProblems++;
            return SKIP; // file 1 can not be opened, skip this file for further compares!
        }

        // Check if both files are already linked
        // 2012-02-06  HaSchm: Before reading any data or calculation hash, because
        // dfhl may be used to hard link additional files where existing files are
        // already linked (e.g. differnet backups of own data).
        if (fs1->getFileDetails(&info1) && fs2->getFileDetails(&info2)) {
            if (info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber &&
                info1.nFileIndexHigh == info2.nFileIndexHigh &&
                info1.nFileIndexLow == info2.nFileIndexLow) {

                logVerbose(L"Files are already hard linked, skipping.");
                fs1->close();
                fs2->close();
                delete fs1;
                delete fs2;
                Statistics::getInstance()->i64FileAlreadyLinked++;
                return SAME;
            }
        }


        // Optimization #345:
        // Maybe we already have a hash of file #1, so we start processing file #2 first...

        // Read File Content and compare
        INT64 i64BytesToRead = file2->i64GetSize();
        int read2;
        read2 = fs2->read(block2f, FIRST_BLOCK_SIZE);

        // calculate the hash of the block of file 2 if not done so far...
        if (!file2->boIsHashAvailable()) {
            file2->vSetHash(u32GenerateHash(block2f, read2));  // 2012-02-03  HaSchm  FIRST_BLOCK_SIZE

            // If we have a hash of file #1 also, we can compare now...
            if (file1->boIsHashAvailable() && file1->u32GetHash() != file2->u32GetHash()) {
                // okay, hashes are different, files seem to differ!
                logVerbose(L"Files differ in content (hash).");
                fs1->close();
                fs2->close();
                delete fs1;
                delete fs2;
                Statistics::getInstance()->i64HashCompares++;
                Statistics::getInstance()->i64HashComparesHit2++;
                return DIFFERENT;
            }
        }

        // Read File Content of file #1 now and compare
        int read1;
        if (boBlock1fRead) {
            read1 = fs1->skip(FIRST_BLOCK_SIZE);
        }
        else {
            read1 = fs1->read(block1f, FIRST_BLOCK_SIZE);
            boBlock1fRead = true;
        }

        // generate hash for file #1 if not done so far...
        if (!file1->boIsHashAvailable()) {
            file1->vSetHash(u32GenerateHash(block1f, read1));  // 2012-02-03  HaSchm  FIRST_BLOCK_SIZE

            // Finally we can be sure that we have hashes of both files, compare them first (faster?)
            if (file1->u32GetHash() != file2->u32GetHash()) {
                // okay, hashes are different, files seem to differ!
                logVerbose(L"Files differ in content (hash).");
                fs1->close();
                fs2->close();
                delete fs1;
                delete fs2;
                Statistics::getInstance()->i64HashCompares++;
                Statistics::getInstance()->i64HashComparesHit3++;
                return DIFFERENT;
            }
        }

        // compare all bytes now - as the hashes are equal
        for (int i = 0; i < read1; i++) {
            if (block1f[i] != block2f[i]) {
                logVerbose(L"Files differ in content.");
                fs1->close();
                fs2->close();
                delete fs1;
                delete fs2;
                Statistics::getInstance()->i64FileContentDifferFirstBlock++;
                return DIFFERENT;
            }
        }

        // okay, now optimized fast start was done, run in a loop to compare the rest of the file content!
        i64BytesToRead -= read1;
        bool switcher = true; // helper variable for performance optimization
        bool boFistBlock1Read = true; // helper variable for performance optimization
        while (i64BytesToRead > 0) {

            // Read Blocks - Performance boosted: read alternating to mimimize head shifts... ;-)
            if (switcher) {
                if (boFistBlock1Read && boBlock1Read) {
                    read1 = fs1->skip(BLOCK_SIZE);
                }
                else {
                    read1 = fs1->read(block1, BLOCK_SIZE);
                    boBlock1Read = boFistBlock1Read;
                }
                read2 = fs2->read(block2, BLOCK_SIZE);
            } else {
                read2 = fs2->read(block2, BLOCK_SIZE);
                read1 = fs1->read(block1, BLOCK_SIZE);
                boBlock1Read = false;
            }
            boFistBlock1Read = false;

            // change the state for the next read operation
            switcher = !switcher; // alternate read

            // Compare Data
            i64BytesToRead -= read1;
            if (read1 != read2 || read1 == 0) {
                if (logLevel>LOG_VERBOSE || !boNoStdErr) {
                    logErrorInfo(L"Comparing files \"%s\" and \"%s\".", file1Name, file2Name);  /* 2012-10-15  HaSchm */
                }
                logError(L"File length differ or read error! This _should_ not happen!?!?");
                fs1->close();
                fs2->close();
                delete fs1;
                delete fs2;
                Statistics::getInstance()->fileCompareProblems++;
                return DIFFERENT;
            }

            for (int i = 0; i < read1; i++) {
                if (block1[i] != block2[i]) {
                    logVerbose(L"Files differ in content.");
                    fs1->close();
                    fs2->close();
                    delete fs1;
                    delete fs2;
                    Statistics::getInstance()->i64FileContentDifferLater++;
                    return DIFFERENT;
                }
            }
        }

        // Close Files
        fs1->close();
        fs2->close();
        delete fs1;
        delete fs2;

        logVerbose(L"Files are equal, hard link possible.");
        Statistics::getInstance()->i64FileContentSame++;
        return EQUAL;
    }

    /**
     * Adds a file to the collection of files to process
     * @param newFile FindFile Structure of further file information
     */
    void addFile(File* newFile) {
        newFile->addReference();
        files->addFile(newFile);
    }

    /**
     * Add the given 2 files to duplicates
     */
    void addDuplicate(File* file1, File* file2) {
        duplicates->addDuplicate(new DuplicateEntry(file1, file2));
    }

public:

    /**
     * Constructor for the Object
     */
    DuplicateFileHardLinker() {
        Statistics::getInstance()->i64DuplicateFileHardLinkerObjCreated++;
        fs = new FileSystem();
        paths = new Collection();
        pathsNextLevel = new Collection();
        iLevel = 0;
        files = new SortedFileCollection();
        duplicates = new DuplicateFileCollection();
        file1Name = new WCHAR[MAX_PATH_LENGTH];
        file2Name = new WCHAR[MAX_PATH_LENGTH];
        boNameMustMatch = false;
        attributeMustMatch = false;
        smallFiles = false;
        recursive = false;
        dateTimeMustMatch = false;
        boLevelIndividually = false;
        block = new BYTE[2*(FIRST_BLOCK_SIZE+BLOCK_SIZE)+MAX_SECTOR_SIZE];
        // calculate sector aligned buffer addresses (for ReadFile with FILE_FLAG_NO_BUFFERING)
        block1f = &block[MAX_SECTOR_SIZE-(((unsigned)block)&(MAX_SECTOR_SIZE-1))];
        block2f = &block1f[FIRST_BLOCK_SIZE];
        block1 = &block2f[FIRST_BLOCK_SIZE];
        block2 = &block1[BLOCK_SIZE];
        boBlock1fRead = boBlock1Read = false;
    }

    ~DuplicateFileHardLinker() {
        Statistics::getInstance()->i64DuplicateFileHardLinkerObjDestroyed++;
        delete fs;
        delete paths;
        delete pathsNextLevel;
        delete files;
        delete duplicates;
        if (block != NULL) {
            delete block;
        }
        delete file1Name;
        delete file2Name;
    }

    /**
     * Setter for the name match flag
     */
    void vSetNameMustMatch(bool bo) {
        boNameMustMatch = bo;
    }

    /**
     * Setter for the attriute match flag
     */
    void setAttributeMustMatch(bool newValue) {
        attributeMustMatch = newValue;
    }

    /**
     * Setter for the hidden files flag
     */
    void setHiddenFiles(bool newValue) {
        fs->setHiddenFiles(newValue);
    }

    /**
     * Setter for the junctions follow flag
     */
    void setFollowJunctions(bool newValue) {
        fs->setFollowJunctions(newValue);
    }

    /**
     * Setter for the small files flag
     */
    void setSmallFiles(bool newValue) {
        smallFiles = newValue;
    }

    /**
     * Setter for the small files flag
     */
    void setRecursive(bool newValue) {
        recursive = newValue;
    }

    /**
     * Setter for the system files flag
     */
    void setSystemFiles(bool newValue) {
        fs->setSystemFiles(newValue);
    }

    /**
     * Setter for the attriute match flag
     */
    void setDateMatch(bool newValue) {
        dateTimeMustMatch = newValue;
    }

    /**
     * Adds a path to the collection of path's to process
     * @param path Path name to add to the collection (e.g. "D:\dir")
     */
    void addPath(LPWSTR path) {
        Path* parsedPath = fs->parsePath(path);
        if (parsedPath != NULL) {
            paths->push(parsedPath);
        } else {
            logError(L"Folder \"%s\" is not existing!", path);
        }
    }

    /**
     * Starts the search for duplicate files
     */
    void findDuplicates() {
        // Step 1: Walk through the located directories until all directories are processed and found
        delete files;
        files = new SortedFileCollection();

        ++iLevel;
        fflush(stderr);
        if (boLevelIndividually) logInfo(L"Parsing Directory Tree Level %i ...",iLevel);
        else logInfo(L"Parsing Directory Tree...");
        fflush(stdout);
        Path* aPath;
        while ((aPath = (Path*) paths->pop()) != NULL) {
            aPath->copyName(cleanString(file1Name));
            logVerbose(L"Parsing Folder \"%s\"", file1Name);

            Collection* folderContent = fs->getFolderContent(aPath);
            FolderEntry *aItem;
            while ((aItem = (FolderEntry*) folderContent->pop()) != NULL) {

                // check what kind of entry we found...
                if (aItem->isFile()) {

                    // we have a file entry
                    File* aFile = (File*) aItem;

                    // Check if filesize is big enough and contains data
                    if (aFile->i64GetSize() == 0 || (!smallFiles && aFile->i64GetSize() < MIN_FILE_SIZE)) {
                        logDebug(L"ignoring file \"%s\", is too small.", aFile->getName());
                        Statistics::getInstance()->i64FileTooSmall++;
                        Statistics::getInstance()->i64TotalSmallFileSize+=aFile->i64GetSize();
                        aFile->removeReference();
                    } else {
                        files->addFile(aFile);
                    }
                } else {

                    // okay, we found a directory...
                    if (recursive) {
                      if (boLevelIndividually) {
                          pathsNextLevel->push(aItem);
                      } else {
                          paths->push(aItem);
                      }
                    } else {
                        // if the path is not used, mark it as unused...
                        aItem->removeReference();
                    }
                }
            }
            delete folderContent;

            // mark the object as "unused"
            aPath->removeReference();
        }

        // Step 2: Walk over all relevant files
        fflush(stderr);
        logInfo(L"%i (%0.4g) files, %I64i (%0.4g) bytes, comparing relevant files.", files->getFileCount(), 1.0*files->getFileCount(), files->i64GetTotalFileSize(), 1.0*files->i64GetTotalFileSize());
        fflush(stdout);
        /* Note: Count and size without ignored files */
        SizeGroup* sg;
        File* file1;
        File* file2;
        bool duplicateChecked;
        bool boNewFile1;
        while ((sg = files->popGroup()) != NULL) { // process all size groups
            logVerbose(L"Processing file group with %I64i bytes, comparing %i files...", sg->i64GetFileSize(), sg->getFileCount());
            while ((file1 = sg->popFile()) != NULL) { // process all files in the group
                duplicateChecked = file1->boIsDup; // flag if the file 1 was compared "enough" and next file should be taken
                boNewFile1 = true;
                for (int i = 0; i < sg->getFileCount() && !duplicateChecked; i++) {
                    file2 = sg->getNextFile();
                    DWORD start = GetTickCount();
                    DWORD time = 0;
                    switch (compareFiles(file1, file2, boNewFile1))
                    {
                    case EQUAL:
                        time = GetTickCount() - start;
                        logDebug(L"file compare took %ims, %I64i KB/s", time, (time>0)?(file1->i64GetSize() * 2 * 1000 / time / 1024):0);

                        // Files are equal, marking them for later processing...
                        addDuplicate(file1, file2);
                        file2->boIsDup = true;  // skip this file as file1 (more duplicates of file2 will be found via file1 with bufferes already read)
                        // duplicateChecked = true;
                        break;
                    case SAME:
                        file2->boIsDup = true;  // skip this file as file1
                        // duplicateChecked = true; // In case the files are already hard linked, we skip further checks and check the second file later...
                        break;
                    case SKIP:
                        // okay, it seems that this pair should not be processed...
                        duplicateChecked = true;
                        break;
                    case DIFFERENT:
                        // yeah, just do nothing.
                        break;
                    }
                    boNewFile1 = false;
                } /* for */
                file1->removeReference();
            }
            logVerbose(L"File group with %I64i bytes completed.", sg->i64GetFileSize());
            delete sg;
        }

        // Step 3: Show search results
        fflush(stderr);
        logInfo(L"Found %i duplicate files, savings of %I64i (%0.4g) bytes possible.", duplicates->getDuplicateCount(), duplicates->i64GetTotalDuplicateSize(), 1.0*duplicates->i64GetTotalDuplicateSize());
        fflush(stdout);
    }

    /**
     * Processes all duplicates and crestes hard links of the files
     */
    void linkAllDuplicates() {
        INT64 i64SumSize = 0;

        if (duplicates->getDuplicateCount() > 0) {
            DuplicateEntry* de;

            fflush(stderr);
            logInfo(L"Hard linking %i duplicate files", duplicates->getDuplicateCount());
            fflush(stdout);
            // Loop over all found files...
            while ((de = duplicates->popDuplicate()) != NULL) {
                if (fs->hardLinkFiles(de->getFile1(), de->getFile2())) {
                    i64SumSize += de->getFile1()->i64GetSize();
                } /* hardLinkFiles() OK */
                else { /* hardLinkFiles() error */
                  #if 0  /* 2012-10-12  HaSchm: Don't abort on link error like 1142 (to many links) */
                    throw L"Unable to process links";
                  #else
                    DWORD dwError = GetLastError();
                    if (dwError != 0) {
                        logError(dwError, L"Unable to process links");
                    } else {
                        logError(L"Unable to process links");
                    }
                    iExitCode = 1;
                  #endif
                } /* hardLinkFiles() error */
                delete de;
            }
            fflush(stderr);
            logInfo(L"Hard linking done, %I64i (%0.4g) bytes saved.", i64SumSize, 1.0*i64SumSize);
            fflush(stdout);
            Statistics::getInstance()->i64BytesSaved+=i64SumSize;  /* 2012-10-12  HaSchm */
        }
    }

    /**
     * Delete all duplicates entries
     */
    void deleteDuplicatesList() {
        duplicates->deleteAll();
    }

    /**
     * Displays the result duplicate list to stdout
     */
    void listDuplicates() {
        DuplicateEntry* de;
        if ((de = duplicates->next()) != NULL) {
            logInfo(L"Result of duplicate analysis:");
            do {
                de->getFile1()->copyName(cleanString(file1Name));
                de->getFile2()->copyName(cleanString(file2Name));
                logInfo(L"%I64i bytes: %s = %s", de->getFile1()->i64GetSize(), file1Name, file2Name);
                /* delete de;  2012-10-12  HaSchm: No delete here! */
            } while ((de = duplicates->next()) != NULL);
        } else {
            logInfo(L"No duplicates to list.");
        }
    }

    /**
     * Get number ob duplicates found
     */
    int getDuplicateCount() {
        return duplicates->getDuplicateCount();
    }


    /**
     * Prepare lists for next level processing
     */
    bool prepareNextLevel() {
        if (pathsNextLevel->getSize()==0) return false;
        paths->addAll(pathsNextLevel);
        return true;
    }
};

/**
 * Helper function to parse the command line
 * @param argc Argument Counter
 * @param argv Argument Vector
 * @param prog Program Instance Reference to fill with options
 */
bool parseCommandLine(int argc, char* argv[], DuplicateFileHardLinker* prog) {
    bool pathAdded = false;

    // iterate over all arguments...
    for (int i=1; i<argc; i++) {

        // first check if command line option
        if (argv[i][0] == '-' || argv[i][0] == '/') {

            if (strlen(argv[i]) == 2) {
                switch (argv[i][1]) {
                case '?':
                    // Show program usage
                    WCHAR programName[MAX_PATH_LENGTH];
                    mbstowcs(programName,argv[0],sizeof(programName));
                    logInfo(PROGRAM_NAME);
                    logInfo(L"Program to link duplicate files in several paths on one disk.");
                    logInfo(L"%s - %s", PROGRAM_VERSION, PROGRAM_AUTHOR);
                    logInfo(L"");
                    logInfo(L"NOTE: Use this tool on your own risk!");
                    logInfo(L"");
                    logInfo(L"Usage:");
                    logInfo(L"%s [options] [path] [...]", programName);
                    logInfo(L"Options:");
                    logInfo(L"/?\tShows this help screen");
                    logInfo(L"/a\tFile attributes must match for linking (expect Archive)");
                    logInfo(L"/d\tDebug mode");
                    logInfo(L"/e\tWrite error messages to standard output (not stderr)");  /* 2012-10-15  HaSchm */
                    logInfo(L"/h\tProcess hidden files");
                    logInfo(L"/i\tProcess recursively, but every level individually");
                    logInfo(L"/j\tAlso follow junctions (=reparse points) in filesystem");
                    //logInfo(L"/l\tHard links for files. If not specified, tool will just read (test) for duplicates");
                    logInfo(L"/l\tCreate hard links for files (default is just read/test for duplicates)"); // 2012-01-23  HaSchm
                    logInfo(L"/m\tAlso process small files <1024 bytes, they are skipped by default");
                    logInfo(L"/n\tFile names must match for linking");
                    //logInfo(L"/NoFileName\tDon't show files which are hard linked");
                    logInfo(L"/o\tList duplicate file result to stdout (for use without /l, /q)");
                    logInfo(L"/q\tSilent mode");
                    logInfo(L"/q1\tDon't show files which are hard linked");
                    logInfo(L"/r\tRuns recursively through the given folder list");
                    logInfo(L"/s\tProcess system files");
                    logInfo(L"/t\tTime + Date of files must match");
                    logInfo(L"/v\tVerbose mode");
                    logInfo(L"/w\tShow statistics after processing");
                    throw L""; //just to terminate the program...
                    break;
                case 'a':
                    prog->setAttributeMustMatch(true);
                    break;
                case 'd':
                    setLogLevel(LOG_DEBUG);
                    break;
                case 'e':  /* 2012-10-15  HaSchm */
                    boNoStdErr = true;
                    break;
                case 'h':
                    prog->setHiddenFiles(true);
                    break;
                case 'i':
                    prog->boLevelIndividually = true;
                    prog->setRecursive(true);
                    break;
                case 'j':
                    prog->setFollowJunctions(true);
                    break;
                case 'l':
                    reallyLink = true;
                    break;
                case 'm':
                    prog->setSmallFiles(true);
                    break;
                case 'n':
                    prog->vSetNameMustMatch(true);  /* 2012-10-22  HaSchm */
                    break;
                case 'o':
                    outputList = true;
                    break;
                case 'q':
                    setLogLevel(LOG_ERROR);
                    break;
                case 'r':
                    prog->setRecursive(true);
                    break;
                case 's':
                    prog->setSystemFiles(true);
                    break;
                case 't':
                    prog->setDateMatch(true);
                    break;
                case 'v':
                    setLogLevel(LOG_VERBOSE);
                    break;
                case 'w':
                    showStatistics = true;
                    break;
                default:
                    logError(L"Illegal Command line option! Use /? to see valid options!");
                    return false;
                }
            } /* if (strlen(argv[i]) == 2 */
            else if (stricmp(&argv[i][1],"NoFileName")==0
                  || strcmp(&argv[i][1],"q1")==0) {
                    boNoFileNameLog = true;  /* 2012-10-12  HaSchm */
            }
            else {

                logError(L"Illegal Command line option! Use /? to see valid options!");
                return false;
            }
        } else {
            // the command line options seems to be a path...
            WCHAR tmpPath[MAX_PATH_LENGTH];
            mbstowcs(tmpPath,argv[i],sizeof(tmpPath));

            // check if the path is existing!
            WCHAR DirSpec[MAX_PATH_LENGTH];  // directory specification
            wcsncpy(DirSpec, tmpPath, wcslen(tmpPath)+1);
            wcsncat(DirSpec, L"\\*", 3);
            WIN32_FIND_DATA FindFileData;
            HANDLE hFind = FindFirstFile(DirSpec, &FindFileData);
            if (hFind == INVALID_HANDLE_VALUE) {
                logError(L"Specified directory \"%s\" does not exist", tmpPath);
                return false;
            }
            FindClose(hFind);

            prog->addPath(tmpPath);
            pathAdded = true;
        }
    }

    // check for parameters
    if (!pathAdded) {
        logError(L"You need to specify at least one folder to process!\n"
                 L"Use /? to see valid options!");
        return false;
    }

    return true;
}

/**
 * Main runnable and entry point for executing the application
 * @param argc Argument Counter
 * @param argv Argument Vector
 * @return Application Return code
 */
int main(int argc, char* argv[]) {
    int iTotalDuplicateCount = 0;
    DuplicateFileHardLinker* prog = new DuplicateFileHardLinker();

    try {
        // parse the command line
        if (!parseCommandLine(argc, argv, prog)) {
            return 1;  /* 2012-02-23  HaSchm: Do not use -1, as standard "if errorlevel 1" does not work */
        }

        // show desired option info
        logInfo(PROGRAM_NAME);
        logInfo(L"%s - %s", PROGRAM_VERSION, PROGRAM_AUTHOR);
        logInfo(L"");

        do {  /* to process levels individually */
            // find duplicates
            prog->findDuplicates();

            if (outputList) {
                prog->listDuplicates();
            }

            iTotalDuplicateCount += prog->getDuplicateCount();
            if (reallyLink) {
                // link duplicates
                prog->linkAllDuplicates();
            }
            prog->deleteDuplicatesList();
        } while (prog->prepareNextLevel());

        if (iTotalDuplicateCount=0) logInfo(L"No files found for linking");
        else if (!reallyLink) {
            logInfo(L"Skipping real linking. To really create hard links, use the /l switch.");
        }

    } catch (LPCWSTR err) {
        DWORD dwError = GetLastError();
        if (wcslen(err) > 0) {
            if (dwError != 0) {
                logError(dwError, err);
            } else {
                logError(err);
            }
        }
        iExitCode = 1;  /* 2012-02-23  HaSchm: Do not use -1, as standard "if errorlevel 1" does not work */
    }

    delete prog;

    // show processing statistics
    if (showStatistics) {
        fflush(stderr);
        setLogLevel(LOG_INFO);  /* 2012-10-12  HaSchm */
        logInfo(L"");
        logInfo(L"Processing statistics:");
        Statistics* s = Statistics::getInstance();
        logInfo(L"Number of file comparisons %I64i (%0.4g)", s->i64FileCompares, 1.0*s->i64FileCompares);
        logInfo(L"Number of file comparisons using a hash: %I64i", s->i64HashCompares);
        logInfo(L"Number of file comparisons using a hash, both hashes were available before: %I64i", s->i64HashComparesHit1);
        /* Note: Since hash is checked before meta data, values for i64HashComparesHit1,
                 i64FileNameMismatch, i64FileAttributeMismatch and i64FileMTimeMismatch
                 depend on (more or less random) sequence of compares. */
        logInfo(L"Number of file comparisons using a hash, one hash was missing before: %I64i", s->i64HashComparesHit2);
        logInfo(L"Number of file comparisons using a hash, both hashes were missing before: %I64i", s->i64HashComparesHit3);
        /* logInfo(L"File meta data not matching: %I64i", s->i64FileMetaDataMismatch);  2012-10-12  HaSchm */
        if (s->i64FileNameMismatch!=0) logInfo(L"File names not matching: %I64i", s->i64FileNameMismatch);
        if (s->i64FileAttributeMismatch!=0) logInfo(L"File attributes not matching: %I64i", s->i64FileAttributeMismatch);
        if (s->i64FileMTimeMismatch!=0) logInfo(L"File modification time not matching: %I64i", s->i64FileMTimeMismatch);
        logInfo(L"Files were already linked: %I64i", s->i64FileAlreadyLinked);
        logInfo(L"Files content differed in first %i bytes: %I64i", FIRST_BLOCK_SIZE, s->i64FileContentDifferFirstBlock);
        logInfo(L"Files content differ after %i bytes: %I64i", FIRST_BLOCK_SIZE, s->i64FileContentDifferLater);
        logInfo(L"Files content is same: %I64i", s->i64FileContentSame);
        logInfo(L"File compare problems: %i", s->fileCompareProblems);
        logInfo(L"Directory open problems: %i", s->directoryOpenProblems);  /* 2012-10-16  HaSchm */
        logInfo(L"Number of file hashes calculated: %I64i", s->i64HashesCalculated);
        logInfo(L"Number of directories found: %i (incl. %i junctions)", s->directoriesFound, s->junctionsFound);
        logInfo(L"Number of files found: %I64i (%0.4g), incl. %I64i (%0.4g) h or s files", s->i64FilesFound, 1.0*s->i64FilesFound, s->i64HSFilesFound, 1.0*s->i64HSFilesFound);
        logInfo(L"Number of files which were filtered by size: %I64i (%0.4g), %I64i (%0.4g) bytes", s->i64FileTooSmall, 1.0*s->i64FileTooSmall, s->i64TotalSmallFileSize, 1.0*s->i64TotalSmallFileSize);
        logInfo(L"Number of bytes read from disk: %I64i (%0.4g)", s->i64BytesRead, 1.0*s->i64BytesRead);
        logInfo(L"Number of files opened: %I64i (%0.4g)", s->i64FilesOpened, 1.0*s->i64FilesOpened);
        #ifdef _DEBUG
            logInfo(L"Number of files closed: %I64i (%0.4g)", s->i64FilesClosed, 1.0*s->i64FilesClosed);
        #endif
        logInfo(L"Number of file open problems: %i", s->fileOpenProblems);
        logInfo(L"Number of successful hard link generations: %I64i (%0.4g)", s->i64HardLinksSuccess, 1.0*s->i64HardLinksSuccess);
        logInfo(L"Number of hard link generation errors: %I64i", (s->i64HardLinks-s->i64HardLinksSuccess));
        logInfo(L"Number of bytes saved: %I64i (%0.4g)", s->i64BytesSaved, 1.0*s->i64BytesSaved);  /* 2012-10-12  HaSchm */
        #ifdef _DEBUG
            logInfo(L"Path objects created: %i", s->pathObjCreated);
            logInfo(L"Path objects destroyed: %i", s->pathObjDestroyed);
            logInfo(L"File objects created: %I64i", s->i64FileObjCreated);
            logInfo(L"File objects destroyed: %I64i", s->i64FileObjDestroyed);
            logInfo(L"UnbufferedFileStream objects created: %I64i", s->i64UnbufferedFileStreamObjCreated);
            logInfo(L"UnbufferedFileStream objects destroyed: %I64i", s->i64UnbufferedFileStreamObjDestroyed);
            logInfo(L"FileSystem objects created: %I64i", s->i64FileSystemObjCreated);
            logInfo(L"FileSystem objects destroyed: %I64i", s->i64FileSystemObjDestroyed);
            logInfo(L"Collection objects created: %I64i", s->i64CollectionObjCreated);
            logInfo(L"Collection objects destroyed: %I64i", s->i64CollectionObjDestroyed);
            logInfo(L"Item objects created: %I64i", s->i64ItemObjCreated);
            logInfo(L"Item objects destroyed: %I64i", s->i64ItemObjDestroyed);
            logInfo(L"ReferenceCounter objects created: %I64i", s->i64ReferenceCounterObjCreated);
            logInfo(L"ReferenceCounter objects destroyed: %I64i", s->i64ReferenceCounterObjDestroyed);
            logInfo(L"SizeGroup objects created: %I64i", s->i64SizeGroupObjCreated);
            logInfo(L"SizeGroup objects destroyed: %I64i", s->i64SizeGroupObjDestroyed);
            logInfo(L"SortedFileCollection objects created: %I64i", s->i64SortedFileCollectionObjCreated);
            logInfo(L"SortedFileCollection objects destroyed: %I64i", s->i64SortedFileCollectionObjDestroyed);
            logInfo(L"DuplicateEntry objects created: %I64i", s->i64DuplicateEntryObjCreated);
            logInfo(L"DuplicateEntry objects destroyed: %I64i", s->i64DuplicateEntryObjDestroyed);
            logInfo(L"DuplicateFileCollection objects created: %I64i", s->i64DuplicateFileCollectionObjCreated);
            logInfo(L"DuplicateFileCollection objects destroyed: %I64i", s->i64DuplicateFileCollectionObjDestroyed);
            logInfo(L"DuplicateFileHardLinker objects created: %I64i", s->i64DuplicateFileHardLinkerObjCreated);
            logInfo(L"DuplicateFileHardLinker objects destroyed: %I64i", s->i64DuplicateFileHardLinkerObjDestroyed);
        #endif
    }
    return iExitCode;
} /* main */


/* eof DFHL.cpp */
