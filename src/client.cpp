#include "common.hpp"
#include <yaml-cpp/yaml.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static bool send_json(const json &msg) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return false; }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    if (connect(sock, (sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return false;
    }

    std::string s = msg.dump();
    write(sock, s.c_str(), s.size());
    close(sock);
    return true;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage:\n"
                  << "  uledctl create <name> <type> [args...]\n"
                  << "  uledctl destroy <name>\n"
                  << "  uledctl load <file.yaml>\n";
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "destroy" && argc == 3) {
        json j = {{"cmd", "destroy"}, {"name", argv[2]}};
        return send_json(j) ? 0 : 1;
    }
    // else if (cmd == "create" && argc >= 4) {
    //     std::string name = argv[2];
    //     std::string type = argv[3];
    //     json j = {{"cmd", "create"}, {"name", name}, {"type", type}};

    //     for (int i = 4; i + 1 < argc; i += 2)
    //         j[argv[i]] = argv[i + 1];

    //     return send_json(j) ? 0 : 1;
    // }
    else if (cmd == "load" && argc == 3) {
        YAML::Node root = YAML::LoadFile(argv[2]);
        for (auto node : root["leds"]) {
            json j;
            j["cmd"] = "create";
            for (auto it : node){
                try {
                    j[it.first.as<std::string>()] = it.second.as<int>();
                    continue;
                } catch (...) {
                }
                try {
                    j[it.first.as<std::string>()] = it.second.as<bool>();
                    continue;
                } catch (...) {
                }
                try {
                    j[it.first.as<std::string>()] = it.second.as<double>();
                    continue;
                } catch (...) {
                }
                j[it.first.as<std::string>()] = it.second.as<std::string>();
            }
            send_json(j);
        }
        return 0;
    }

    std::cerr << "Invalid command\n";
    return 1;
}
