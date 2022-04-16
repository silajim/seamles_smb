#include "ramdiskmanager.h"
#include <boost-1_77/boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <fstream>
#include <chrono>
#include <time.h>
#include <ctime>

using std::cout; using std::endl;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;
RamDiskManager::RamDiskManager(std::string ramDrive)
{
    this->ramDrive = ramDrive;
    t = new boost::thread(boost::bind(&RamDiskManager::checkDirty,this));
}

std::string RamDiskManager::createFile()
{
    auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    std::string name = ramDrive;
    name += +"\\" + std::to_string(millisec_since_epoch) + ".temp";
    std::ofstream(name.c_str());
    opens.push_back(name);
    return name;
}

std::string RamDiskManager::createdirectory()
{

    auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    std::string name = ramDrive;
    name += +"\\" + std::to_string(millisec_since_epoch);
    boost::filesystem::create_directory(name);
    opens.push_back(name);
    return name;
}

void RamDiskManager::deleteFile(std::string file)
{
    mutex.lock();
    opens.remove(file);
    dirty = !boost::filesystem::remove(file);
    mutex.unlock();
}

void RamDiskManager::deletedirectory(std::string dir)
{
    deleteFile(dir);
//    mutex.lock();
//    opens.remove(dir);
//    dirty = !boost::filesystem::remove(dir);
//    mutex.unlock();
}

void RamDiskManager::checkDirty(){
     std::string format = "format ";
     format +=  ramDrive.substr(0, ramDrive.size()-1)  + " /FS:NTFS /X /Q /y";

    try{
        while (true){
            boost::this_thread::sleep_for(boost::chrono::seconds{15});
            mutex.lock();
            if(dirty && opens.empty()){
                system(format.c_str());
                dirty = false;
            }
            mutex.unlock();
        }
    } catch (boost::thread_interrupted&) {}
}
