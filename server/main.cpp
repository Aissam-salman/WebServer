#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <stdexcept>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>


#include "utils.hpp"
#include "Socket.hpp"
#include "Server.hpp"
// #include "Parser.hpp"
#include "Lexer.hpp"


using namespace std;

bool g_running = true;

void signal_sigint(int) {
	g_running = false;
	cerr << "SIGNAL RECEIVED" << endl;
}


// int main (void) {
int main(int argc, char **argv) {
	try {
		if (argc != 2)
			throw std::runtime_error ("Wrong number of argument ! Try with : ./webserv [config file path]\n");
		// CREATING A SERVER LISTENING ON ONLY ONE PORT
		// SETTING UP UTILS
			
		// INIT SERVER
		Server server("SERVER TEST");

		// FIRST STAGE LEXER
		Lexer lexer(argv[1]);
		lexer.initRawVector();
		printTokens(lexer.getTokenVector());

		Location test_location("root");
		test_location.setMethods(GET);
		test_location.setMethods(PUT);

		Location test_location2("test");
		test_location2.setMethods(GET);
		test_location2.setMethods(PUT);

		server.getLocations().push_back(test_location);
		server.getLocations().push_back(test_location2);
		// SECOND STAGE PARSER

		Socket socket1("SocketTest");
		server.getSockets().push_back(socket1);

		server.printServer();

    serv.run();
  }
	catch (runtime_error &e) {
		std::cerr << RED << e.what() << endofline;
	} catch (exception &e) {
		std::cerr << YELLOW << e.what() << endofline;
	} catch (...) {
		cerr << "FELL INTO THE PIT" << endl;
	}
	return 0;
}
