# XMLRCS Systemd Services

This directory contains systemd service files for the XMLRCS components:

1. `es2r.service` - EventStream to Redis converter service
2. `xmlrcsd.service` - XMLRCS TCP daemon service

## Installation

You can install the systemd service files using the provided installation script:

```bash
sudo ./install_systemd.sh
```

Or manually:

```bash
sudo cp es2r.service /etc/systemd/system/
sudo cp xmlrcsd.service /etc/systemd/system/
sudo systemctl daemon-reload
```

## Usage

### Starting the services:

```bash
sudo systemctl start es2r
sudo systemctl start xmlrcsd
```

### Enabling the services to start at boot:

```bash
sudo systemctl enable es2r
sudo systemctl enable xmlrcsd
```

### Checking service status:

```bash
sudo systemctl status es2r
sudo systemctl status xmlrcsd
```

### Viewing logs:

```bash
sudo journalctl -u es2r -f
sudo journalctl -u xmlrcsd -f
```

## Configuration

The service files assume:

1. Your XMLRCS installation is in `/opt/xmlrcs`
2. Services run as the `xmlrcs` user
3. Redis is installed and running on the local machine

You may need to adjust paths and settings in the service files based on your specific installation.

## Dependencies

Both services depend on:

- A running Redis server (redis-server.service)
- Network connectivity

The xmlrcsd service is configured to start after the es2r service.
