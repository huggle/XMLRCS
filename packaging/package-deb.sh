#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DEBIAN_DIR="$PROJECT_ROOT/debian"
OUTPUT_DIR="$SCRIPT_DIR/output"
NCPUS="$(nproc)"

APP_VERSION="$(
    awk -F'"' '/Configuration::version/ { print $2; exit }' \
        "$PROJECT_ROOT/src/xmlrcsd/configuration.cpp"
)"

usage() {
    echo "Usage: $0 [--version x.y.z]"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version)
            APP_VERSION="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

if [[ -z "${APP_VERSION:-}" ]]; then
    echo "Error: unable to determine package version"
    exit 1
fi

if [[ ! -d "$DEBIAN_DIR" ]]; then
    echo "Error: Debian packaging metadata not found at $DEBIAN_DIR"
    exit 1
fi

echo "================================"
echo "Building Debian packages for XMLRCS"
echo "================================"
echo "Project root: $PROJECT_ROOT"
echo "CPUs: $NCPUS"
echo "Version: $APP_VERSION"
echo ""

if command -v dpkg-query >/dev/null 2>&1; then
    missing=()

    require_pkg() {
        local pkg="$1"
        if ! dpkg-query -W -f='${Status}' "$pkg" 2>/dev/null | grep -q "install ok installed"; then
            missing+=("$pkg")
        fi
    }

    require_pkg build-essential
    require_pkg cmake
    require_pkg debhelper
    require_pkg dh-python
    require_pkg dpkg-dev
    require_pkg pkgconf
    require_pkg python3-all

    if [[ ${#missing[@]} -gt 0 ]]; then
        echo "Missing build dependencies detected."
        echo ""
        if command -v apt-get >/dev/null 2>&1; then
            echo "Install them and re-run:"
            echo "  sudo apt-get install ${missing[*]}"
        else
            echo "Install these packages and re-run:"
            printf '  %s\n' "${missing[@]}"
        fi
        exit 1
    fi
fi

DISTRO_TAG=""
if [[ -r /etc/os-release ]]; then
    . /etc/os-release
    if [[ "${ID:-}" == "debian" && -n "${VERSION_ID:-}" ]]; then
        DISTRO_TAG="deb${VERSION_ID}"
    elif [[ "${ID:-}" == "ubuntu" && -n "${VERSION_ID:-}" ]]; then
        DISTRO_TAG="ubuntu${VERSION_ID}"
    fi
fi

if [[ -n "$DISTRO_TAG" ]]; then
    DEB_VERSION="${APP_VERSION}-1~${DISTRO_TAG}"
else
    DEB_VERSION="${APP_VERSION}-1"
fi

BACKUP_CHANGELOG=""
if [[ -f "$DEBIAN_DIR/changelog" ]]; then
    BACKUP_CHANGELOG="$(mktemp)"
    cp "$DEBIAN_DIR/changelog" "$BACKUP_CHANGELOG"
fi

cleanup() {
    if [[ -n "$BACKUP_CHANGELOG" && -f "$BACKUP_CHANGELOG" ]]; then
        cp "$BACKUP_CHANGELOG" "$DEBIAN_DIR/changelog"
        rm -f "$BACKUP_CHANGELOG"
    fi
}
trap cleanup EXIT

cat > "$DEBIAN_DIR/changelog" <<EOF
xmlrcs (${DEB_VERSION}) unstable; urgency=medium

  * Automated build.

 -- $(git config user.name 2>/dev/null || echo "Petr Bena") <$(git config user.email 2>/dev/null || echo "petr@bena.rocks")>  $(date -R)
EOF

rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

echo "Running dpkg-buildpackage..."
cd "$PROJECT_ROOT"
DEB_BUILD_OPTIONS="parallel=${NCPUS}" dpkg-buildpackage -b -us -uc

echo ""
echo "Collecting .deb output..."

mapfile -t DEB_FILES < <(find "$PROJECT_ROOT/.." "$PROJECT_ROOT" -maxdepth 1 -type f -name "*_${DEB_VERSION}_*.deb" | sort -u)

if [[ ${#DEB_FILES[@]} -eq 0 ]]; then
    echo "Error: no .deb artifacts found for version $DEB_VERSION"
    exit 1
fi

for deb in "${DEB_FILES[@]}"; do
    mv "$deb" "$OUTPUT_DIR/"
done

echo ""
echo "================================"
echo "Build complete"
echo "================================"
printf '  %s\n' "$OUTPUT_DIR"/*.deb
echo ""
echo "To install:"
echo "  sudo dpkg -i $OUTPUT_DIR/*.deb"
