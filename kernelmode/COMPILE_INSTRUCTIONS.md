# Linux 3.9 Kernel Compilation Instructions

## Environment Requirements

**Cannot compile on macOS directly** — requires Linux environment.

### Option 1: Native Linux Machine (Recommended)
- Ubuntu 12.04 LTS or compatible
- 4+ GB RAM
- 20+ GB disk space
- GCC 4.6+, Make 3.8+

### Option 2: Docker Container
```bash
docker run --platform linux/amd64 -it ubuntu:12.04 bash
# Inside container: install build-essential, gcc, make, libncurses5-dev
```

### Option 3: AWS EC2
- Instance: t3.medium or larger
- AMI: Ubuntu Server 12.04 LTS (ami-...)
- Storage: 20 GB gp2

### Option 4: VirtualBox/VMware
- Guest OS: Ubuntu 12.04 LTS
- RAM: 4+ GB
- Disk: 20+ GB

## Compilation Steps

```bash
#!/bin/bash
set -e

# 1. Navigate to kernel source
cd /path/to/aerie/kernelmode/linux-3.9

# 2. Clean previous builds
make mrproper

# 3. Configure for your system
# For default x86-64:
make x86_64_defconfig

# Or interactive configuration:
make menuconfig

# 4. Compile (this takes 10-30 minutes depending on CPU)
make -j$(nproc)

# 5. Compile modules (optional)
make modules

# 6. Install (requires sudo)
sudo make modules_install
sudo make install

# 7. Update bootloader
sudo update-grub

# 8. Reboot to new kernel
sudo reboot
```

## Verification After Compilation

```bash
# Check kernel version
uname -r

# Verify SCM module loaded
grep SCM /proc/modules
or
lsmod | grep scm

# Test SCM syscalls
cat /proc/sys/kernel/scm_* 
```

## SCM-Specific Configuration Options

When running `make menuconfig`, ensure these options are enabled:

```
CONFIG_SCM=y                    # Enable SCM support
CONFIG_SCM_DEVICE=y             # SCM device driver
CONFIG_TRANSPARENT_HUGEPAGE=y   # THP support (optional but recommended)
```

## Build Artifacts

After successful compilation:

```
vmlinux                    - Uncompressed kernel image
arch/x86/boot/bzImage     - Compressed kernel for boot
```

## Troubleshooting

### "make: command not found"
```bash
sudo apt-get install build-essential
```

### "libncurses5-dev: not found"
```bash
sudo apt-get install libncurses5-dev
```

### "error: gcc version is too old"
Upgrade GCC or use system default:
```bash
sudo apt-get install gcc-4.6
```

### Out of disk space
Free up space:
```bash
make clean  # Remove object files but keep config
make mrproper  # Complete clean
```

## Time Estimates

| Hardware | Compile Time |
|----------|------------|
| Single Core (1 CPU) | 45-60 min |
| Dual Core (2 CPU) | 25-30 min |
| Quad Core (4 CPU) | 10-15 min |
| 8+ Core (8+ CPU) | 5-10 min |

## Next Steps

After compilation:
1. Boot into new kernel
2. Verify SCM subsystem is loaded
3. Run test suite against SCM kernel module
4. Compare with upstream Linux 3.9 baseline

---

**Note**: The aerie/kernelmode/linux-3.9 source is fully patched with Sankar's SCM support. All necessary modifications are already integrated. Just configure, compile, and install.
