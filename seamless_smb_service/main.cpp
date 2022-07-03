#include "seamless_smb_service.h"
int main(int argc, char *argv[])
{

    seamless_smb_service src(argc,argv);
    src.exec();
    return 0;
}
