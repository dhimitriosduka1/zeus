#include <lightwave/core.hpp>
#include <lightwave/registry.hpp>
#include <lightwave/logger.hpp>

#include "parser.hpp"

#include <fstream>

#ifdef LW_OS_WINDOWS
#include <cstdlib>
#endif

using namespace lightwave;

void print_exception(const std::exception &e, int level = 0) {
    logger(EError, "%s%s", std::string(2 * level, ' '), e.what());
    try {
        std::rethrow_if_nested(e);
    } catch(const std::exception& nestedException) {
        print_exception(nestedException, level + 1);
    } catch(...) {}
}

int main(int argc, const char *argv[]) {
#ifdef LW_DEBUG
    logger(EWarn, "lightwave was compiled in Debug mode, expect rendering to be much slower");
#endif
#ifdef LW_CC_MSC
    logger(EWarn, "lightwave was compiled using MSVC, expect rendering to be slower (we recommend using clang instead)");
#endif

#ifdef LW_OS_WINDOWS
    // The error dialog might interfere with the run_tests.py script on Windows, so disable it. 
   _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif

    try {
        if (argc <= 1) {
            logger(EError, "please specify path to scene");
            return -1;
        }
        
        std::filesystem::path scenePath = argv[1];

        SceneParser parser { scenePath };
        for (auto &object : parser.objects()) {
            if (auto executable = dynamic_cast<Executable *>(object.get())) {
                executable->execute();
            }
        }
    } catch(const std::exception &e) {
        print_exception(e);
        return 1;
    }

    return 0;
}
