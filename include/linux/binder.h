/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/

/* ============================================================
 * Android Binder IPC — 内核与用户空间接口定义
 *
 * 本文件定义了 Android Binder 驱动 (/dev/binder) 的用户空间 API，
 * 包括所有 ioctl 命令、事务结构体、引用协议等。
 *
 * 关键概念：
 * - flat_binder_object: 进程间传递的 binder 对象（引用或指针）
 * - binder_transaction_data: 一次 Binder 调用的完整数据包
 * - BC_* 命令: 用户空间 → 驱动（Command）
 * - BR_* 命令: 驱动 → 用户空间（Return/Response）
 * ============================================================ */

#ifndef _UAPI_LINUX_BINDER_H
#define _UAPI_LINUX_BINDER_H
#include <linux/ioctl.h>

/* 将四个 ASCII 字符打包成 32 位类型标识（如 'sb*'、'sh*' 等） */
#define B_PACK_CHARS(c1, c2, c3, c4)   ((((c1)<<24)) | (((c2)<<16)) | (((c3)<<8)) | (c4))

/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */

/* 对象类型标志：表示 binder 对象为"大类型"，使用 B_PACK_CHARS 编码 */
#define B_TYPE_LARGE 0x85

/* ============================================================
 * Binder 对象类型枚举
 * - BINDER_TYPE_BINDER:    本地 binder 实体（服务端发布的接口指针）
 * - BINDER_TYPE_WEAK_BINDER: 本地弱引用 binder
 * - BINDER_TYPE_HANDLE:    远程 binder 代理（客户端持有的服务句柄）
 * - BINDER_TYPE_WEAK_HANDLE: 远程弱引用句柄
 * - BINDER_TYPE_FD:       文件描述符跨进程传递
 * ============================================================ */
