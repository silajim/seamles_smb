#ifndef RAMDISKMANAGER_H
#define RAMDISKMANAGER_H

#include <iostream>
#include <boost/thread.hpp>
#include <mutex>
#include <boost/container/list.hpp>

class RamDiskManager
{
public:
    RamDiskManager(std::string ramDrive);
    std::string createFile();
    std::string createdirectory();
    void deleteFile(std::string file);
    void deletedirectory(std::string dir);

private:
    void checkDirty();
    std::string ramDrive;
    bool dirty=false;
    boost::thread *t = nullptr;
    std::mutex mutex;
    boost::container::list<std::string> opens;
};

#endif // RAMDISKMANAGER_H
