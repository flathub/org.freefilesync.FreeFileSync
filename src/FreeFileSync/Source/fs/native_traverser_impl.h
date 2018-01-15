// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: https://www.gnu.org/licenses/gpl-3.0          *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#include <zen/sys_error.h>
#include <zen/symlink_target.h>
#include <zen/file_access.h>


    #include <cstddef> //offsetof
    #include <sys/stat.h>
    #include <dirent.h>


//implementation header for native.cpp, not for reuse!!!

namespace
{
using namespace zen;
using AFS = AbstractFileSystem;


inline
AFS::FileId convertToAbstractFileId(const zen::FileId& fid)
{
    if (fid == zen::FileId())
        return AFS::FileId();

    AFS::FileId out(reinterpret_cast<const char*>(&fid.volumeId),  sizeof(fid.volumeId));
    out.     append(reinterpret_cast<const char*>(&fid.fileIndex), sizeof(fid.fileIndex));
    return out;
}


class DirTraverser
{
public:
    static void execute(const Zstring& baseDirPath, AFS::TraverserCallback& sink)
    {
        DirTraverser(baseDirPath, sink); //throw X
    }

private:
    DirTraverser(const Zstring& baseDirPath, AFS::TraverserCallback& sink)
    {
        /* quote: "Since POSIX.1 does not specify the size of the d_name field, and other nonstandard fields may precede
                   that field within the dirent structure, portable applications that use readdir_r() should allocate
                   the buffer whose address is passed in entry as follows:
                       len = offsetof(struct dirent, d_name) + pathconf(dirPath, _PC_NAME_MAX) + 1
                       entryp = malloc(len); */
        const size_t nameMax = std::max<long>(::pathconf(baseDirPath.c_str(), _PC_NAME_MAX), 10000); //::pathconf may return long(-1)
        buffer_.resize(offsetof(struct ::dirent, d_name) + nameMax + 1);

        traverse(baseDirPath, sink); //throw X
    }

    DirTraverser           (const DirTraverser&) = delete;
    DirTraverser& operator=(const DirTraverser&) = delete;

    void traverse(const Zstring& dirPath, AFS::TraverserCallback& sink) //throw X
    {
        tryReportingDirError([&] //throw X
        {
            traverseWithException(dirPath, sink); //throw FileError, X
        }, sink);
    }

    void traverseWithException(const Zstring& dirPath, AFS::TraverserCallback& sink) //throw FileError, X
    {
        //no need to check for endless recursion:
        //1. Linux has a fixed limit on the number of symbolic links in a path
        //2. fails with "too many open files" or "path too long" before reaching stack overflow

        DIR* folder = ::opendir(dirPath.c_str()); //directory must NOT end with path separator, except "/"
        if (!folder)
            THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot open directory %x."), L"%x", fmtPath(dirPath)), L"opendir");
        ZEN_ON_SCOPE_EXIT(::closedir(folder)); //never close nullptr handles! -> crash

        for (;;)
        {
            struct ::dirent* dirEntry = nullptr;
            if (::readdir_r(folder, reinterpret_cast< ::dirent*>(&buffer_[0]), &dirEntry) != 0)
                THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot read directory %x."), L"%x", fmtPath(dirPath)), L"readdir_r");
            //don't retry but restart dir traversal on error! https://blogs.msdn.microsoft.com/oldnewthing/20140612-00/?p=753/

            if (!dirEntry) //no more items
                return;

            const char* itemNameRaw = dirEntry->d_name; //evaluate dirEntry *before* going into recursion => we use a single "buffer"!

            //skip "." and ".."
            if (itemNameRaw[0] == '.' &&
                (itemNameRaw[1] == 0 || (itemNameRaw[1] == '.' && itemNameRaw[2] == 0)))
                continue;
            const Zstring& itemName = itemNameRaw;
            if (itemName.empty()) //checks result of osx::normalizeUtfForPosix, too!
                throw FileError(replaceCpy(_("Cannot read directory %x."), L"%x", fmtPath(dirPath)), L"readdir_r: Data corruption; item with empty name.");

            const Zstring& itemPath = appendSeparator(dirPath) + itemName;

            struct ::stat statData = {};
            if (!tryReportingItemError([&] //throw X
        {
            if (::lstat(itemPath.c_str(), &statData) != 0) //lstat() does not resolve symlinks
                    THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtPath(itemPath)), L"lstat");
            }, sink, itemName))
            continue; //ignore error: skip file

            if (S_ISLNK(statData.st_mode)) //on Linux there is no distinction between file and directory symlinks!
            {
                const AFS::TraverserCallback::SymlinkInfo linkInfo = { itemName, statData.st_mtime };

                switch (sink.onSymlink(linkInfo)) //throw X
                {
                    case AFS::TraverserCallback::LINK_FOLLOW:
                    {
                        //try to resolve symlink (and report error on failure!!!)
                        struct ::stat statDataTrg = {};

                        const bool validLink = tryReportingItemError([&] //throw X
                        {
                            if (::stat(itemPath.c_str(), &statDataTrg) != 0)
                                THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot resolve symbolic link %x."), L"%x", fmtPath(itemPath)), L"stat");
                        }, sink, itemName);

                        if (validLink)
                        {
                            if (S_ISDIR(statDataTrg.st_mode)) //a directory
                            {
                                if (std::unique_ptr<AFS::TraverserCallback> trav = sink.onFolder({ itemName, &linkInfo })) //throw X
                                    traverse(itemPath, *trav); //throw X
                            }
                            else //a file or named pipe, ect.
                            {
                                AFS::TraverserCallback::FileInfo fi = { itemName, makeUnsigned(statDataTrg.st_size), statDataTrg.st_mtime, convertToAbstractFileId(extractFileId(statDataTrg)), &linkInfo };
                                sink.onFile(fi); //throw X
                            }
                        }
                        // else //broken symlink -> ignore: it's client's responsibility to handle error!
                    }
                    break;

                    case AFS::TraverserCallback::LINK_SKIP:
                        break;
                }
            }
            else if (S_ISDIR(statData.st_mode)) //a directory
            {
                if (std::unique_ptr<AFS::TraverserCallback> trav = sink.onFolder({ itemName, nullptr })) //throw X
                    traverse(itemPath, *trav); //throw X
            }
            else //a file or named pipe, ect.
            {
                AFS::TraverserCallback::FileInfo fi = { itemName, makeUnsigned(statData.st_size), statData.st_mtime, convertToAbstractFileId(extractFileId(statData)), nullptr /*symlinkInfo*/ };
                sink.onFile(fi); //throw X
            }
            /*
            It may be a good idea to not check "S_ISREG(statData.st_mode)" explicitly and to not issue an error message on other types to support these scenarios:
            - RTS setup watch (essentially wants to read directories only)
            - removeDirectory (wants to delete everything; pipes can be deleted just like files via "unlink")

            However an "open" on a pipe will block (https://sourceforge.net/p/freefilesync/bugs/221/), so the copy routines need to be smarter!!
            */
        }
    }

    std::vector<char> buffer_;
};
}
