/* Copyright 2008 The Android Open Source Project
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/types.h>
#include<stdbool.h>
#include <string.h>
#include <pthread.h>

#include "binder.h"

/* binder_io 缓冲区最大大小限制（1GB） */
#define MAX_BIO_SIZE (1 << 30)

/* 调试跟踪开关：设为1启用详细日志输出 */
#define TRACE 0

#if TRACE
#define ALOGI(x...) fprintf(stderr, "binder: " x)
#define ALOGE(x...) fprintf(stderr, "binder: " x)
#else
#define ALOGI(x...) 
#define ALOGE(x...) 
#endif

#define LOG_TAG "Binder"
//#include <cutils/log.h>

/* 前置声明：从 binder_transaction_data 初始化 binder_io 结构 */
void bio_init_from_txn(struct binder_io *io, struct binder_transaction_data *txn);

#if TRACE
/**
 * @brief 以十六进制和 ASCII 形式输出数据内容
 * 
 * 用于调试：将任意数据块以 hexdump 风格打印到 stderr，
 * 每行显示16字节，不可打印字符显示为 '.'
 * 
 * @param _data 待输出的数据指针
 * @param len 数据长度（字节）
 */
void hexdump(void *_data, size_t len)
{
    unsigned char *data = _data;
    size_t count;

    for (count = 0; count < len; count++) {
        if ((count & 15) == 0)
            fprintf(stderr,"%04zu:", count);
        fprintf(stderr," %02x %c", *data,
                (*data < 32) || (*data > 126) ? '.' : *data);
        data++;
        if ((count & 15) == 15)
            fprintf(stderr,"\n");
    }
    if ((count & 15) != 0)
        fprintf(stderr,"\n");
}

/**
 * @brief 输出 binder_transaction_data 的详细内容
 * 
 * 用于调试：打印事务的目标、cookie、调用码、标志位、
 * 发送者 pid/uid、数据大小，并 hexdump 实际数据内容
 * 
 * @param txn 待输出的事务数据结构指针
 */
void binder_dump_txn(struct binder_transaction_data *txn)
{
    struct flat_binder_object *obj;
    binder_size_t *offs = (binder_size_t *)(uintptr_t)txn->data.ptr.offsets;
    size_t count = txn->offsets_size / sizeof(binder_size_t);

    fprintf(stderr,"  target %016"PRIx64"  cookie %016"PRIx64"  code %08x  flags %08x\n",
            (uint64_t)txn->target.ptr, (uint64_t)txn->cookie, txn->code, txn->flags);
    fprintf(stderr,"  pid %8d  uid %8d  data %"PRIu64"  offs %"PRIu64"\n",
            txn->sender_pid, txn->sender_euid, (uint64_t)txn->data_size, (uint64_t)txn->offsets_size);
    hexdump((void *)(uintptr_t)txn->data.ptr.buffer, txn->data_size);
    while (count--) {
        obj = (struct flat_binder_object *) (((char*)(uintptr_t)txn->data.ptr.buffer) + *offs++);
        fprintf(stderr,"  - type %08x  flags %08x  ptr %016"PRIx64"  cookie %016"PRIx64"\n",
                obj->type, obj->flags, (uint64_t)obj->binder, (uint64_t)obj->cookie);
    }
}

#define NAME(n) case n: return #n
/**
 * @brief 将 binder 驱动命令码转换为可读字符串
 * @param cmd 驱动返回的命令码（如 BR_TRANSACTION）
 * @return 命令名字符串，未知命令返回 "???"
 */
const char *cmd_name(uint32_t cmd)
{
    switch(cmd) {
        NAME(BR_NOOP);
        NAME(BR_TRANSACTION_COMPLETE);
        NAME(BR_INCREFS);
        NAME(BR_ACQUIRE);
        NAME(BR_RELEASE);
        NAME(BR_DECREFS);
        NAME(BR_TRANSACTION);
        NAME(BR_REPLY);
        NAME(BR_FAILED_REPLY);
        NAME(BR_DEAD_REPLY);
        NAME(BR_DEAD_BINDER);		
    default: return "???";
    }
}
#else
#define hexdump(a,b) do{} while (0)
#define binder_dump_txn(txn)  do{} while (0)
#endif

#define BIO_F_SHARED    0x01  /* 需要 buffer 被释放 */
#define BIO_F_OVERFLOW  0x02  /* 空间不足 */
#define BIO_F_IOERROR   0x04
#define BIO_F_MALLOCED  0x08  /* 需要 free() */

/* ============================================================
 * 以下为 binder 核心数据结构
 * ============================================================ */

