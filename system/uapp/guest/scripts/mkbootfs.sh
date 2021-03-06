#!/usr/bin/env bash

# Copyright 2017 The Fuchsia Authors
#
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT

SRCFILE="$PWD/${BASH_SOURCE[0]}"
MAGENTADIR="${SRCFILE%magenta/*}magenta"
BUILDDIR="$MAGENTADIR/build-magenta-pc-x86-64"

KERNEL="${2:-$BUILDDIR/magenta.bin}"
BOOTDATA="${3:-$BUILDDIR/bootdata.bin}"
LINUX="${4:-$BUILDDIR/linux-x86/vmlinux}"

echo "
data/dsdt.aml=$MAGENTADIR/system/ulib/hypervisor/acpi/dsdt.aml
data/madt.aml=$MAGENTADIR/system/ulib/hypervisor/acpi/madt.aml
data/kernel.bin=$KERNEL
data/bootdata.bin=$BOOTDATA" > /tmp/guest.manifest

if [ -f "$LINUX" ]; then
    echo "data/vmlinux=$LINUX" >> /tmp/guest.manifest
fi

$BUILDDIR/tools/mkbootfs \
    --target=boot \
    -o $BUILDDIR/bootdata-with-kernel.bin \
    $BUILDDIR/bootdata.bin \
    /tmp/guest.manifest
