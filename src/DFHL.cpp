/* DFHL.cpp : Defines the entry point for the console application.

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
#include "Util.h"
#include "FileSystem.h"


// Global Definitions
// *******************************************
#define FIRST_BLOCK_SIZE		65536 // Smaller block size
#define BLOCK_SIZE				4194304 // 4MB seems to be a good value for performance without too much memory load
#define MIN_FILE_SIZE			1024 // Minimum file size so that hard linking will be checked...

#define PROGRAM_NAME		L"Duplicate File Hard Linker"
#define PROGRAM_VERSION     L"Version 2.0"
#define PROGRAM_AUTHOR      L"Jens Scheffler and Oliver Schneider, http://www.jensscheffler.de"

enum CompareResult {
	EQUAL,			// File compare was successful and content is matching
	SAME,			// Files are already hard linked
	SKIP,			// File 1 should not be processed (filter!)
	DIFFERENT		// Files differ
};


// Global Variables
// *******************************************
/** Flag if list should be displayed */
bool outputList = false;
/** Flag if running in real or test mode */
bool reallyLink = false;
/** Flag if statistics should be displayed */
bool showStatistics = false;



// Global Classes
// *******************************************


/**
 * Class implementation for a group of files having the same size
 */
class SizeGroup {
private:
	INT64 fileSize;
	Collection* items;
	SizeGroup* next;
public:
	/**
	 * Creates a new FileGroup with a single file as start
	 */
	SizeGroup(File* newFile, SizeGroup* newNext) {
		Statistics::getInstance()->sizeGroupObjCreated++;
		fileSize = newFile->getSize();
		items = new Collection();
		items->append(newFile);
		next = newNext;
	}

