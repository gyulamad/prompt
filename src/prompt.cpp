#include "tools/utils/Test.hpp"

#include "tools/utils/ERROR.hpp"

// #include "tools/utils/utils.hpp"
// #include "tools/cmd/cmd.hpp"
// #include "tools/voice/voice.hpp"
#include "tools/agency/agency.hpp"

using namespace std;
// using namespace tools::utils;
// using namespace tools::cmd;
using namespace tools::agency;
using namespace tools::agency::agents;
// using namespace tools::voice;



// #include "../libs/K-Adam/SafeQueue/SafeQueue.hpp"


int safe_main(int , char *[]) {
    run_tests({
        
    });

    try {

        PackQueue<string> queue;
        Agency<string> agency(queue);
        agency.spawn<EchoAgent<string>>().async();
        agency.spawn<UserAgent<string>>().async();
        agency.sync();


    } catch (exception &e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    return safe_main(argc, argv);
}