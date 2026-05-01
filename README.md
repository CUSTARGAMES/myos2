# 🦊 TFD OS v1.0 "Foxy"

**The World's Lightest Modern Operating System**

![Version](https://img.shields.io/badge/version-1.0-green)
![License](https://img.shields.io/badge/license-GPL%20v3-blue)
![Architecture](https://img.shields.io/badge/arch-x86%2032--bit-orange)
![Type](https://img.shields.io/badge/type-Text--Based-lightgrey)
![RAM](https://img.shields.io/badge/RAM-2MB%20minimum-brightgreen)

---

## 📖 About

TFD OS (TinyFoxyDOS) is a **text-based operating system** built entirely from scratch by **Sadman** in 2026. It runs on real hardware using classic VGA text mode (80×25 characters) — no GPU required.

- ✅ **72 built-in commands**
- ✅ **Fake file system** (ls, cd, mkdir, touch, cat, rm)
- ✅ **3 Games** (Snake, Tetris, Pong)
- ✅ **Text editor** (FXC NOTEPAD)
- ✅ **Password lock system**
- ✅ **Install wizard** (fake permanent install)
- ✅ **Command history & aliases**
- ✅ **16 VGA colors support**

---

## 🖥️ System Requirements

| Component | Minimum |
|-----------|---------|
| **CPU** | Intel 80386 (i386) or newer |
| **RAM** | 2 MB |
| **Storage** | 64 MB USB drive |
| **Display** | VGA-compatible (80×25 text mode) |
| **GPU** | ❌ Not Required |
| **Keyboard** | PS/2 |
| **Mouse** | ❌ Not Required |
| **Network** | ❌ Not Required |

---

## 🚀 Quick Start

### Test in QEMU (No USB needed):
```bash
qemu-system-i386 -cdrom TFD-OS-v1.0.iso -m 32