/**
 * @brief Binder 状态结构，保存与 /dev/binder 驱动的连接信息
 * 
 * 每次 binder_open() 会创建一个 binder_state，包含：
 * - fd: 打开的 binder 设备文件描述符
 * - mapped: 通过 mmap 映射的内核缓冲区指针
 * - mapsize: 映射区域大小
 */
struct binder_state
{
    int fd;       /* /dev/binder 文件描述符 */
    void *mapped; /* mmap 映射的内存起始地址 */
    size_t mapsize; /* 映射区域大小 */
};

/**
 * @brief 打开 binder 设备并建立内存映射
 * 
 * 1. 打开 /dev/binder 字符设备
 * 2. 校验驱动协议版本是否与用户空间一致
 * 3. 通过 mmap 将驱动缓冲区映射到用户空间
 * 
 * @param mapsize 期望映射的缓冲区大小（字节）
 * @return 成功返回 binder_state 指针，失败返回 NULL
 */
struct binder_state *binder_open(size_t mapsize)
{
    struct binder_state *bs;
    struct binder_version vers;

    bs = malloc(sizeof(*bs));
    if (!bs) {
        errno = ENOMEM;
        return NULL;
    }

    /* 第一步：打开 /dev/binder 字符设备，获取文件描述符 */
    bs->fd = open("/dev/binder", O_RDWR);
    if (bs->fd < 0) {
        fprintf(stderr,"binder: cannot open device (%s)\n",
                strerror(errno));
        goto fail_open;
    }

    /* 第二步：校验驱动协议版本，确保内核与用户空间匹配 */
    if ((ioctl(bs->fd, BINDER_VERSION, &vers) == -1) ||
        (vers.protocol_version != BINDER_CURRENT_PROTOCOL_VERSION)) {
        fprintf(stderr, "binder: driver version differs from user space\n");
        goto fail_open;
    }

    /* 第三步：mmap 映射驱动缓冲区到用户空间，用于共享事务数据 */
    bs->mapsize = mapsize;
    bs->mapped = mmap(NULL, mapsize, PROT_READ, MAP_PRIVATE, bs->fd, 0);
    if (bs->mapped == MAP_FAILED) {
        fprintf(stderr,"binder: cannot map device (%s)\n",
                strerror(errno));
        goto fail_map;
    }

    return bs;

fail_map:
    close(bs->fd);
fail_open:
    free(bs);
    return NULL;
}

/**
 * @brief 关闭 binder 连接，释放所有资源
 * @param bs binder_state 指针，由 binder_open() 返回
 */
void binder_close(struct binder_state *bs)
{
    munmap(bs->mapped, bs->mapsize); /* 解除 mmap 映射 */
    close(bs->fd);                    /* 关闭设备文件 */
    free(bs);                         /* 释放状态结构体内存 */
}

/**
 * @brief 将当前进程注册为 Binder 上下文管理者
 * 
 * 系统中同一时刻只能有一个 Context Manager，
 * 它负责管理所有已注册的 Binder 服务
 * 
 * @param bs binder 状态
 * @return 成功返回 0，失败返回 -1（进程已有其他 Context Manager）
 */
int binder_become_context_manager(struct binder_state *bs)
{
    return ioctl(bs->fd, BINDER_SET_CONTEXT_MGR, 0);
}

/**
 * @brief 向 binder 驱动写入原始数据
 * 
 * 这是所有 binder_ioctl 操作的底层函数，
 * 通过 BINDER_WRITE_READ ioctl 向内核发送命令
 * 
 * @param bs binder 状态
 * @param data 待写入的数据缓冲区
 * @param len 数据长度（字节）
 * @return ioctl 返回值，成功 >= 0，失败 < 0
 */
int binder_write(struct binder_state *bs, void *data, size_t len)
{
    struct binder_write_read bwr;
    int res;

    /* 构造 binder_write_read 结构：只写不读 */
    bwr.write_size = len;           /* 要写入的字节数 */
    bwr.write_consumed = 0;         /* 已消费字节数（输出） */
    bwr.write_buffer = (uintptr_t) data; /* 写入缓冲区指针 */
    bwr.read_size = 0;              /* 读取缓冲区大小为 0 */
    bwr.read_consumed = 0;
    bwr.read_buffer = 0;
    res = ioctl(bs->fd, BINDER_WRITE_READ, &bwr);
    if (res < 0) {
        fprintf(stderr,"binder_write: ioctl failed (%s)\n",
                strerror(errno));
    }
    return res;
}

