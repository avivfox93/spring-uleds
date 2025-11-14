#!/bin/bash
# This script is run after the package is installed
# Please run it with root privileges

# Create new group
echo "Creating new group spring_uled"
groupadd --system spring_uled

# Copy service file and enable it
echo "Enabling spring_uledd.service"
cp ./examples/spring_uledd.service /etc/systemd/system/
systemctl daemon-reload
systemctl enable spring_uledd.service
systemctl start spring_uledd.service
