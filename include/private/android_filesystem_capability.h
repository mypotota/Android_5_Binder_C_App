/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * 从 linux/capability.h 中提取并稍作修改
 * 本文件定义了 Linux Capability 的用户空间 API 和所有 capability 常量。
 * 主要被 android_filesystem_config.h 引用，用于给文件设置额外的 capability。
 */

#ifndef _SYSTEM_CORE_INCLUDE_PRIVATE_ANDROID_FILESYSTEM_CAPABILITY_H
#define _SYSTEM_CORE_INCLUDE_PRIVATE_ANDROID_FILESYSTEM_CAPABILITY_H

#include <stdint.h>

/* 用户空间类型别名（模拟内核头文件风格） */
#define __user
#define __u32 uint32_t
#define __le32 uint32_t

/* Linux capability 协议版本
 * V1: 1 个 32 位能力位
 * V2: 2 个 32 位能力位
 * V3: 2 个 32 位能力位 + 边界集 */
#define _LINUX_CAPABILITY_VERSION_1 0x19980330
#define _LINUX_CAPABILITY_U32S_1 1
#define _LINUX_CAPABILITY_VERSION_2 0x20071026
#define _LINUX_CAPABILITY_U32S_2 2
#define _LINUX_CAPABILITY_VERSION_3 0x20080522
#define _LINUX_CAPABILITY_U32S_3 2

/* capget() / capset() 的头部结构 */
typedef struct __user_cap_header_struct {
 __u32 version;   /* capability 版本号 */
 int pid;         /* 目标进程 PID（0 表示当前进程） */
} __user *cap_user_header_t;

/* capget() / capset() 的数据结构
 * 三个字段都是位掩码：
 * - effective:   当前生效的能力
 * - permitted:    允许保留的能力上限
 * - inheritable: 可通过 execve 继承的能力 */
typedef struct __user_cap_data_struct {
 __u32 effective;
 __u32 permitted;
 __u32 inheritable;
} __user *cap_user_data_t;

/* ============================================================
 * VFS capability xattr 格式（文件扩展属性）
 *
 * 存储在文件系统扩展属性 security.capability 中，
 * 用于给可执行文件指定附加的 capability（替代传统 setuid/setgid）。
 * ============================================================ */

/* revision 字段位于 magic_etc 的高 8 位，effectiveness 标志在低 8 位 */
#define VFS_CAP_REVISION_MASK 0xFF000000
#define VFS_CAP_REVISION_SHIFT 24
#define VFS_CAP_FLAGS_MASK ~VFS_CAP_REVISION_MASK
#define VFS_CAP_FLAGS_EFFECTIVE 0x000001

/* VFS capability 有 revision 1 和 2 两种格式 */
#define VFS_CAP_REVISION_1 0x01000000
#define VFS_CAP_U32_1 1
#define XATTR_CAPS_SZ_1 (sizeof(__le32)*(1 + 2*VFS_CAP_U32_1))
#define VFS_CAP_REVISION_2 0x02000000
#define VFS_CAP_U32_2 2
#define XATTR_CAPS_SZ_2 (sizeof(__le32)*(1 + 2*VFS_CAP_U32_2))
#define XATTR_CAPS_SZ XATTR_CAPS_SZ_2
#define VFS_CAP_U32 VFS_CAP_U32_2
#define VFS_CAP_REVISION VFS_CAP_REVISION_2

/**
 * @brief VFS capability 扩展属性数据结构
 * 
 * 结构：[revision + effective_flag] [permitted[0]] [inheritable[0]] [permitted[1]] [inheritable[1]]
 */
struct vfs_cap_data {
 __le32 magic_etc;              /* 高 8 位 = revision，低 8 位 = effectiveness */
 struct {
 __le32 permitted;              /* 允许的 capability 位 */
 __le32 inheritable;            /* 可继承的 capability 位 */
 } data[VFS_CAP_U32];           /* U32 数组（V2 版本为 2 个） */
};

/* 当前默认使用 V1 版本的 capability */
#define _LINUX_CAPABILITY_VERSION _LINUX_CAPABILITY_VERSION_1
#define _LINUX_CAPABILITY_U32S _LINUX_CAPABILITY_U32S_1

