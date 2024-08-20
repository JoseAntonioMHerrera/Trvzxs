#include <iostream>

extern "C" {

#include "engineCInterface.h"

}


int main() {
    std::cout << "LLAMAMOS DESDE C++" << '\n';
#ifdef NDEBUG
std::cout << "Assertions are disabled" << std::endl;
#else
std::cout << "Assertions are enabled" << std::endl;
#endif


   try {
        engineEntryPoint();
    } catch (const std::exception& e) {
        std::cerr << "Caught exception: " << e.what() << std::endl;
        throw;  // Re-throw the exception
    } catch (...) {
        std::cerr << "Caught unknown exception" << std::endl;
        throw;  // Re-throw the exception
    }
    return 0;
}
