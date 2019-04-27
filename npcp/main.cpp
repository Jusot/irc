#include <unistd.h>

// #define _DEBUG

int main(int argc, char *argv[])
{
#ifndef _DEBUG
    daemon(0, 0);
#endif

    return 0;
}