#include "protocol.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "A fournir: " << argv[0] << " <config_file.json>\n";
        return 1;
    }

    std::string config_file = argv[1];
    Protocol proto;
    if (!proto.load_config(config_file)) {
        std::cerr << "Erreur chargement de la configuration." << std::endl;
        return 1;
    }

    proto.start();
    return 0;
}