#include "led_manager.hpp"
#include "common.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <linux/uleds.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <fstream>

bool LEDManager::is_uled_fd(int fd) const {
    return fd_to_name.find(fd) != fd_to_name.end();
}

void LEDManager::create_led(const nlohmann::json &cfg, int epoll_fd)
{
    LEDDevice led;
    led.name = cfg.value("name", "");
    std::string str_type = cfg.value("type", "gpio");
    if (str_type == "gpio") led.type = LEDType::GPIO;
    else if (str_type == "pwm") led.type = LEDType::PWM;
    else led.type = LEDType::UNKNOWN;
    led.max_brightness = cfg.value("max_brightness", 255);

    if (cfg.contains("gpiochip")) led.gpiochip = cfg["gpiochip"];
    if (cfg.contains("line")) led.gpioline = cfg["line"];
    if (cfg.contains("pwmchip")) led.pwmchip = cfg["pwmchip"];
    if (cfg.contains("channel")) led.channel = cfg["channel"];

    int fd = open("/dev/uleds", O_RDWR);
    if (fd < 0) {
        perror("open /dev/uleds");
        return;
    }

    struct uleds_user_dev uled {};
    strncpy(uled.name, led.name.c_str(), sizeof(uled.name) - 1);
    uled.max_brightness = led.max_brightness;

    if (write(fd, &uled, sizeof(uled)) != sizeof(uled)) {
        perror("write uleds_user_dev");
        close(fd);
        return;
    }

    if(led.type == LEDType::GPIO) {
        led.chip = gpiod::chip(led.gpiochip);
        led.line = led.chip.get_line(led.gpioline);
        led.line.request({"spring_uled", gpiod::line_request::DIRECTION_OUTPUT, 0});
    }

    led.uled_fd = fd;
    fd_to_name[fd] = led.name;
    leds_by_name[led.name] = led;

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0)
        perror("epoll_ctl add led");

    std::cout << "[uledd] Created LED: " << led.name
              << " (type=" << led.to_string() << ", max=" << led.max_brightness << ")\n";
}

void LEDManager::destroy_led(const std::string &name)
{
    auto it = leds_by_name.find(name);
    if (it == leds_by_name.end()) return;

    LEDDevice &led = it->second;
    if (led.uled_fd >= 0) close(led.uled_fd);

    fd_to_name.erase(led.uled_fd);
    leds_by_name.erase(it);
    std::cout << "[uledd] Destroyed LED: " << name << "\n";
}

void LEDManager::handle_event(int fd)
{
    auto it = fd_to_name.find(fd);
    if (it == fd_to_name.end()) return;

    LEDDevice &led = leds_by_name[it->second];
    int val = 0;
    ssize_t n = read(fd, &val, sizeof(val));
    if (n != sizeof(val)) {
        if (errno != EAGAIN) perror("read brightness");
        return;
    }

    led.brightness = val;

    led.set_brightness(val);
}

// -----------------------------------------------------------
// GPIO/PWM Writes
// -----------------------------------------------------------
static void set_gpio(LEDDevice &led, uint32_t brightness)
{
    bool level = brightness > 0;
    try {
        led.line.set_value(level);
    } catch (std::exception &e) {
        std::cerr << "GPIO write failed: " << e.what() << "\n";
    }
}

static int calculate_pwm_duty(const LEDDevice &led, uint32_t brightness, int max_brightness)
{
    int period_ns = -1;
    //read period
    std::string period_path = 
        led.pwmchip + "/pwm" + std::to_string(led.channel) + "/period";
    std::ifstream f(period_path);
    if (!f.is_open()) {
        std::cerr << "Cannot read PWM period: " << period_path << "\n";
        return -1;
    }
    f >> period_ns;
    f.close();
    if (brightness >= static_cast<uint32_t>(max_brightness))
        return period_ns;
    return ((uint64_t)brightness * (uint64_t)period_ns) / max_brightness;
}

static void set_pwm(LEDDevice &led, uint32_t brightness)
{
    std::string duty_path =
        led.pwmchip + "/pwm" + std::to_string(led.channel) + "/duty_cycle";
    std::string enable_path =
        led.pwmchip + "/pwm" + std::to_string(led.channel) + "/enable";

    std::ofstream f(enable_path);
    if (!f.is_open()) {
        std::cerr << "Cannot find PWM: " << enable_path << "\n";
        std::string export_path = led.pwmchip + "/export";
        f.open(export_path);
        if (!f.is_open()) {
            std::cerr << "Cannot export PWM: " << export_path << "\n";
            return;
        }
        f << led.channel;
        f.close();
        f.open(enable_path);
        if (!f.is_open()) {
            std::cerr << "Cannot enable PWM: " << enable_path << "\n";
            return;
        }
    }
    f << 1;
    f.close();
    int duty = calculate_pwm_duty(led, brightness, led.max_brightness);
    if (duty < 0) return;
    f.open(duty_path);
    if (!f.is_open()) {
        std::cerr << "Cannot write PWM: " << duty_path << "\n";
        return;
    }
    f << duty;
}

void LEDDevice::set_brightness(int val) {
    switch (type)
    {
    case LEDType::GPIO:
        set_gpio(*this, val);
        break;
    case LEDType::PWM:
        set_pwm(*this, val);
        break;
    default:
        break;
    }
}

const char *LEDDevice::to_string() {
    switch (type) {
        case LEDType::GPIO: return "gpio";
        case LEDType::PWM:  return "pwm";
        default:            return "unknown";
    }
}
