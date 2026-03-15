# XmlRcs Nagios Check

Simple Nagios-compatible check for XmlRcs (`huggle-rc.wmflabs.org:8822`).

## What it checks

1. TCP connection to XmlRcs.
2. Sends `stat` command.
3. Parses `io` counter from response.
4. Compares `io` with previous run from a state file.

If `io` did not increase compared to previous check, plugin returns `CRITICAL`.

## Script

- `check_xmlrcs.py`

## Usage

```bash
./check_xmlrcs.py
```

Options:

- `--host` (default: `huggle-rc.wmflabs.org`)
- `--port` (default: `8822`)
- `--timeout` (default: `5` seconds)
- `--state-file` (default: `/tmp/xmlrcs_test/last_io`)

Example:

```bash
./check_xmlrcs.py --state-file /tmp/xmlrcs_test/last_io
```

## Nagios command example

```nagios
define command {
  command_name    check_xmlrcs
  command_line    /path/to/check_xmlrcs.py --state-file /tmp/xmlrcs_test/last_io
}
```

Ensure the Nagios user can write the state file path.