/**
 * @brief 发送事务回复给 binder 驱动
 * 
 * 服务端处理完 BR_TRANSACTION 后调用此函数返回结果。
 * 同时负责释放请求缓冲区（BC_FREE_BUFFER + BC_REPLY 组合发送）
 * 
 * @param bs binder 状态
 * @param reply 回复数据（binder_io 结构）
 * @param buffer_to_free 需要释放的原请求缓冲区地址
 * @param status 处理状态：0 表示成功，非零表示错误码
 */
void binder_send_reply(struct binder_state *bs,
                       struct binder_io *reply,
                       binder_uintptr_t buffer_to_free,
                       int status)
{
    /* 用 packed 结构一次发送两个命令：先释放旧缓冲区，再发送回复 */
    struct {
        uint32_t cmd_free;       /* BC_FREE_BUFFER：释放请求侧的缓冲区 */
        binder_uintptr_t buffer; /* 要释放的缓冲区地址 */
        uint32_t cmd_reply;      /* BC_REPLY：发送回复 */
        struct binder_transaction_data txn; /* 回复的事务数据 */
    } __attribute__((packed)) data;

    data.cmd_free = BC_FREE_BUFFER;     /* 1. 先命令驱动释放旧缓冲区 */
    data.buffer = buffer_to_free;
    data.cmd_reply = BC_REPLY;          /* 2. 再发送回复 */
    data.txn.target.ptr = 0;            /* 回复时 target 由驱动自动填充 */
    data.txn.cookie = 0;
    data.txn.code = 0;
    if (status) {
        /* 有错误：回复中只携带 status 错误码 */
        data.txn.flags = TF_STATUS_CODE;
        data.txn.data_size = sizeof(int);
        data.txn.offsets_size = 0;
        data.txn.data.ptr.buffer = (uintptr_t)&status;
        data.txn.data.ptr.offsets = 0;
    } else {
        /* 成功：从 reply binder_io 中提取实际数据 */
        data.txn.flags = 0;
        data.txn.data_size = reply->data - reply->data0; /* 有效数据长度 */
        data.txn.offsets_size = ((char*) reply->offs) - ((char*) reply->offs0); /* 偏移数组长度 */
        data.txn.data.ptr.buffer = (uintptr_t)reply->data0;
        data.txn.data.ptr.offsets = (uintptr_t)reply->offs0;
    }
    binder_write(bs, &data, sizeof(data));
}


/**
 * @brief 独立的 binder 线程循环（配合 BR_SPAWN_LOOPER 使用）
 * 
 * 与 binder_loop() 类似，但在进入循环前先发送 BC_REGISTER_LOOPER
 * 而不是 BC_ENTER_LOOPER。BC_REGISTER_LOOPER 表示这是按需创建的工作线程。
 * 
 * @param bs binder 状态
 * @param func 事务处理回调函数
 */
void binder_thread_loop(struct binder_state *bs, binder_handler func)
{
    int res;
    struct binder_write_read bwr;
    uint32_t readbuf[32];

    bwr.write_size = 0;
    bwr.write_consumed = 0;
    bwr.write_buffer = 0;

    /* 向驱动注册为 Looper 线程（与 BC_ENTER_LOOPER 语义类似，都是声明准备接收事务） */
    readbuf[0] = BC_REGISTER_LOOPER;
    binder_write(bs, readbuf, sizeof(uint32_t));

    for (;;) {
        bwr.read_size = sizeof(readbuf);
        bwr.read_consumed = 0;
        bwr.read_buffer = (uintptr_t) readbuf;

        /* 通过 BINDER_WRITE_READ 等待驱动事件（阻塞在这里） */
        res = ioctl(bs->fd, BINDER_WRITE_READ, &bwr);

        if (res < 0) {
            ALOGE("binder_loop: ioctl failed (%s)\n", strerror(errno));
            break;
        }

        /* 解析驱动返回的命令序列 */
        res = binder_parse(bs, 0, (uintptr_t) readbuf, bwr.read_consumed, func);
        if (res == 0) {
            ALOGE("binder_loop: unexpected reply?!\n");
            break;
        }
        if (res < 0) {
            ALOGE("binder_loop: io error %d %s\n", res, strerror(errno));
            break;
        }
    }
}

/* 线程描述结构：用于传递 binder 状态和处理函数到新线程 */
struct binder_thread_desc {
    struct binder_state *bs;
	binder_handler func;
};

/**
 * @brief 新线程入口函数：包装 binder_thread_loop
 * @param btd 线程描述结构指针
 * @return 返回 NULL（线程正常退出时）
 */
