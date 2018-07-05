# Duplicate File Hard Linker (DFHL)
Places Project in Git-Hub @ Jul 2018
* * *
## [Version 2.0 - Sep 2006](https://www.jensscheffler.de/dfhl)
#### Jens Scheffler & Oliver Schneider

Submitted by jscheffl on 16. September 2006 - 23:00

Small command line tool to reduce the size of duplicate files on one partition. Despite all other known tools the files in this implementation are not being listed, archived or removed but going to be hard linked using NTFS hard links. The tool runs in Windows NT 4.0 / 2000 / XP and 2003 Server and requires a NTFS file system to run on.

Features
--------

*   Command line Tool for Windows NT4.0/2000/XP/2003
*   Links duplicate files in file system using NTFS "Hard Link" facility. For further information please take a look to [Website at Microsoft](http://msdn.microsoft.com/en-us/library/aa365006%28VS.85%29.aspx).
*   Runs recursive through a tree of folders on one partition
*   Compares all files byte-per-byte to ensure really only equal files to be hard linked
*   Check-Mode (default) allows listing of all optimization possibilities. In this mode no changes will be done.
*   Additional options allow control of which parameters of files will be compared, to check attributes and time stamps of files
*   All read/compare operations were optimized for maximum performance and will reach almost the maximum throughput of the physical disk. The system will be loaded only in background.
*   File system short named (8.3 Names) can be lost during hard linking. This can lead to problems with programs requiring 8.3 names.
*   Further optimizations by using file hashes. A hash is now compared before running a byte-by-byte compare.

Limitations
-----------

*   Hard Links can only be created on same partition. A permission overriding link creation is not possible due to physical limitations.
*   Only NTFS is supported as file system for hard links.
*   All hard linked files share the same set of file attributes, security descriptors and timestamps. Changes on one file Meta attribute will reflect to the other linked file immediately. This is limited by the underlying file system.
*   When removing files which are hard linked, the space will only be freed on hard drive when the last reference of the file is going to be removed.
*   Already linked files can not be linked again to gain space - of course...

Command Line Parameters
-----------------------
```
DFHL \[options\] \[path\] \[...\]

  /l	Create hard >links for files (default is just read/test for duplicates)

  /r	Runs >recursively through the given folder list
  /i	Process recursively, but every level >individually
  /j	Also follow >junctions (=reparse points) in file system

  /m	Also process s>mall files <1024 bytes, they are skipped by default
  /h	Process >hidden files
  /s	Process >system files
  /a	File >attributes must match for linking (expect Archive)
  /n	File >names must match for linking
  /t	>Time + Date of files must match

  /q	>Quite mode
  /v	>Verbose mode
  /d	>Debug mode
  /e	Write >error messages to standard output (not stderr)
  /o	List duplicate file result to std>out (for use without /l, /q)
  /q1	Don't show files which are hard linked (alias /NoFileName)
  /w	Sho>w statistics after processing

  /?	Shows this help screen
```

Example:
```
  DFHL /l /r /hs .
```

Use Cases
---------

*   If you have a collection of duplicate files in a disk archive
*   Archive with a lot of similar files which take away a huge amount of space

System Requirements
-------------------

*   Windows NT 4.0, 2000, XP, 2003 Server
*   Full control (as file permission) over the analyzed files
*   NTFS-File System

Change Log
----------

*   Changes from Version 1.2 to Version 2.0
    *   Changing to long path support - paths can now contain up to 32000 characters.
    *   Further optimizations, file hashes are now used for compare operations besides a byte-by-byte compare to gain speed. Nevertheless, a byte-by-byte compare is always done to ensure that file content matches.
    *   Refactored lot of code and made more object oriented stuff...
    *   Further statistics for looking inside.
*   Changes from Version 1.1 to Version 1.2
    *   Bugfix if supplied path contained a tailing backslash
    *   Program continues to run even if a folder is not accessible
    *   Set GPL as software license for program
*   Changes from Version 1.0 to Version 1.1
    *   Added Support for Windows NT 4.0, missing Hardlink API was created
    *   Removed typos
    *   A system check prior execution verifies system environment

Installation
------------

Installation is not required. The program just consists of one .EXE-file, which can be called. Just copy the files to a folder inside the PATH of your system.

Authors
-------
*   Jens Scheffler, [http://www.jensscheffler.de](http://www.jensscheffler.de/)
*   Oliver Schneider, [http://assarbad.net](http://assarbad.net/)

* * *

## [Version 2.5 - Okt 2012](http://hanss.bplaced.net/Tools%20und%20Links.html)
#### by Hans Schmidts
Kommandozeilenprogramm, um Duplikate von Dateien durch NTFS-Hardlinks zu ersetzen.  

Neben der normalen Funktionalität ist DFHL auch geeignet, um platzsparende Vollbackups zu machen:

*   Z.B. per FastCopy neue vollständige Kopie machen (mit "Verify").  
    Optionen z.B.:  /cmd=force\_copy /force\_close /bufsize=128 /log /logfile="E:\\Backup\\Projects\_FastCopy.log" /speed=full /stream /reparse /verify /linkdest "D:\\Projects" /to="E:\\Backup\\Projects\_%ISODate%"  
    
*   Per DFHL das neue Backup mit dem letzten verlinken, so dass nur noch die neuen und geänderten Dateien wirklich Platz brauchen.  
    Optionen z.B.: /a /e /h /i /n /l /q1 /s /t /w
*   Anm.: Das ist zwar viel langsamer als per "rsync" oder Hermann Schinagls "ln", funktioniert aber auch mit überlangen Namen, extrem vielen Dateien, berücksichtigt auch Dateien mit geändertem Inhalt aber gleicher Zeit und Größe, und FastCopy (bzw. FastCopy64) kann vor allem mit "Verify", mit "AltStreams" und auch mit "ACL" kopieren ("AltStreams" und "ACL" werden allerdings von DFHL nicht berücksichtigt).  
    

### History

Note:
\- DFHL version number must be set in:
  \- "Resource File"
  \- DFHL.cpp in PROGRAM_VERSION
  \- CVS label
  \- Doxyfile\_html\_all_graphs
  (program date is not shown)


Todo:
\- Output of some number of files or bytes with thousands separators and with
  k,M,G etc. (not simply %I64i and %0.4g).
\- Option to skip content comparison if size, hash, name, attributes and
  time match?



2012-10-22  BDK/HS
\- Output of some number of files or bytes additionally with %0.4g
\- Option -n for "File names must match for linking"
\- Option -q1 or -NoFileName
\- prog->deleteDuplicatesList() in level loop (to get correct
  "savings of ... bytes possible" with /i)

2012-10-18  BDK/HS
\- fflush(stdout); fflush(stderr); added in DFHL.cpp

2012-10-16  BDK/HS
\- Various counters changed from int to INT64 (and names with i64 prefix etc.)
  (because of overflow: Number of files compared: -1852943428
  and all number of files in preparation for a 64 bit version)
\- Add size (to saved byes) for successful hardlinks only
\- Init of Statistics::getInstance()->i64BytesSaved
\- Reinit DuplicateFileHardLinker.files (SortedFileCollection) in
  findDuplicates() (to get correct info display with /i)
\- More statistic values (i64HSFilesFound etc.) and some messages changed

2012-10-15  BSK/HS
\- Try to rename backup back to original name on error
\- Option "/e" = "Write error messages to standard output" added
\- Make sure that files or dir names are printed for errors (see logErrorInfo())
\- Extension for temp. backup changed to ".DfhlBackup" (instead of ".backup")

2012-10-12  BSK/HS
\- File attribute comparison corrected
\- Ignore FILE\_ATTRIBUTE\_ARCHIVE in attribute comparison
\- fileAttributeMismatch and fileMTimeMismatch instead of fileMetaDataMismatch

2012-10-12  BSK/HS
\- GPF from /o corrected (no delete de; in listDuplicates())
\- Don't supress statistic output with /q
\- Don't abort on link error like 1142 (to many links)
\- Show "Number of bytes saved" in statistics
\- Option "/n" = "Don't show files which are hard linked" added
\- Print "Parsing Directory Tree Level %i" if /i

2012-02-24  BSK/HS
\- /i = "Process recursively, but every level individually" added

2012-02-23  BSK/HS
\- Version number changed, tabs expanded, trailing spaces removed
\- Use crc32 and 16KiB instead of xor64 and 64KiB
\- Set file1Name, file2Name before use in logVerbose
\- Reuse first 2 blocks of file 1 if possible
\- Return 1 as exit code for error (not -1)

2012-02-06  BSK/HS
\- Check if both files are already linked before reading any data
\- logError ...trying to change attribute... removed
\- generateHash() corrected and optimized
\- DfhlTest.bat added

2012-01-23  BSK/HS
\- "beta-2-1-DFHL-HS"
\- Adapted to BSK dir structure, file names corrected, no functional change
\- "release-2-0-DFHL-Vendor"
\- Checkin of original "dfhl-source_2.0.zip" from https://www.jensscheffler.de/dfhl