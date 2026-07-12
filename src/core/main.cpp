#include <exception>
#include <iostream>
#include <stdexcept>

#include "WebServ.hpp"
#include "errors.hpp"
#include "utils.hpp"

using namespace std;

int main(int argc, char **argv) {
  try {
    if (argc != 2)
      throw std::runtime_error(ERRS_ARGS_MAIN);

    WebServ webserv(argv[1]);

#if DEBUG == 1
    webserv.printConfig();
#endif

#if RUN == 1
    webserv.run();
#endif
  } catch (runtime_error &e) {
    std::cerr << RED << e.what() << endofline;
  } catch (exception &e) {
    std::cerr << YELLOW << e.what() << endofline;
  } catch (...) {
    cerr << "FELL INTO THE PIT" << endl;
  }
  return 0;
}