static void * binder_thread_routine(struct binder_thread_desc *btd)
{
	binder_thread_loop(btd->bs, btd->func);
	return NULL;
}

/**
 * @brief 解析 binder 驱动返回的命令流
 * 
 * 遍历驱动返回的命令序列，按 cmd 类型分发处理：
 * - BR_TRANSACTION: 收到新的服务请求，调用处理函数
 * - BR_REPLY: 收到之前发起调用的回复，填充 bio
 * - BR_SPAWN_LOOPER: 驱动请求创建新线程
 * - BR_DEAD_BINDER: 被监听的 binder 对象已死亡
 * - BR_INCREFS/ACQUIRE/RELEASE/DECREFS: 引用计数相关命令
 * 
 * @param bs binder 状态
 * @param bio 用于接收 BR_REPLY 的 binder_io 指针（客户端调用时传入）
 * @param ptr 驱动返回数据的起始地址
 * @param size 数据总长度（字节）
 * @param func 服务端事务处理函数（服务端循环时传入）
 * @return 1 正常处理完成；0 成功收到回复；-1 发生错误
 */
int binder_parse(struct binder_state *bs, struct binder_io *bio,
                 uintptr_t ptr, size_t size, binder_handler func)
{
    int r = 1;                /* 默认返回 1 表示正常循环 */
    uintptr_t end = ptr + (uintptr_t) size; /* 计算数据末尾 */

    while (ptr < end) {
        uint32_t cmd = *(uint32_t *) ptr; /* 读取当前命令码 */
        ptr += sizeof(uint32_t);          /* 指针前移跳过命令码 */
#if TRACE
        fprintf(stderr,"%s:\n", cmd_name(cmd));
#endif
        switch(cmd) {
        case BR_NOOP:
            break;
        case BR_TRANSACTION_COMPLETE:
            break;
        case BR_INCREFS:
        case BR_ACQUIRE:
        case BR_RELEASE:
        case BR_DECREFS:
#if TRACE
            fprintf(stderr,"  %p, %p\n", (void *)ptr, (void *)(ptr + sizeof(void *)));
#endif
            ptr += sizeof(struct binder_ptr_cookie);
            break;
		case BR_SPAWN_LOOPER: {
			/* 创建新线程 */
			//if (fork() == 0) {
			//}
			pthread_t thread;
			struct binder_thread_desc btd;

			btd.bs = bs;
			btd.func = func;
			
			pthread_create(&thread, NULL, binder_thread_routine, &btd);

			/* 在新线程中：ioctl(BC_ENTER_LOOPER)，进入 binder_looper */

			break;
		}
        case BR_TRANSACTION: {
            struct binder_transaction_data *txn = (struct binder_transaction_data *) ptr;
            if ((end - ptr) < sizeof(*txn)) {
                ALOGE("parse: txn too small!\n");
                return -1;
            }
            binder_dump_txn(txn);
            if (func) {
                unsigned rdata[256/4];
                struct binder_io msg;
                struct binder_io reply;
                int res;

                bio_init(&reply, rdata, sizeof(rdata), 4);
                bio_init_from_txn(&msg, txn);
                res = func(bs, txn, &msg, &reply);
                binder_send_reply(bs, &reply, txn->data.ptr.buffer, res);
            }
            ptr += sizeof(*txn);
            break;
        }
        case BR_REPLY: {
            struct binder_transaction_data *txn = (struct binder_transaction_data *) ptr;
            if ((end - ptr) < sizeof(*txn)) {
                ALOGE("parse: reply too small!\n");
                return -1;
            }
            binder_dump_txn(txn);
            if (bio) {
                bio_init_from_txn(bio, txn);
                bio = 0;
            } else {
                /* 待办：释放缓冲区 */
            }
            ptr += sizeof(*txn);
            r = 0;
            break;
        }
        case BR_DEAD_BINDER: {
            struct binder_death *death = (struct binder_death *)(uintptr_t) *(binder_uintptr_t *)ptr;
            ptr += sizeof(binder_uintptr_t);
            death->func(bs, death->ptr);
            break;
        }
        case BR_FAILED_REPLY:
            r = -1;
            break;
        case BR_DEAD_REPLY:
            r = -1;
            break;
        default:
            ALOGE("parse: OOPS %d\n", cmd);
            return -1;
        }
    }

    return r;
}

/**
 * @brief 增加 binder 对象的强引用计数
 * @param bs binder 状态
 * @param target 目标 binder 句柄
 */
