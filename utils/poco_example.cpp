#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/SocketStream.h>
#include <iostream>

using namespace Poco::Net;
using namespace std;

int main() {
    try {
        // Create a server socket on port 8080
        ServerSocket serverSocket(8080);
        cout << "Server is running on port 8080..." << endl;

        // Wait for a client to connect
        StreamSocket clientSocket = serverSocket.acceptConnection();
        cout << "Client connected!" << endl;

        // Create a socket stream for communication
        SocketStream socketStream(clientSocket);

        // Send a welcome message
        socketStream << "Hello from Poco server!" << endl;

        // Read data from the client
        string message;
        getline(socketStream, message);
        cout << "Received from client: " << message << endl;

    }
    catch (Poco::Exception& ex) {
        cerr << "Server error: " << ex.displayText() << endl;
    }

    return 0;
}
