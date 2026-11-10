/* linux/ioctl.h — macOS 下的 stub 文件，让 clangd 能正确解析项目
 *
 * 项目是为 Android/Linux 编译的（使用 arm-linux-gcc），
 * 但代码索引在 macOS 上做。本文件提供：
 * 1. _IO / _IOR / _IOW / _IOWR 宏（Linux ioctl 宏）
 * 2. __u32 / __s32 / __u64 / __le32 等内核类型别名
 * 3. pid_t / uid_t 等 POSIX 类型
 *
 * 注意：此文件不参与交叉编译（Makefile 使用 arm-linux-gcc，
 *       它自带完整的 linux/ioctl.h）。
 */

#ifndef _LINUX_IOCTL_STUB_H
#define _LINUX_IOCTL_STUB_H

#include <stdint.h>
#include <sys/types.h>

/* === Linux 内核类型别名 === */
#ifndef __u32
typedef uint32_t  __u32;
typedef uint64_t  __u64;
typedef uint16_t  __u16;
typedef uint8_t   __u8;
typedef int32_t   __s32;
typedef int64_t   __s64;
typedef int16_t   __s16;
typedef int8_t    __s8;
typedef uint32_t  __le32;
typedef uint64_t  __le64;
typedef uint16_t  __le16;
typedef uint8_t   __le8;
#endif

/* === Linux ioctl 宏 ===
 *
 * 编码格式（低 → 高）:
 *   dir(2bit) | nr(8bit) | type(8bit) | size(14bit)
 *   bits 0-1   2-9         10-17        18-31
 *
 * 下面按 Linux 内核的 _IOC 宏正确实现
 */

#define _IO_NRBITS      8
#define _IO_TYPEBITS    8
#define _IOC_SIZEBITS  14
#define _IOC_DIRBITS    2

#define _IO_NRMASK     ((1 << _IO_NRBITS) - 1)
#define _IO_TYPEMASK   ((1 << _IO_TYPEBITS) - 1)
#define _IOC_SIZEMASK  ((1 << _IOC_SIZEBITS) - 1)
#define _IOC_DIRMASK   ((1 << _IOC_DIRBITS) - 1)

#define _IO_NRSHIFT     0
#define _IO_TYPESHIFT   (_IO_NRSHIFT + _IO_NRBITS)          /* 8 */
#define _IOC_SIZESHIFT  (_IO_TYPESHIFT + _IO_TYPEBITS)      /* 16 */
#define _IOC_DIRSHIFT   (_IOC_SIZESHIFT + _IOC_SIZEBITS)    /* 30 */

#define _IOC_NONE      0U
#define _IOC_WRITE     1U
#define _IOC_READ      2U

#define _IOC(dir, type, nr, size) \
    (((dir)  << _IOC_DIRSHIFT) | \
     ((type) << _IO_TYPESHIFT) | \
     ((nr)   << _IO_NRSHIFT) | \
     ((size) << _IOC_SIZESHIFT))

#define _IO(type, nr)              _IOC(_IOC_NONE,  (type), (nr), 0)
#define _IOR(type, nr, size)       _IOC(_IOC_READ,  (type), (nr), sizeof(size))
#define _IOW(type, nr, size)       _IOC(_IOC_WRITE, (type), (nr), sizeof(size))
#define _IOWR(type, nr, size)      _IOC(_IOC_READ | _IOC_WRITE, (type), (nr), sizeof(size))

#endif /* _LINUX_IOCTL_STUB_H */