void binder_acquire(struct binder_state *bs, uint32_t target)
{
    uint32_t cmd[2];
    cmd[0] = BC_ACQUIRE;  /* 命令码：增加强引用 */
    cmd[1] = target;      /* 目标句柄 */
    binder_write(bs, cmd, sizeof(cmd));
}

/**
 * @brief 减少 binder 对象的强引用计数
 * @param bs binder 状态
 * @param target 目标 binder 句柄
 */
void binder_release(struct binder_state *bs, uint32_t target)
{
    uint32_t cmd[2];
    cmd[0] = BC_RELEASE;  /* 命令码：减少强引用 */
    cmd[1] = target;
    binder_write(bs, cmd, sizeof(cmd));
}

/**
 * @brief 注册 binder 对象的死亡通知回调
 * 
 * 当目标 binder 对象死亡时，驱动会发送 BR_DEAD_BINDER，
 * 然后调用 death->func(bs, death->ptr) 通知客户端
 * 
 * @param bs binder 状态
 * @param target 要监听的 binder 句柄
 * @param death 死亡通知结构，包含回调函数和用户数据
 */
void binder_link_to_death(struct binder_state *bs, uint32_t target, struct binder_death *death)
{
    struct {
        uint32_t cmd;
        struct binder_handle_cookie payload;
    } __attribute__((packed)) data;

    data.cmd = BC_REQUEST_DEATH_NOTIFICATION; /* 请求死亡通知 */
    data.payload.handle = target;              /* 要监听的句柄 */
    data.payload.cookie = (uintptr_t) death;   /* 死亡结构作为 cookie，回调时传回 */
    binder_write(bs, &data, sizeof(data));
}

/**
 * @brief 发起一次阻塞式 binder 调用
 * 
 * 客户端通过此函数向服务端发起 IPC 请求：
 * 1. 将 msg 中的数据打包成 BC_TRANSACTION 命令
 * 2. 阻塞等待服务端处理并回复（通过 BINDER_WRITE_READ 循环）
 * 3. 收到回复后将数据解析到 reply 中
 * 
 * @param bs binder 状态
 * @param msg 请求数据（客户端填充好参数）
 * @param reply 回复数据（成功调用后从服务端获取）
 * @param target 目标服务的 binder 句柄
 * @param code 调用方法的事务码（如 SVC_MGR_GET_SERVICE）
 * @return 成功返回 0，失败返回 -1（reply->flags 会标记 BIO_F_IOERROR）
 */
int binder_call(struct binder_state *bs,
                struct binder_io *msg, struct binder_io *reply,
                uint32_t target, uint32_t code)
{
    int res;
    struct binder_write_read bwr;
    struct {
        uint32_t cmd;
        struct binder_transaction_data txn;
    } __attribute__((packed)) writebuf;
    unsigned readbuf[32];

    if (msg->flags & BIO_F_OVERFLOW) {
        fprintf(stderr,"binder: txn buffer overflow\n");
        goto fail;
    }

    writebuf.cmd = BC_TRANSACTION;
    writebuf.txn.target.handle = target;
    writebuf.txn.code = code;
    writebuf.txn.flags = 0;
    writebuf.txn.data_size = msg->data - msg->data0;
    writebuf.txn.offsets_size = ((char*) msg->offs) - ((char*) msg->offs0);
    writebuf.txn.data.ptr.buffer = (uintptr_t)msg->data0;
    writebuf.txn.data.ptr.offsets = (uintptr_t)msg->offs0;

    bwr.write_size = sizeof(writebuf);
    bwr.write_consumed = 0;
    bwr.write_buffer = (uintptr_t) &writebuf;

    hexdump(msg->data0, msg->data - msg->data0);
    for (;;) {
        bwr.read_size = sizeof(readbuf);
        bwr.read_consumed = 0;
        bwr.read_buffer = (uintptr_t) readbuf;

        res = ioctl(bs->fd, BINDER_WRITE_READ, &bwr);

        if (res < 0) {
            fprintf(stderr,"binder: ioctl failed (%s)\n", strerror(errno));
            goto fail;
        }

        res = binder_parse(bs, reply, (uintptr_t) readbuf, bwr.read_consumed, 0);
        if (res == 0) return 0;
        if (res < 0) goto fail;
    }

fail:
    memset(reply, 0, sizeof(*reply));
    reply->flags |= BIO_F_IOERROR;
    return -1;
}

/**
 * @brief 设置 binder 驱动允许的最大线程数
 * @param bs binder 状态
 * @param threads 最大线程数（驱动按需 spawn 新线程时的上限）
 */
void binder_set_maxthreads(struct binder_state *bs, int threads)
{
	ioctl(bs->fd, BINDER_SET_MAX_THREADS, &threads);
}

