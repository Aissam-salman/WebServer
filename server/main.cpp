#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <stdexcept>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Server.hpp"

using namespace std;

void error(string error) { cerr << error << endl; }

void display(string print) { cout << print << endl; }

int main(void) {
  try {
    Server serv("WebServTest");
    serv.run();
  } catch (runtime_error &e) {
    error("RUNTIME ERROR");
  } catch (exception &e) {
    error("EXCEPTION");
  } catch (...) {
    cerr << "FELL INTO THE PIT" << endl;
  }
  return 0;
}
