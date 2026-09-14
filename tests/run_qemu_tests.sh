#!/usr/bin/env bash
# Native cross-compile + qemu-arm-static correctness loop for this
# library's resize scalers, per NEON_TESTING.md. Static-links against
# lib/libneonarmmiyoo.a so qemu-arm-static needs no sysroot. Rebuilds the
# library first so this always tests current source, not a stale lib/.
set -euo pipefail

script_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(CDPATH= cd -- "$script_dir/.." && pwd)"

CFLAGS="-marm -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv7ve"

export CROSS_COMPILE="${CROSS_COMPILE:-arm-linux-gnueabihf-}"
make -C "$repo_root" clean
make -C "$repo_root"

out_bin="$(mktemp -u)"
${CROSS_COMPILE}gcc $CFLAGS \
  -I"$repo_root/include" \
  "$script_dir/test_scale_matrix.c" "$repo_root/lib/libneonarmmiyoo.a" \
  -static -o "$out_bin"

qemu-arm-static "$out_bin"
status=$?
rm -f "$out_bin"
exit $status