/**
 * @brief 主 binder 线程循环（服务端主事件循环）
 * 
 * 与 binder_thread_loop() 的区别：先发送 BC_ENTER_LOOPER 声明主线程身份。
 * 绝大多数服务端程序在 main() 末尾调用此函数，随后阻塞在事件循环中。
 * 
 * @param bs binder 状态
 * @param func 事务处理回调函数
 */
void binder_loop(struct binder_state *bs, binder_handler func)
{
    int res;
    struct binder_write_read bwr;
    uint32_t readbuf[32];

    bwr.write_size = 0;
    bwr.write_consumed = 0;
    bwr.write_buffer = 0;

    readbuf[0] = BC_ENTER_LOOPER;
    binder_write(bs, readbuf, sizeof(uint32_t));

    for (;;) {
        bwr.read_size = sizeof(readbuf);
        bwr.read_consumed = 0;
        bwr.read_buffer = (uintptr_t) readbuf;

        res = ioctl(bs->fd, BINDER_WRITE_READ, &bwr);

        if (res < 0) {
            ALOGE("binder_loop: ioctl failed (%s)\n", strerror(errno));
            break;
        }

        res = binder_parse(bs, 0, (uintptr_t) readbuf, bwr.read_consumed, func);
        if (res == 0) {
            ALOGE("binder_loop: unexpected reply?!\n");
            break;
        }
        if (res < 0) {
            ALOGE("binder_loop: io error %d %s\n", res, strerror(errno));
            break;
        }
    }
}

/**
 * @brief 从 binder_transaction_data 初始化 binder_io 结构（接收侧）
 * 
 * 将驱动返回的事务数据包装成可解析的 binder_io。
 * 此时 data 指向驱动共享缓冲区，flags 设为 BIO_F_SHARED。
 * 
 * @param bio 待初始化的 binder_io
 * @param txn 驱动返回的事务数据
 */
void bio_init_from_txn(struct binder_io *bio, struct binder_transaction_data *txn)
{
    bio->data = bio->data0 = (char *)(intptr_t)txn->data.ptr.buffer;
    bio->offs = bio->offs0 = (binder_size_t *)(intptr_t)txn->data.ptr.offsets;
    bio->data_avail = txn->data_size;
    bio->offs_avail = txn->offsets_size / sizeof(size_t);
    bio->flags = BIO_F_SHARED;
}

/**
 * @brief 初始化 binder_io 结构（发送侧）
 * 
 * 在调用者提供的工作缓冲区上建立 binder_io：
 * - 前半部分预留为对象偏移数组（offs 区）
 * - 后半部分用于存放实际数据（data 区）
 * 
 * @param bio 待初始化的 binder_io
 * @param data 调用者提供的工作缓冲区
 * @param maxdata 工作缓冲区总大小（字节）
 * @param maxoffs 预留的偏移条目数量
 */
void bio_init(struct binder_io *bio, void *data,
              size_t maxdata, size_t maxoffs)
{
    size_t n = maxoffs * sizeof(size_t);

    if (n > maxdata) {
        bio->flags = BIO_F_OVERFLOW;
        bio->data_avail = 0;
        bio->offs_avail = 0;
        return;
    }

    bio->data = bio->data0 = (char *) data + n;
    bio->offs = bio->offs0 = data;
    bio->data_avail = maxdata - n;
    bio->offs_avail = maxoffs;
    bio->flags = 0;
}

/**
 * @brief 从 binder_io 的 data 区分配一段内存（内部函数）
 * 
 * @param bio binder_io 结构
 * @param size 需要分配的大小（字节，会向上对齐到4字节）
 * @return 成功返回分配的指针，空间不足返回 NULL 并设置 BIO_F_OVERFLOW
 */
static void *bio_alloc(struct binder_io *bio, size_t size)
{
    size = (size + 3) & (~3);
    if (size > bio->data_avail) {
        bio->flags |= BIO_F_OVERFLOW;
        return NULL;
    } else {
        void *ptr = bio->data;
        bio->data += size;
        bio->data_avail -= size;
        return ptr;
    }
}

/**
 * @brief 完成 binder_call 后的清理工作
 * 
 * 当 reply 来自驱动共享缓冲区时（BIO_F_SHARED），
 * 通过 BC_FREE_BUFFER 命令释放该缓冲区。
 * 
 * @param bs binder 状态
 * @param msg 请求侧的 binder_io（目前未使用）
 * @param reply 回复侧的 binder_io（可能包含需要释放的缓冲区）
 */
