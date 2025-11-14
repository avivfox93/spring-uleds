#include "common.hpp"
#include "led_manager.hpp"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <grp.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

constexpr int MAX_EVENTS = 32;

static int create_unix_server()
{
    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        if(connect(fd, (sockaddr*)&addr, sizeof(addr)) == 0) {
            close(fd);
            perror(SOCKET_PATH);
            return -1;
        } else {
            unlink(SOCKET_PATH);
            if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
                perror("bind");
                close(fd);
                return -1;
            }
        }
    }

    if (listen(fd, 16) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }
    struct group *grp;
    grp = getgrnam("spring_uled");
    if (grp != NULL) {
        fchown(fd, -1, grp->gr_gid);
    }
    chmod(SOCKET_PATH, 0660);
    return fd;
}

int main()
{
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1");
        return 1;
    }

    int sock_fd = create_unix_server();
    if (sock_fd < 0)
        return 1;

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = sock_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &ev);

    LEDManager leds;

    std::cout << "[uledd] Listening on " << SOCKET_PATH << std::endl;

    epoll_event events[MAX_EVENTS];

    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;

            // --- New client connection ---
            if (fd == sock_fd) {
                int client = accept4(sock_fd, nullptr, nullptr, SOCK_NONBLOCK);
                if (client >= 0) {
                    epoll_event cev{};
                    cev.events = EPOLLIN;
                    cev.data.fd = client;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client, &cev);
                }
                continue;
            }

            // --- Client command available ---
            if (fd != sock_fd && leds.is_uled_fd(fd) == false) {
                char buf[4096];
                ssize_t len = read(fd, buf, sizeof(buf));
                if (len <= 0) {
                    close(fd);
                    continue;
                }

                std::string msg(buf, len);
                try {
                    json j = json::parse(msg);
                    std::string cmd = j.value("cmd", "");

                    if (cmd == "create") {
                        leds.create_led(j, epoll_fd);
                    } else if (cmd == "destroy") {
                        leds.destroy_led(j.value("name", ""));
                    } else {
                        std::cerr << "[uledd] Unknown command: " << cmd << "\n";
                    }
                } catch (const std::exception &e) {
                    std::cerr << "[uledd] JSON parse error: " << e.what() << "\n";
                }
            }
            // --- LED brightness change event ---
            else if (leds.is_uled_fd(fd)) {
                leds.handle_event(fd);
            }
        }
    }

    close(sock_fd);
    close(epoll_fd);
    return 0;
}