	~SizeGroup() {
		Statistics::getInstance()->sizeGroupObjDestroyed++;
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
	INT64 getFileSize() {
		return fileSize;
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
	INT64 totalSize;
	SizeGroup* first;
public:
	SortedFileCollection() {
		Statistics::getInstance()->sortedFileCollectionObjCreated++;
		files = 0;
		groups = 0;
		totalSize = 0;
		first = NULL;
	}

	~SortedFileCollection() {
		Statistics::getInstance()->sortedFileCollectionObjDestroyed++;
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
		while (g != NULL && g->getFileSize() < newFile->getSize()) {
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
			if (g->getFileSize() == newFile->getSize()) {
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
		totalSize += newFile->getSize();
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

	INT64 getTotalFileSize() {
		return totalSize;
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
		Statistics::getInstance()->duplicateEntryObjCreated++;
		file1 = newFile1;
		file1->addReference();
		file2 = newFile2;
		file2->addReference();
	}

	~DuplicateEntry() {
		Statistics::getInstance()->duplicateEntryObjDestroyed++;
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
	INT64 duplicateSize;
	Collection* items;
public:
	DuplicateFileCollection() {
		Statistics::getInstance()->duplicateFileCollectionObjCreated++;
		items = new Collection();
		duplicateSize = 0;
	}

	~DuplicateFileCollection() {
		Statistics::getInstance()->duplicateFileCollectionObjDestroyed++;
		// remove orphan references of duplicates if NOT linking...
		DuplicateEntry* leftover;
		while ((leftover = popDuplicate()) != NULL) {
			delete leftover;
		}
		delete items;
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
		duplicateSize += newDuplicate->getFile1()->getSize();
		items->append(newDuplicate);
	}

	DuplicateEntry* next() {
		return (DuplicateEntry*) items->next();
	}

	int getDuplicateCount() {
		return items->getSize();
	}
	INT64 getTotalDuplicateSize() {
		return duplicateSize;
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
	/** Sorted File Collection of files to check and process */
	SortedFileCollection* files;
	/** List of duplicates to process */
	DuplicateFileCollection* duplicates;
	/** buffer variables for file compare */
	LPBYTE block1;
	LPBYTE block2;
	/** temporary file name buffers */
	LPWSTR file1Name;
	LPWSTR file2Name;
	/** Temporary file information storages */
	BY_HANDLE_FILE_INFORMATION info1;
	BY_HANDLE_FILE_INFORMATION info2;
	/** Flag if attributes of file need to match */
	bool attributeMustMatch;
	/** Flag if small files (<1024 bytes) should be processed */
	bool smallFiles;
	/** Flag if recursive processing should be enabled */
	bool recursive;
	/** Flag if timestamps of file need to match */
	bool dateTimeMustMatch;

	/**
	 * Calculates a hash from byte data
	 * @param bytes Array of bytes to analyze
	 * @param length number of bytes to use for calculation
	 * @return calculated hash value
	 */
	INT64 generateHash(LPBYTE block, int length) {
		Statistics::getInstance()->hashesCalculated++;
		INT64 result = 0;
		INT64 temp;
		for (int i = 0; i + 7 < length; i+=8) {
			temp = (INT64) block2[i];
			result = result ^ temp;
		}
		// just ignore the leftover bytes if the length is not diviable by 8 (which is not the case in out code...)
		return result;
	}

	/**
	 * Compares the given 2 files content
	 */
	CompareResult compareFiles(File* file1, File* file2) {
		logVerbose(L"Comparing files \"%s\" and \"%s\".", file1Name, file2Name);
		Statistics::getInstance()->fileCompares++;

		// optimization: check the file hash codes (if existing already)
		if (file1->isHashAvailable() && file2->isHashAvailable() && file1->getHash() != file2->getHash()) {
			// okay, hashes are different, files seem to differ!
			logVerbose(L"Files differ in content (hash).");
			Statistics::getInstance()->hashCompares++;
			Statistics::getInstance()->hashComparesHit1++;
			return DIFFERENT;
		}

		// check for attributes matching
		if (attributeMustMatch && file1->getAttributes() == file2->getAttributes()) {
			logVerbose(L"Attributes of files do not match, skipping.");
			Statistics::getInstance()->fileMetaDataMismatch++;
			return DIFFERENT;
		}

		// check for time stamp matching
		if (dateTimeMustMatch && (
				file1->getLastModifyTime().dwHighDateTime != file2->getLastModifyTime().dwHighDateTime ||
				file1->getLastModifyTime().dwLowDateTime  != file2->getLastModifyTime().dwLowDateTime
				)) {
			logVerbose(L"Modification timestamps of files do not match, skipping.");
			Statistics::getInstance()->fileMetaDataMismatch++;
			return DIFFERENT;
		}

		// doublecheck data consistency!
		file1->copyName(cleanString(file1Name));
		file2->copyName(cleanString(file2Name));
		if (file1->equals(file2)) {
			logError(L"Same file \"%s\"found as duplicate, ignoring!", file1Name);
			Statistics::getInstance()->fileCompareProblems++;
			return SKIP;
		}

		// Optimization #345:
		// Maybe we already have a hash of file #1, so we start opening and processing file #2 first...

		// Open File 2
		UnbufferedFileStream* fs2 = new UnbufferedFileStream(file2);
		if (!fs2->open()) {
			logError(L"Unable to open file \"%s\"", file2Name);
			delete fs2;
			Statistics::getInstance()->fileCompareProblems++;
			return DIFFERENT; // assume that only file2 is not to be opened, signal different file
		}

		// Read File Content and compare
		INT64 bytesToRead = file2->getSize();
		int read2;
		memset(block2, 0, FIRST_BLOCK_SIZE); // clean the second block for later hash calculations
		read2 = fs2->read(block2, FIRST_BLOCK_SIZE);

		// calculate the hash of the block of file 2 if not done so far...
		if (!file2->isHashAvailable()) {
			file2->setHash(generateHash(block2, FIRST_BLOCK_SIZE));

			// If we have a hash of file #1 also, we can compare now...
			if (file1->isHashAvailable() && file1->getHash() != file2->getHash()) {
				// okay, hashes are different, files seem to differ!
				logVerbose(L"Files differ in content (hash).");
				fs2->close();
				delete fs2;
				Statistics::getInstance()->hashCompares++;
				Statistics::getInstance()->hashComparesHit2++;
				return DIFFERENT;
			}
		}

		// Open File 1 - now file #1 must be opened for further compares...
		UnbufferedFileStream* fs1 = new UnbufferedFileStream(file1);
		if (!fs1->open()) {
			logError(L"Unable to open file \"%s\"", file1Name);
			fs2->close();
			delete fs1;
			delete fs2;
			Statistics::getInstance()->fileCompareProblems++;
			return SKIP; // file 1 can not be opened, skip this file for further compares!
		}

		// Read File Content of file #1 now and compare
		int read1;
		memset(block1, 0, FIRST_BLOCK_SIZE); // clean the first block for later hash calculations
		read1 = fs1->read(block1, FIRST_BLOCK_SIZE);

		// generate hash for file #1 if not donw so far...
		if (!file1->isHashAvailable()) {
			file1->setHash(generateHash(block1, FIRST_BLOCK_SIZE));

			// Finally we can be sure that we have hashes of both files, compare them first (faster?)
			if (file1->getHash() != file2->getHash()) {
				// okay, hashes are different, files seem to differ!
				logVerbose(L"Files differ in content (hash).");
				fs1->close();
				fs2->close();
				delete fs1;
				delete fs2;
				Statistics::getInstance()->hashCompares++;
				Statistics::getInstance()->hashComparesHit3++;
				return DIFFERENT;
			}
		}

		// Check if both files are already linked
		if (fs1->getFileDetails(&info1) && fs2->getFileDetails(&info2)) {
			if (info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber &&
				info1.nFileIndexHigh == info2.nFileIndexHigh &&
				info1.nFileIndexLow == info2.nFileIndexLow) {

				logVerbose(L"Files are already hard linked, skipping.");
				fs1->close();
				fs2->close();
				delete fs1;
				delete fs2;
				Statistics::getInstance()->fileAlreadyLinked++;
				return SAME;
			}
		}

		// compare all bytes now - as the hashes seem to be equal
		for (int i = 0; i < read1; i++) {
			if (block1[i] != block2[i]) {
				logVerbose(L"Files differ in content.");
				fs1->close();
				fs2->close();
				delete fs1;
				delete fs2;
				Statistics::getInstance()->fileContentDifferFirstBlock++;
				return DIFFERENT;
			}
		}

		// okay, now optimized fast start was done, run in a loop to compare the rest of the file content!
		bytesToRead -= read1;
		bool switcher = true; // helper variable for performance optimization
		while (bytesToRead > 0) {

			// Read Blocks - Performance boosted: read alternating to mimimize head shifts... ;-)
			if (switcher) {
				read1 = fs1->read(block1, BLOCK_SIZE);
				read2 = fs2->read(block2, BLOCK_SIZE);
			} else {
				read2 = fs2->read(block2, BLOCK_SIZE);
				read1 = fs1->read(block1, BLOCK_SIZE);
			}

			// change the state for the next read operation
			switcher = !switcher; // alternate read

			// Compare Data
			bytesToRead -= read1;
			if (read1 != read2 || read1 == 0) {
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
					Statistics::getInstance()->fileContentDifferLater++;
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
		Statistics::getInstance()->fileContentSame++;
		return EQUAL;
	}

	/**
	 * Adds a file to the collection of files to process
	 * @param file FindFile Structure of further file information
	 */
	void addFile(File* newFile) {
		newFile->addReference();
		files->addFile(newFile);
	}

	/**
	 * Compares the given 2 files content
	 */
	void addDuplicate(File* file1, File* file2) {
		duplicates->addDuplicate(new DuplicateEntry(file1, file2));
	}

public:

	/**
	 * Constructor for the Object
	 */
	DuplicateFileHardLinker() {
		Statistics::getInstance()->duplicateFileHardLinkerObjCreated++;
		fs = new FileSystem();
		paths = new Collection();
		files = new SortedFileCollection();
		duplicates = new DuplicateFileCollection();
		file1Name = new WCHAR[MAX_PATH_LENGTH];
		file2Name = new WCHAR[MAX_PATH_LENGTH];
		attributeMustMatch = false;
		smallFiles = false;
		recursive = false;
		dateTimeMustMatch = false;
		block1 = new BYTE[BLOCK_SIZE];
		block2 = new BYTE[BLOCK_SIZE];
	}

	~DuplicateFileHardLinker() {
		Statistics::getInstance()->duplicateFileHardLinkerObjDestroyed++;
		delete fs;
		delete paths;
		delete files;
		delete duplicates;
		if (block1 != NULL) {
			delete block1;
		}
		if (block2 != NULL) {
			delete block2;
		}
		delete file1Name;
		delete file2Name;
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
	 * @param path Path name to add to the collection
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
		logInfo(L"Parsing Directory Tree...");
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
					if (aFile->getSize() == 0 || (!smallFiles && aFile->getSize() < MIN_FILE_SIZE)) {
						logDebug(L"ignoring file \"%s\", is too small.", aFile->getName());
						aFile->removeReference();
						Statistics::getInstance()->fileTooSmall++;
					} else {
						files->addFile(aFile);
					}
				} else {

					// okay, we found a directory...
					if (recursive) {
						paths->push(aItem);
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
		logInfo(L"Found %i Files (%I64i bytes) in folders, comparing relevant files.", files->getFileCount(), files->getTotalFileSize());
		SizeGroup* sg;
		File* file1;
		File* file2;
		bool duplicateChecked;
		while ((sg = files->popGroup()) != NULL) { // process all size groups
			logVerbose(L"Processing file group with %I64i bytes, comparing %i files...", sg->getFileSize(), sg->getFileCount());
			while ((file1 = sg->popFile()) != NULL) { // process all files in the group
				duplicateChecked = false; // flag if the file 1 was compared "enough" and next file should be taken
				for (int i = 0; i < sg->getFileCount() && !duplicateChecked; i++) {
					file2 = sg->getNextFile();
					DWORD start = GetTickCount();
					DWORD time = 0;
					switch (compareFiles(file1, file2))
					{
					case EQUAL:
						time = GetTickCount() - start;
						logDebug(L"file compare took %ims, %I64i KB/s", time, (time>0)?(file1->getSize() * 2 * 1000 / time / 1024):0);

						// Files seem to be equal, marking them for later processing...
						addDuplicate(file1, file2);
						duplicateChecked = true;
						break;
					case SAME:
						// In case the files are already hard linked, we skip further checks and check the second file later...
						duplicateChecked = true;
						break;
					case SKIP:
						// okay, it seems that this pair should not be processed...
						duplicateChecked = true;
						break;
					case DIFFERENT:
						// yeah, just do nothing.
						break;
					}
				}
				file1->removeReference();
			}
			logVerbose(L"File group with %I64i bytes completed.", sg->getFileSize());
			delete sg;
		}

		// Step 3: Show search results
		logInfo(L"Found %i duplicate files, savings of %I64i bytes possible.", duplicates->getDuplicateCount(), duplicates->getTotalDuplicateSize());
	}

	/**
	 * Processes all duplicates and crestes hard links of the files
	 */
	void linkAllDuplicates() {
		INT64 sumSize = 0;

		if (duplicates->getDuplicateCount() > 0) {
			// Loop over all found files...
			logInfo(L"Hard linking %i duplicate files", duplicates->getDuplicateCount());
			DuplicateEntry* de;
			while ((de = duplicates->popDuplicate()) != NULL) {
				sumSize += de->getFile1()->getSize();
				if (!fs->hardLinkFiles(de->getFile1(), de->getFile2())) {
					throw L"Unable to process links";
				}
				delete de;
			}
			logInfo(L"Hard linking done, %I64i bytes saved.", sumSize);
		} else {
			logInfo(L"No files found for linking");
		}
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
				logInfo(L"%I64i bytes: %s = %s", de->getFile1()->getSize(), file1Name, file2Name);
				delete de;
			} while ((de = duplicates->next()) != NULL);
		} else {
			logInfo(L"No duplicates to list.");
		}
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
					logInfo(L"/a\tFile attributes must match for linking");
					logInfo(L"/d\tDebug mode");
					logInfo(L"/h\tProcess hidden files");
					logInfo(L"/j\tAlso follow junctions (=reparse points) in filesystem");
					logInfo(L"/l\tHard links for files. If not specified, tool will just read (test) for duplicates");
					logInfo(L"/m\tAlso process small files <1024 bytes, they are skipped by default");
					logInfo(L"/o\tList duplicate file result to stdout");
					logInfo(L"/q\tSilent mode");
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
				case 'h':
					prog->setHiddenFiles(true);
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
			} else {
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
			} else {
				FindClose(hFind);
			}

			prog->addPath(tmpPath);
			pathAdded = true;
		}
	}

	// check for parameters
	if (!pathAdded) {
		logError(L"You need to specify at least one folder to process!\nUse /? to see valid options!");
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
	int result = 0;
	DuplicateFileHardLinker* prog = new DuplicateFileHardLinker();

	try {
		// parse the command line
		if (!parseCommandLine(argc, argv, prog)) {
			return -1;
		}

		// show desired option info
		logInfo(PROGRAM_NAME);
		logInfo(L"%s - %s", PROGRAM_VERSION, PROGRAM_AUTHOR);
		logInfo(L"");

		// find duplicates
		prog->findDuplicates();

		if (outputList) {
			prog->listDuplicates();
		}

		if (reallyLink) {
			// link duplicates
			prog->linkAllDuplicates();
		} else {
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
		result = -1;
	}

	delete prog;

	// show processing statistics
	if (showStatistics) {
		logInfo(L"");
		logInfo(L"Processing statistics:");
		Statistics* s = Statistics::getInstance();
		logInfo(L"Number of files compared: %i", s->fileCompares);
		logInfo(L"Number of files compared using a hash: %i", s->hashCompares);
		logInfo(L"Number of files compared using a hash, both hashes were available before: %i", s->hashComparesHit1);
		logInfo(L"Number of files compared using a hash, one hash was missing before: %i", s->hashComparesHit2);
		logInfo(L"Number of files compared using a hash, both hashes were missing before: %i", s->hashComparesHit3);
		logInfo(L"File mata data not matching: %i", s->fileMetaDataMismatch);
		logInfo(L"Files were already linked: %i", s->fileAlreadyLinked);
		logInfo(L"Files content differed in first %i bytes: %i", FIRST_BLOCK_SIZE, s->fileContentDifferFirstBlock);
		logInfo(L"Files content differ after %i bytes: %i", FIRST_BLOCK_SIZE, s->fileContentDifferLater);
		logInfo(L"Files content is same: %i", s->fileContentSame);
		logInfo(L"File compare problems: %i", s->fileCompareProblems);
		logInfo(L"Number of file hashes calculated: %i", s->hashesCalculated);
		logInfo(L"Number of directories found: %i", s->directoriesFound);
		logInfo(L"Number of Files found in directories: %i", s->filesFound);
		logInfo(L"Number of Files which were filtered by size: %i", s->fileTooSmall);
		logInfo(L"Number of bytes read from disk: %I64i", s->bytesRead);
		logInfo(L"Number of files opened: %i", s->filesOpened);
		#ifdef _DEBUG
			logInfo(L"Number of files closed: %i", s->filesClosed);
		#endif
		logInfo(L"Number of file open problems: %i", s->fileOpenProblems);
		logInfo(L"Number of hard links created: %i", s->hardLinks);
		logInfo(L"Number of successful link generations: %i", s->hardLinksSuccess);
		#ifdef _DEBUG
			logInfo(L"Path objects created: %i", s->pathObjCreated);
			logInfo(L"Path objects destroyed: %i", s->pathObjDestroyed);
			logInfo(L"File objects created: %i", s->fileObjCreated);
			logInfo(L"File objects destroyed: %i", s->fileObjDestroyed);
			logInfo(L"UnbufferedFileStream objects created: %i", s->unbufferedFileStreamObjCreated);
			logInfo(L"UnbufferedFileStream objects destroyed: %i", s->unbufferedFileStreamObjDestroyed);
			logInfo(L"FileSystem objects created: %i", s->fileSystemObjCreated);
			logInfo(L"FileSystem objects destroyed: %i", s->fileSystemObjDestroyed);
			logInfo(L"Collection objects created: %i", s->collectionObjCreated);
			logInfo(L"Collection objects destroyed: %i", s->collectionObjDestroyed);
			logInfo(L"Item objects created: %i", s->itemObjCreated);
			logInfo(L"Item objects destroyed: %i", s->itemObjDestroyed);
			logInfo(L"ReferenceCounter objects created: %i", s->referenceCounterObjCreated);
			logInfo(L"ReferenceCounter objects destroyed: %i", s->referenceCounterObjDestroyed);
			logInfo(L"SizeGroup objects created: %i", s->sizeGroupObjCreated);
			logInfo(L"SizeGroup objects destroyed: %i", s->sizeGroupObjDestroyed);
			logInfo(L"SortedFileCollection objects created: %i", s->sortedFileCollectionObjCreated);
			logInfo(L"SortedFileCollection objects destroyed: %i", s->sortedFileCollectionObjDestroyed);
			logInfo(L"DuplicateEntry objects created: %i", s->duplicateEntryObjCreated);
			logInfo(L"DuplicateEntry objects destroyed: %i", s->duplicateEntryObjDestroyed);
			logInfo(L"DuplicateFileCollection objects created: %i", s->duplicateFileCollectionObjCreated);
			logInfo(L"DuplicateFileCollection objects destroyed: %i", s->duplicateFileCollectionObjDestroyed);
			logInfo(L"DuplicateFileHardLinker objects created: %i", s->duplicateFileHardLinkerObjCreated);
			logInfo(L"DuplicateFileHardLinker objects destroyed: %i", s->duplicateFileHardLinkerObjDestroyed);
		#endif
	}
	return result;
}