void binder_done(struct binder_state *bs,
                 struct binder_io *msg,
                 struct binder_io *reply)
{
    struct {
        uint32_t cmd;
        uintptr_t buffer;
    } __attribute__((packed)) data;

    if (reply->flags & BIO_F_SHARED) {
        data.cmd = BC_FREE_BUFFER;
        data.buffer = (uintptr_t) reply->data0;
        binder_write(bs, &data, sizeof(data));
        reply->flags = 0;
    }
}

/**
 * @brief 在 binder_io 的 offs 区分配一个 flat_binder_object
 * 
 * 除了分配对象内存外，还会在偏移数组中记录对象相对于 data0 的偏移
 * （便于接收侧定位对象）。
 * 
 * @param bio binder_io 结构
 * @return 成功返回 flat_binder_object 指针，失败返回 NULL
 */
static struct flat_binder_object *bio_alloc_obj(struct binder_io *bio)
{
    struct flat_binder_object *obj;

    obj = bio_alloc(bio, sizeof(*obj));

    if (obj && bio->offs_avail) {
        bio->offs_avail--;
        *bio->offs++ = ((char*) obj) - ((char*) bio->data0);
        return obj;
    }

    bio->flags |= BIO_F_OVERFLOW;
    return NULL;
}

/**
 * @brief 向 binder_io 中写入一个 uint32 值
 * @param bio binder_io 结构
 * @param n 要写入的值
 */
void bio_put_uint32(struct binder_io *bio, uint32_t n)
{
    uint32_t *ptr = bio_alloc(bio, sizeof(n));
    if (ptr)
        *ptr = n;
}

/**
 * @brief 向 binder_io 中写入一个 binder 对象（服务端发布服务时使用）
 * 
 * 将本地函数指针包装成 BINDER_TYPE_BINDER 类型的 flat_binder_object，
 * 对端收到后可作为函数指针调用。
 * 
 * @param bio binder_io 结构
 * @param ptr 要发布的 binder 对象（通常是处理函数指针）
 */
void bio_put_obj(struct binder_io *bio, void *ptr)
{
    struct flat_binder_object *obj;

    obj = bio_alloc_obj(bio);
    if (!obj)
        return;

    obj->flags = 0x7f | FLAT_BINDER_FLAG_ACCEPTS_FDS;
    obj->type = BINDER_TYPE_BINDER;
    obj->binder = (uintptr_t)ptr;
    obj->cookie = 0;
}

/**
 * @brief 向 binder_io 中写入一个 binder 引用（客户端获取服务时使用）
 * 
 * 将服务端返回的 handle 包装成 BINDER_TYPE_HANDLE 类型的 flat_binder_object。
 * 如果 handle 为 0，则不创建偏移条目（占位用）。
 * 
 * @param bio binder_io 结构
 * @param handle binder 句柄（服务端的唯一标识）
 */
void bio_put_ref(struct binder_io *bio, uint32_t handle)
{
    struct flat_binder_object *obj;

    if (handle)
        obj = bio_alloc_obj(bio);
    else
        obj = bio_alloc(bio, sizeof(*obj));

    if (!obj)
        return;

    obj->flags = 0x7f | FLAT_BINDER_FLAG_ACCEPTS_FDS;
    obj->type = BINDER_TYPE_HANDLE;
    obj->handle = handle;
    obj->cookie = 0;
}

/**
 * @brief 向 binder_io 中写入一个 UTF-16 字符串
 * 
 * 格式：先写入 32 位长度，再写入字符串数据（含 null 终止符）。
 * NULL 字符串会特殊编码为 0xffffffff。
 * 
 * @param bio binder_io 结构
 * @param str 要写入的 UTF-16 字符串（以 null 结尾）
 */
void bio_put_string16(struct binder_io *bio, const uint16_t *str)
{
    size_t len;
    uint16_t *ptr;

    if (!str) {
        bio_put_uint32(bio, 0xffffffff);
        return;
    }

    len = 0;
    while (str[len]) len++;

    if (len >= (MAX_BIO_SIZE / sizeof(uint16_t))) {
        bio_put_uint32(bio, 0xffffffff);
        return;
    }

    /* 注意：载荷将使用 32 位大小而非 size_t */
    bio_put_uint32(bio, (uint32_t) len);
    len = (len + 1) * sizeof(uint16_t);
    ptr = bio_alloc(bio, len);
    if (ptr)
        memcpy(ptr, str, len);
}

