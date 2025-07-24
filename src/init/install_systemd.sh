#!/bin/bash
# Installation script for XMLRCS systemd service files

set -e

# Check if running as root
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

# Determine script location
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SYSTEMD_DIR="/etc/systemd/system"

# Create xmlrcs user if it doesn't exist
if ! id -u xmlrcs &>/dev/null; then
    echo "Creating xmlrcs user..."
    useradd -r -s /bin/false -m -d /opt/xmlrcs xmlrcs
    # Set appropriate permissions
    mkdir -p /opt/xmlrcs
    chown -R xmlrcs:xmlrcs /opt/xmlrcs
fi

# Copy service files
echo "Installing systemd service files..."
cp "$SCRIPT_DIR/es2r.service" "$SYSTEMD_DIR/"
cp "$SCRIPT_DIR/xmlrcsd.service" "$SYSTEMD_DIR/"

# Reload systemd
echo "Reloading systemd daemon..."
systemctl daemon-reload

echo "Services installed. You can now start them with:"
echo "  systemctl start es2r.service"
echo "  systemctl start xmlrcsd.service"
echo
echo "To enable them at boot:"
echo "  systemctl enable es2r.service"
echo "  systemctl enable xmlrcsd.service"
echo
echo "For status information:"
echo "  systemctl status es2r.service"
echo "  systemctl status xmlrcsd.service"