/* ============================================================
 * Linux Capability 常量（POSIX.1e 标准）
 * 每个常量对应一个特定的特权操作，值即为 capability 位编号
 * 
 * 常用的 Android 场景：
 * - CAP_SETUID / CAP_SETGID:  run-as 工具使用
 * - CAP_NET_BIND_SERVICE:     绑定 1024 以下端口
 * - CAP_NET_ADMIN:            网络配置
 * - CAP_SYS_ADMIN:            超级管理能力（谨慎使用）
 * ============================================================ */
#define CAP_CHOWN 0              /* 允许 chown 任何文件 */
#define CAP_DAC_OVERRIDE 1       /* 绕过文件权限检查 */
#define CAP_DAC_READ_SEARCH 2    /* 绕过读/执行权限检查 */
#define CAP_FOWNER 3             /* 忽略文件所有者检查 */
#define CAP_FSETID 4             /* 允许设置 setuid/setgid 位 */
#define CAP_KILL 5               /* 允许向任意进程发送信号 */
#define CAP_SETGID 6             /* 允许改变进程的 GID */
#define CAP_SETUID 7             /* 允许改变进程的 UID */
#define CAP_SETPCAP 8            /* 允许转移/保留 capability */
#define CAP_LINUX_IMMUTABLE 9    /* 允许修改 S_IMMUTABLE 和 S_APPEND */
#define CAP_NET_BIND_SERVICE 10  /* 允许绑定低于 1024 的端口 */
#define CAP_NET_BROADCAST 11     /* 允许网络广播 */
#define CAP_NET_ADMIN 12         /* 允许网络管理操作 */
#define CAP_NET_RAW 13           /* 允许使用原始套接字 */
#define CAP_IPC_LOCK 14          /* 允许锁定共享内存 */
#define CAP_IPC_OWNER 15         /* 绕过 SysV IPC 权限检查 */
#define CAP_SYS_MODULE 16        /* 允许插入/移除内核模块 */
#define CAP_SYS_RAWIO 17         /* 允许访问 /dev/kmem 等原始设备 */
#define CAP_SYS_CHROOT 18        /* 允许 chroot */
#define CAP_SYS_PTRACE 19        /* 允许 ptrace 任意进程 */
#define CAP_SYS_PACCT 20         /* 允许启用/禁用进程记账 */
#define CAP_SYS_ADMIN 21         /* 执行大量管理操作（超级能力） */
#define CAP_SYS_BOOT 22          /* 允许重启系统 */
#define CAP_SYS_NICE 23          /* 允许修改任意进程的优先级 */
#define CAP_SYS_RESOURCE 24      /* 覆盖资源限制 */
#define CAP_SYS_TIME 25          /* 允许修改系统时间 */
#define CAP_SYS_TTY_CONFIG 26    /* 允许配置 TTY 设备 */
#define CAP_MKNOD 27             /* 允许使用 mknod */
#define CAP_LEASE 28             /* 允许建立文件租约 */
#define CAP_AUDIT_WRITE 29       /* 允许写入审计日志 */
#define CAP_AUDIT_CONTROL 30     /* 允许启用/禁用审计 */
#define CAP_SETFCAP 31           /* 允许设置文件 capability */
#define CAP_MAC_OVERRIDE 32       /* 允许覆盖 MAC 强制访问控制 */
#define CAP_MAC_ADMIN 33         /* 允许 MAC 配置更改 */
#define CAP_SYSLOG 34            /* 允许查看内核日志 */
#define CAP_WAKE_ALARM 35        /* 允许触发唤醒闹钟 */
#define CAP_LAST_CAP CAP_WAKE_ALARM

/* capability 位操作宏：
 * cap_valid(x): 检查 x 是否为有效 capability 编号
 * CAP_TO_INDEX(x): 计算 capability x 在位掩码数组中的索引
 * CAP_TO_MASK(x): 计算 capability x 在位掩码中的位移 */
#define cap_valid(x) ((x) >= 0 && (x) <= CAP_LAST_CAP)
#define CAP_TO_INDEX(x) ((x) >> 5)
#define CAP_TO_MASK(x) (1 << ((x) & 31))

/* 恢复临时定义的类型别名 */
#undef __user
#undef __u32
#undef __le32

#endif