/**
 * @brief 向 binder_io 中写入一个 char* 字符串（自动转换为 UTF-16）
 * 
 * 格式与 bio_put_string16 相同：先写入 32 位长度，再写入 UTF-16 数据。
 * 这是最常用的字符串写入便捷函数，内部会将 ASCII/Latin-1 字符串
 * 逐字节扩展为 UTF-16。
 * 
 * @param bio binder_io 结构
 * @param _str 要写入的 char* 字符串（以 null 结尾）
 */
void bio_put_string16_x(struct binder_io *bio, const char *_str)
{
    unsigned char *str = (unsigned char*) _str;
    size_t len;
    uint16_t *ptr;

    if (!str) {
        bio_put_uint32(bio, 0xffffffff);
        return;
    }

    len = strlen(_str);

    if (len >= (MAX_BIO_SIZE / sizeof(uint16_t))) {
        bio_put_uint32(bio, 0xffffffff);
        return;
    }

    /* 注意：载荷将使用 32 位大小而非 size_t */
    bio_put_uint32(bio, len);
    ptr = bio_alloc(bio, (len + 1) * sizeof(uint16_t));
    if (!ptr)
        return;

    while (*str)
        *ptr++ = *str++;
    *ptr++ = 0;
}

/**
 * @brief 从 binder_io 的 data 区读取一段内存并前移指针（内部函数）
 * 
 * @param bio binder_io 结构
 * @param size 需要读取的大小（字节，会向上对齐到4字节）
 * @return 成功返回数据指针，空间不足返回 NULL 并设置 BIO_F_OVERFLOW
 */
static void *bio_get(struct binder_io *bio, size_t size)
{
    size = (size + 3) & (~3);

    if (bio->data_avail < size){
        bio->data_avail = 0;
        bio->flags |= BIO_F_OVERFLOW;
        return NULL;
    }  else {
        void *ptr = bio->data;
        bio->data += size;
        bio->data_avail -= size;
        return ptr;
    }
}

/**
 * @brief 从 binder_io 中读取一个 uint32 值
 * @param bio binder_io 结构
 * @return 读取的值，空间不足返回 0
 */
uint32_t bio_get_uint32(struct binder_io *bio)
{
    uint32_t *ptr = bio_get(bio, sizeof(*ptr));
    return ptr ? *ptr : 0;
}

/**
 * @brief 从 binder_io 中读取一个 UTF-16 字符串
 * 
 * 格式匹配 bio_put_string16：先读 32 位长度，再读字符串数据。
 * 读到 NULL 时（0xffffffff）返回 NULL。
 * 
 * @param bio binder_io 结构
 * @param sz 输出参数，返回字符串长度（不含 null 终止符），可传 NULL
 * @return 成功返回 UTF-16 字符串指针，失败返回 NULL
 */
uint16_t *bio_get_string16(struct binder_io *bio, size_t *sz)
{
    size_t len;

    /* 注意：载荷将使用 32 位大小而非 size_t */
    len = (size_t) bio_get_uint32(bio);
    if (sz)
        *sz = len;
    return bio_get(bio, (len + 1) * sizeof(uint16_t));
}

/**
 * @brief 在 binder_io 中查找下一个 flat_binder_object（内部函数）
 * 
 * binder_io 中对象的存放方式：先在 data 区顺序存入对象引用位置，
 * 再在 offs 数组中记录每个对象相对于 data0 的偏移。
 * 此函数通过比较偏移值来定位下一个对象。
 * 
 * @param bio binder_io 结构
 * @return 成功返回 flat_binder_object 指针，失败返回 NULL
 */
static struct flat_binder_object *_bio_get_obj(struct binder_io *bio)
{
    size_t n;
    size_t off = bio->data - bio->data0;

    /* 待办：能否在这里做得更智能一些？ */
    for (n = 0; n < bio->offs_avail; n++) {
        if (bio->offs[n] == off)
            return bio_get(bio, sizeof(struct flat_binder_object));
    }

    bio->data_avail = 0;
    bio->flags |= BIO_F_OVERFLOW;
    return NULL;
}

/**
 * @brief 从 binder_io 中读取一个 binder 引用（服务端句柄）
 * 
 * 这是 bio_put_ref 的逆操作：从回复中解析出服务端返回的 handle。
 * 
 * @param bio binder_io 结构
 * @return 成功返回 handle 值，失败返回 0
 */
uint32_t bio_get_ref(struct binder_io *bio)
{
    struct flat_binder_object *obj;

    obj = _bio_get_obj(bio);
    if (!obj)
        return 0;

    if (obj->type == BINDER_TYPE_HANDLE)
        return obj->handle;

    return 0;
}
