#ifndef FILEMOUNT_H
#define FILEMOUNT_H

#include "globals.h"
#include "seamless_smb_lib_export.h"

#include "dokan/dokan.h"

#include <string>
#include <memory>
#include <mutex>


class Context;

class SEAMLESS_SMB_LIB_EXPORT FileMount
{
public:
    FileMount(std::shared_ptr<Globals> globals,bool debug,bool usestderr ,DOKAN_OPTIONS dokanOptions);
    int mount();
    void unmount();
    bool isRunning();
    void join();

    std::wstring getRootDir();
    std::wstring getMountPoint();
    std::wstring getVolumeName();

private:
    DOKAN_OPERATIONS dokanOperations;
    DOKAN_OPTIONS m_dokanOptions;
    std::shared_ptr<Context> m_context;
    DOKAN_HANDLE handle=NULL;
    bool mounted=false;
    std::mutex mutex;
};

#endif // FILEMOUNT_H
