/* linux/types.h — macOS 下的 stub 文件
 *
 * 只是简单地 re-export 标准 POSIX 头文件。
 * 实际类型在 include/linux/ioctl.h 中定义，
 * 这里确保没有文件会因为找不到 linux/types.h 而报错。
 */
#ifndef _LINUX_TYPES_STUB_H
#define _LINUX_TYPES_STUB_H

#include <stdint.h>
#include <sys/types.h>

#endif /* _LINUX_TYPES_STUB_H */
