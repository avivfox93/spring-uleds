#pragma once
#include <string>
#include <map>
#include <gpiod.hpp>
#include <nlohmann/json.hpp>
#include "common.hpp"

struct LEDDevice {
    std::string name;
    LEDType type;
    int max_brightness = 255;

    int uled_fd = -1;
    int event_fd = -1; // brightness fd
    int brightness = 0;

    std::string gpiochip;
    int gpioline = -1;
    
    gpiod::chip chip;
    gpiod::line line;

    std::string pwmchip;
    int channel = -1;

    void set_brightness(int val);
    const char *to_string();
};

class LEDManager {
public:
    bool is_uled_fd(int fd) const;
    void create_led(const nlohmann::json &cfg, int epoll_fd);
    void destroy_led(const std::string &name);
    void handle_event(int fd);

private:
    std::map<std::string, LEDDevice> leds_by_name;
    std::map<int, std::string> fd_to_name;
};
