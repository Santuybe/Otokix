#!/bin/bash
# Skrip build lokal di server Dev
# Usage: ./build.sh [release|debug]
set -e

# Pastikan Docker tersedia
if ! command -v docker &> /dev/null; then
    echo "Docker tidak ditemukan. Pastikan sudah terinstall di Dev."
    exit 1
fi

BUILD_TYPE=${1:-release}
echo "Memulai build: $BUILD_TYPE"

# Cleanup workspace lama (ignore errors from container mount perms)
rm -rf workspace 2>/dev/null || true
mkdir -p workspace

# Copy file ke workspace
cp -r src include Makefile config.json fonts workspace/

# Pull toolchain
docker pull savant/minui-toolchain:my282

# Run build
CONTAINER_ID=$(docker run -d -v "$(pwd)/workspace":/root/workspace savant/minui-toolchain:my282 tail -f /dev/null)
echo "Container: $CONTAINER_ID"

# Install zip di container
docker exec "$CONTAINER_ID" bash -c "apt-get update && apt-get install -y zip"

# Build
if [ "$BUILD_TYPE" = "debug" ]; then
    docker exec "$CONTAINER_ID" bash -c "source /root/.bashrc && make -C /root/workspace pak-debug"
    echo "Hasil build: workspace/Otokix-my282-debug.pak.zip"
else
    docker exec "$CONTAINER_ID" bash -c "source /root/.bashrc && make -C /root/workspace pak"
    echo "Hasil build: workspace/Otokix-my282.pak.zip"
fi

# Cleanup container
docker container rm -f "$CONTAINER_ID"

echo "Build selesai."
