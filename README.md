
# 📘 README.md — Spring Userspace LED Daemon
Spring Userspace LED Daemon

spring_uledd + spring_uledctl
Userspace-created virtual LEDs using uleds kernel module

## 📖 Overview

spring_uledd and spring_uledctl work together to create and manage virtual LED devices using the Linux kernel's uleds subsystem.
This allows creation of LEDs entirely in userspace without Device Tree, sysfs hacks, or kernel patches.

✔ Create LEDs dynamically

✔ Destroy LEDs at runtime

✔ Control brightness from any application

✔ Support LED definitions via command-line arguments or YAML files

✔ Single background daemon using epoll() to monitor all LED brightness changes

✔ Communication through a UNIX domain socket


## 🧱 Architecture
```
+---------------------+       UNIX socket        +----------------------+
|  spring_uledctl     | <----------------------> |     spring_uledd     |
|  (client CLI/YAML)  |                          |   (LED management)   |
+---------------------+                          +----------------------+
          |                                                       |
          |  LED definitions                                      |
          +------------------> [uleds kernel module] <-------------+
```

## 🚀 Components
### spring_uledd (daemon)

- Listens on /run/spring_uledd.sock

- Accepts LED create/destroy commands

- Creates /sys/class/leds/<name> using /dev/uleds

- Uses epoll() to monitor each LED's brightness file


### spring_uledctl (client)

- Sends commands to daemon

- Supports:

  - create LED from CLI (TODO)

  - create LEDs from YAML file

  - destroy LED


YAML example:
``` yaml
leds:
  - name: status
    type: gpio
    gpio: 27
    max_brightness: 255

  - name: pwm_bl
    type: pwm
    pwmchip: 0
    channel: 3
    max_brightness: 4095
```

## 🔧 Build & Install
1. Clone repository:
    ``` bash
    git clone https://github.com/yourname/spring-uledd.git
    cd spring-uledd
    ```

2. Build:
    ``` bash
    mkdir build && cd build
    cmake ..
    make
    ```

3. Install:
    ``` bash
    sudo make install
    ```
4. Create systemd service and ```spring_uled```:
   ``` bash
   sudo ./post_install.sh
   ```


    Installs:

    /usr/bin/spring_uledd

    /usr/bin/spring_uledctl

## ▶️ Running

Start the daemon:
``` bash
sudo spring_uledd
```

Add user to ```spring_uled``` group (optional) for rootless usage
``` bash
sudo usermod -aG spring_uled $USER
# logout and then login
```

Create from YAML:
``` bash
spring_uledctl load leds.yaml
```

Destroy LED:
``` bash
spring_uledctl destroy led1
```

## License
### GPL-3
