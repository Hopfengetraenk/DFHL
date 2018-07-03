/* FileSystem.h : Headers for File System specific functions for DFHL

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
#define MAX_PATH_LENGTH			32768
#define PATH_SEPARATOR			L"\\" // Path separator in the file system
#define LONG_FILENAME_PREFIX	L"\\\\?\\" // Prefix to put ahead of a long path for Windows API


/**
 * Abstract base class for all entries of a folder in the file system
 */
class FolderEntry : public ReferenceCounter {
protected:
	bool file;
public:
	/**
	 * Checks if this object represents a file
	 */
	bool isFile();

	/**
	 * Checks if this object represents a file
	 */
	bool isFolder();
};


/**
 * Comtainer class which represents a folder in the file system and creates the directory hierarchy in the file system
 */
class Path : public FolderEntry {
private:
	LPWSTR name;
	Path* parent;
public:
	/**
	 * Constructor
	 */
	Path(LPWSTR newName, Path* newParent);

	/**
	 * Destructor
	 */
	~Path();

	/**
	 * Recursively copies the name of this path into the specified buffer.
	 * If this entry has a parent, all parents up to the root of the file system are copied to buffer
	 * @param buffer temporary buffer variable where the path should be copied to
	 */
	void copyName(LPWSTR buffer);

	/**
	 * Checks if a file reference equals an other file reference
	 */
	bool equals(Path* otherPath);
};


/**
 * Container class which represents a file in the file system
 */
class File : public FolderEntry {
private:
	LPWSTR name;
	LPWSTR shortName;
	Path* parentFolder;
	INT64 size;
	DWORD attributes;
	FILETIME lastModifyTime;
	INT64 hash;
	bool hashAvailable;
public:
	/**
	 * Constructor
	 */
	File(WIN32_FIND_DATA fileInfo, Path* newParent);

	/**
	 * Destructor
	 */
	~File();

	/**
	 * copies the absolute file system name of this file into the specified buffer.
	 * @param buffer temporary buffer variable where the path should be copied to
	 */
	void copyName(LPWSTR buffer);

	/**
	 * Retrieves a pointer to the (long) name of the file
	 */
	LPWSTR File::getName();

	/**
	 * Retrieves a pointer to the short name of the file
	 */
	LPWSTR File::getShortName();

	/**
	 * Getter for the size of the file
	 */
	INT64 getSize();

	/**
	 * Getter for the file attributes
	 */
	DWORD getAttributes();

	/**
	 * Getter for the last File modification time
	 */
	FILETIME getLastModifyTime();

	/**
	 * Checks if a hash was already calculated for the file
	 */
	bool isHashAvailable();

	/**
	 * @return The hash value of the file
	 */
	INT64 getHash();

	/**
	 * Sets The hash value of the file
	 */
	void setHash(INT64 newHash);

	/**
	 * Checks if a file reference equals an other file reference
	 */
	bool equals(File* otherFile);
};

/**
 * class implementation for a byte stream operations
 */
class UnbufferedFileStream {
private:
	File* file;
	HANDLE hFile;

public:

	/**
	 * Constructor
	 */
	UnbufferedFileStream(File* newFile);

	/**
	 * Destructor
	 */
	~UnbufferedFileStream();

	/**
	 * Open the file for reading
	 * @return Flag if the open operation was successful
	 */
	bool open();

	/**
	 * Reads data from the file
	 * @param buffer Pointer to buffer space where to copy the read bytes to
	 * @param bytesToRead Number of bytes which should be read in this operation
	 * @return Number of bytes which have been read and which were copied to buffer output
	 */
	int read(LPVOID buffer, int bytesToRead);

	/**
	 * Retrieves further file information from the opened file stream
	 * @param info Object where the details will be stored
	 * @return Flag if the operation was successful
	 */
	bool getFileDetails(BY_HANDLE_FILE_INFORMATION* info);

	/**
	 * Closes the file stream
	 */
	void close();
};


/**
 * logic implementation for abstraction of the file system
 */
class FileSystem {
private:
	bool followJunctions;
	bool hiddenFiles;
	bool systemFiles;
public:

	/**
	 * Constructor
	 */
	FileSystem();

	/**
	 * Destructor
	 */
	~FileSystem();

	/**
	 * Parses a path string and returns an appropriate path object
	 * @return Path object, NULL if the path is not existing
	 */
	Path* parsePath(LPCWSTR pathStr);

	/**
	 * Retrievs a list of items in the given folder
	 * @return Collection of FolderEntry Objects
	 * @throws LPWSTR If the file finding generates a unexpected error (Not access denied)
	 */
	Collection* getFolderContent(Path* folder);

	/**
	 * Links two files on hard disk by deletiong one and linking the name of this to the other
	 * @param file1 File name of the first file
	 * @param file2 File name of the second file to link to
	 * @return boolean value if operation was successful
	 */
	bool hardLinkFiles(File* file1, File* file2);

	/**
	 * Sets the configuration option for following junctions
	 */
	void setFollowJunctions(bool newValue);

	/**
	 * Sets the configuration option for using hidden files and directories
	 */
	void setHiddenFiles(bool newValue);

	/**
	 * Sets the configuration option for using system files and directories
	 */
	void setSystemFiles(bool newValue);
};
