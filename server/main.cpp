#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <stdexcept>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "Lexer.hpp"
#include "Parser.hpp"
#include "Server.hpp"
#include "errors.hpp"
#include "utils.hpp"
// #include "Socket.hpp"

// TODO : Add fcntl to addNewClient to set it to non blocking

using namespace std;

// int main (void) {
int main(int argc, char **argv) {
  try {
    if (argc != 2)
      throw std::runtime_error(ERRS_ARGS_MAIN);

    // FIRST STAGE LEXER
    Lexer lexer(argv[1]);
    lexer.initRawVector();

    // INIT SERVER
    std::vector<Server> servers_vector;

    // SECOND STAGE PARSER
    Parser parser(lexer.getTokenVector(), servers_vector);
    parser.initServers();

    std::vector<Listener> listeners = gatherListeners(servers_vector);

#if DEBUG == 1
    for (size_t i = 0; i < servers_vector.size(); i++) {
      servers_vector[i].printServer();
    }
    printListeners(listeners);
#endif

#if RUN == 1
    servers_vector[0].run(listeners);
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
