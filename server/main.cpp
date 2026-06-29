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
#include "utils.hpp"
#include "Server.hpp"
#include "Parser.hpp"
// #include "Socket.hpp"

using namespace std;


// int main (void) {
int main(int argc, char **argv) {
	try {
		if (argc != 2)
			throw std::runtime_error ("Wrong number of argument ! Try with : ./webserv [config file path]\n");
		// CREATING A SERVER LISTENING ON ONLY ONE PORT
		// SETTING UP UTILS
			

		// FIRST STAGE LEXER
		Lexer lexer(argv[1]);
		lexer.initRawVector();


		// INIT SERVER
		std::vector<Server> servers_vector;

		Parser parser(lexer.getTokenVector(), servers_vector);
		parser.initServers();

		for (size_t i = 0; i < servers_vector.size(); i++) {
			servers_vector[i].printServer();
		}

		// for (size_t i = 0; i < servers_vector.size(); i++) {
		// 	servers_vector[i].run();
		// }
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
