#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

void session(tcp::socket socket) {
    try {
        // Message de bienvenue envoy� au client
        std::string message = "Hello from server!\n";
        boost::asio::write(socket, boost::asio::buffer(message));
    }
    catch (const std::exception& e) {
        std::cerr << "Erreur dans la session : " << e.what() << std::endl;
    }
}

int main() {
    try {
        // Service io pour g�rer les op�rations asynchrones
        boost::asio::io_context io_context;

        // Cr�er un endpoint pour �couter sur le port 12345
        tcp::endpoint endpoint(tcp::v4(), 12345);

        // Cr�er un acceptor pour accepter les connexions
        tcp::acceptor acceptor(io_context, endpoint);

        std::cout << "Le serveur �coute sur le port 12345...\n";

        while (true) {
            // Attendre une connexion client
            tcp::socket socket(io_context);
            acceptor.accept(socket);

            std::cout << "Client connect� !\n";

            // G�rer la session avec le client
            session(std::move(socket));
        }

    }
    catch (const std::exception& e) {
        std::cerr << "Erreur dans le serveur : " << e.what() << std::endl;
    }

    return 0;
}