enum {
 BINDER_TYPE_BINDER = B_PACK_CHARS('s', 'b', '*', B_TYPE_LARGE),
 BINDER_TYPE_WEAK_BINDER = B_PACK_CHARS('w', 'b', '*', B_TYPE_LARGE),
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 BINDER_TYPE_HANDLE = B_PACK_CHARS('s', 'h', '*', B_TYPE_LARGE),
 BINDER_TYPE_WEAK_HANDLE = B_PACK_CHARS('w', 'h', '*', B_TYPE_LARGE),
 BINDER_TYPE_FD = B_PACK_CHARS('f', 'd', '*', B_TYPE_LARGE),
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */

/* flat_binder_object.flags 标志：
 * FLAT_BINDER_FLAG_PRIORITY_MASK:  优先级掩码（低 8 位）
 * FLAT_BINDER_FLAG_ACCEPTS_FDS:    该 binder 对象可接受文件描述符传递
 */
enum {
 FLAT_BINDER_FLAG_PRIORITY_MASK = 0xff,
 FLAT_BINDER_FLAG_ACCEPTS_FDS = 0x100,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */

/* 根据 BINDER_IPC_32BIT 宏选择指针大小类型：
 * 32 位系统：binder_size_t 和 binder_uintptr_t 均为 32 位
 * 64 位系统：均为 64 位
 */
#ifdef BINDER_IPC_32BIT
typedef __u32 binder_size_t;
typedef __u32 binder_uintptr_t;
#else
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
typedef __u64 binder_size_t;
typedef __u64 binder_uintptr_t;
#endif

/**
 * @brief Binder 对象的扁平表示（可跨进程传递）
 * 
 * 当 binder 对象从一个进程传递到另一个进程时，
 * 驱动会将其转换为 flat_binder_object 格式：
 * - type: BINDER_TYPE_BINDER 时 binder 字段为本地指针
 * - type: BINDER_TYPE_HANDLE 时 handle 字段为远程句柄
 * - cookie: 对端回调时原样返回的用户数据
 */
struct flat_binder_object {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 type;      /* 对象类型（BINDER_TYPE_* 枚举值） */
 __u32 flags;     /* 对象标志（FLAT_BINDER_FLAG_*） */
 union {
 binder_uintptr_t binder;  /* 本地 binder 实体指针（BINDER_TYPE_BINDER） */
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 handle;              /* 远程 binder 句柄（BINDER_TYPE_HANDLE） */
 };
 binder_uintptr_t cookie;   /* 用户自定义数据，原样传递 */
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */

/**
 * @brief Binder 驱动读写结构（BINDER_WRITE_READ ioctl 的参数）
 * 
 * 用户空间通过一次 ioctl 同时完成写入命令和读取驱动返回的数据。
 * 写入和读取可以独立为 0（只写或只读）。
 */
struct binder_write_read {
 binder_size_t write_size;      /* 要写入的字节数 */
 binder_size_t write_consumed;  /* 驱动已消费的字节数（输出） */
 binder_uintptr_t write_buffer;  /* 写入缓冲区指针 */
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 binder_size_t read_size;       /* 读取缓冲区大小 */
 binder_size_t read_consumed;   /* 驱动已返回的字节数（输出） */
 binder_uintptr_t read_buffer;  /* 读取缓冲区指针 */
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */

/**
 * @brief Binder 驱动协议版本结构
 */
struct binder_version {
 __s32 protocol_version;  /* 协议版本号：32位=7，64位=8 */
};
#ifdef BINDER_IPC_32BIT
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define BINDER_CURRENT_PROTOCOL_VERSION 7
#else
#define BINDER_CURRENT_PROTOCOL_VERSION 8
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */

/* ============================================================
 * Binder ioctl 命令集（驱动接口）
 *
 * BINDER_WRITE_READ:          最核心的读写命令（同时写入命令并读取返回数据）
 * BINDER_SET_IDLE_TIMEOUT:    设置空闲线程超时时间
 * BINDER_SET_MAX_THREADS:    设置驱动允许的最大线程数
 * BINDER_SET_IDLE_PRIORITY:  设置空闲线程优先级
 * BINDER_SET_CONTEXT_MGR:    声明当前进程为 Context Manager（Service Manager）
 * BINDER_THREAD_EXIT:         通知驱动线程即将退出
 * BINDER_VERSION:            查询驱动协议版本
 * ============================================================ */
#define BINDER_WRITE_READ _IOWR('b', 1, struct binder_write_read)
#define BINDER_SET_IDLE_TIMEOUT _IOW('b', 3, __s64)
#define BINDER_SET_MAX_THREADS _IOW('b', 5, __u32)
#define BINDER_SET_IDLE_PRIORITY _IOW('b', 6, __s32)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define BINDER_SET_CONTEXT_MGR _IOW('b', 7, __s32)
#define BINDER_THREAD_EXIT _IOW('b', 8, __s32)
#define BINDER_VERSION _IOWR('b', 9, struct binder_version)

/* ============================================================
 * Binder 事务标志（transaction_flags）
 * - TF_ONE_WAY:     单向调用（不期望回复）
 * - TF_ROOT_OBJECT: 作为初始 binder 根对象传递
 * - TF_STATUS_CODE: 回复中只携带 status 错误码
 * - TF_ACCEPT_FDS:  允许在事务中传递文件描述符
 * ============================================================ */
enum transaction_flags {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 TF_ONE_WAY = 0x01,
 TF_ROOT_OBJECT = 0x04,
 TF_STATUS_CODE = 0x08,
 TF_ACCEPT_FDS = 0x10,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};

/**
 * @brief Binder 事务数据结构（BC_TRANSACTION / BC_REPLY 的完整载荷）
 * 
 * 一次 Binder 调用的完整信息：
 * - target: 目标 binder（客户端时是 handle，服务端回复时是 ptr）
 * - code:   调用方法的事务码
 * - flags:  事务标志（transaction_flags）
 * - sender_pid/euid: 发送者身份（由驱动填充，用于权限检查）
 * - data_size/offsets_size: 数据区大小和偏移数组大小
 * - data.ptr.buffer/offsets: 数据缓冲区和对象偏移数组
 */
struct binder_transaction_data {
 union {
 __u32 handle;                 /* 客户端调用时：目标服务的远程句柄 */
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 binder_uintptr_t ptr;         /* 服务端回复时：本地 binder 实体指针 */
 } target;
 binder_uintptr_t cookie;      /* 目标 binder 的 cookie（回复时原样返回） */
 __u32 code;                   /* 方法事务码（如 SVC_MGR_GET_SERVICE） */
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 flags;                  /* 事务标志（TF_*） */
 pid_t sender_pid;             /* 发送者进程 PID（驱动自动填充） */
 uid_t sender_euid;            /* 发送者有效 UID（驱动自动填充） */
 binder_size_t data_size;      /* 数据缓冲区中有效数据的字节数 */
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 binder_size_t offsets_size;   /* 对象偏移数组的字节数 */
 union {
 struct {
 binder_uintptr_t buffer;      /* 数据缓冲区指针 */
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 binder_uintptr_t offsets;     /* 对象偏移数组指针 */
 } ptr;
 __u8 buf[8];                  /* 原始字节视图 */
 } data;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};

/* 引用计数操作的参数结构（ptr + cookie 对） */
struct binder_ptr_cookie {
 binder_uintptr_t ptr;       /* binder 对象指针 */
 binder_uintptr_t cookie;    /* 用户回调 cookie */
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};

/**
 * @brief 句柄 + cookie 对（packed 保证无对齐填充）
 * 用于 BC_REQUEST_DEATH_NOTIFICATION 等命令
 */
struct binder_handle_cookie {
 __u32 handle;              /* 远程 binder 句柄 */
 binder_uintptr_t cookie;   /* 用户回调时原样返回的 cookie */
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} __attribute__((packed));

/* 带优先级的 binder 描述结构（用于优先级继承） */
struct binder_pri_desc {
 __s32 priority;   /* 优先级值 */
 __u32 desc;       /* binder 描述符 */
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};

/* 带优先级的 ptr + cookie 结构 */
struct binder_pri_ptr_cookie {
 __s32 priority;
 binder_uintptr_t ptr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 binder_uintptr_t cookie;
};

/* ============================================================
 * Binder 驱动 → 用户空间（BR_*）返回协议
 * 驱动通过 BINDER_WRITE_READ 的读路径返回这些命令：
 * - BR_TRANSACTION:      服务端收到新的事务请求
 * - BR_REPLY:            客户端收到之前调用的回复
 * - BR_DEAD_BINDER:      客户端监听的 binder 已死亡
 * - BR_SPAWN_LOOPER:     驱动请求服务端创建新工作线程
 * - BR_INCREFS/ACQUIRE/RELEASE/DECREFS: 引用计数变化通知
 * ============================================================ */
enum binder_driver_return_protocol {
 BR_ERROR = _IOR('r', 0, __s32),          /* 通用错误返回 */
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 BR_OK = _IO('r', 1),                     /* 操作成功完成 */
 BR_TRANSACTION = _IOR('r', 2, struct binder_transaction_data), /* 新的事务请求 */
 BR_REPLY = _IOR('r', 3, struct binder_transaction_data),       /* 事务回复 */
 BR_ACQUIRE_RESULT = _IOR('r', 4, __s32),
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 BR_DEAD_REPLY = _IO('r', 5),             /* 目标 binder 已死亡，无法回复 */
 BR_TRANSACTION_COMPLETE = _IO('r', 6),   /* 单向事务完成通知 */
 BR_INCREFS = _IOR('r', 7, struct binder_ptr_cookie), /* 增加弱引用计数 */
 BR_ACQUIRE = _IOR('r', 8, struct binder_ptr_cookie), /* 增加强引用计数 */
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 BR_RELEASE = _IOR('r', 9, struct binder_ptr_cookie), /* 减少强引用计数 */
 BR_DECREFS = _IOR('r', 10, struct binder_ptr_cookie), /* 减少弱引用计数 */
 BR_ATTEMPT_ACQUIRE = _IOR('r', 11, struct binder_pri_ptr_cookie),
 BR_NOOP = _IO('r', 12),                 /* 空操作，可忽略 */
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 BR_SPAWN_LOOPER = _IO('r', 13),         /* 驱动请求创建新 Looper 线程 */
 BR_FINISHED = _IO('r', 14),             /* 引用计数操作完成 */
 BR_DEAD_BINDER = _IOR('r', 15, binder_uintptr_t), /* 监听的 binder 已死亡 */
 BR_CLEAR_DEATH_NOTIFICATION_DONE = _IOR('r', 16, binder_uintptr_t),
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 BR_FAILED_REPLY = _IO('r', 17),          /* 事务失败回复 */
};

/* ============================================================
 * 用户空间 → Binder 驱动（BC_*）命令协议
 * 用户空间通过 BINDER_WRITE_READ 的写路径发送这些命令：
 * - BC_TRANSACTION:              客户端发起事务调用
 * - BC_REPLY:                    服务端回复事务
 * - BC_FREE_BUFFER:              释放共享缓冲区
 * - BC_ACQUIRE/RELEASE:          增加/减少强引用
 * - BC_REGISTER_LOOPER/ENTER_LOOPER: 注册为 Looper 线程
 * - BC_REQUEST_DEATH_NOTIFICATION:    请求死亡通知
 * ============================================================ */
enum binder_driver_command_protocol {
 BC_TRANSACTION = _IOW('c', 0, struct binder_transaction_data), /* 发起事务调用 */
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 BC_REPLY = _IOW('c', 1, struct binder_transaction_data),      /* 事务回复 */
 BC_ACQUIRE_RESULT = _IOW('c', 2, __s32),
 BC_FREE_BUFFER = _IOW('c', 3, binder_uintptr_t),              /* 释放共享缓冲区 */
 BC_INCREFS = _IOW('c', 4, __u32),
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 BC_ACQUIRE = _IOW('c', 5, __u32),                             /* 增加强引用 */
 BC_RELEASE = _IOW('c', 6, __u32),                             /* 减少强引用 */
 BC_DECREFS = _IOW('c', 7, __u32),
 BC_INCREFS_DONE = _IOW('c', 8, struct binder_ptr_cookie),
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 BC_ACQUIRE_DONE = _IOW('c', 9, struct binder_ptr_cookie),
 BC_ATTEMPT_ACQUIRE = _IOW('c', 10, struct binder_pri_desc),
 BC_REGISTER_LOOPER = _IO('c', 11),     /* 注册为 Looper 线程（子线程） */
 BC_ENTER_LOOPER = _IO('c', 12),        /* 声明进入 Looper（主线程） */
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 BC_EXIT_LOOPER = _IO('c', 13),         /* 退出 Looper */
 BC_REQUEST_DEATH_NOTIFICATION = _IOW('c', 14, struct binder_handle_cookie), /* 请求死亡通知 */
 BC_CLEAR_DEATH_NOTIFICATION = _IOW('c', 15, struct binder_handle_cookie),
 BC_DEAD_BINDER_DONE = _IOW('c', 16, binder_uintptr_t),
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#endif
