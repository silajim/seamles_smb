#include "seamless_smb_service.h"
#include "daemon.h"
int main(int argc, char *argv[])
{

    seamless_smb_service src(argc,argv);
    src.exec();
    return 0;
}
