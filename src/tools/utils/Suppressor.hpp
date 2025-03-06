#pragma once

#include <iostream>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

namespace tools::utils {

    class Suppressor {
    private:
        FILE* s = nullptr;
        int s_backup;
        int dev_null;
    public:
        Suppressor(FILE *s): s(s) {        
            s_backup = dup(fileno(s));
            dev_null = open("/dev/null", O_WRONLY);
            dup2(dev_null, fileno(s));
        }
        ~Suppressor() {
            dup2(s_backup, fileno(s));
            close(s_backup);
            close(dev_null);
        }
    };

}