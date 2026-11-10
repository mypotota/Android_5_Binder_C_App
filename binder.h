/* Copyright 2008 The Android Open Source Project
 */

#ifndef _BINDER_H_
#define _BINDER_H_

#include <sys/ioctl.h>
#include <linux/binder.h>

struct binder_state;

struct binder_io
{
    char *data;            /* 用于读/写的指针 */
    binder_size_t *offs;   /* 偏移量数组 */
    size_t data_avail;     /* 数据缓冲区中的可用字节数 */
    size_t offs_avail;     /* 偏移量数组中的可用条目数 */

    char *data0;           /* 数据缓冲区起始位置 */
    binder_size_t *offs0;  /* 偏移量缓冲区起始位置 */
    uint32_t flags;
    uint32_t unused;
};

struct binder_death {
    void (*func)(struct binder_state *bs, void *ptr);
    void *ptr;
};

/* 一个神奇的句柄 */
#define BINDER_SERVICE_MANAGER  0U

#define SVC_MGR_NAME "android.os.IServiceManager"

enum {
    /* 必须与 IBinder.h 和 IServiceManager.h 中的定义匹配 */
    PING_TRANSACTION  = B_PACK_CHARS('_','P','N','G'),
    SVC_MGR_GET_SERVICE = 1,
    SVC_MGR_CHECK_SERVICE,
    SVC_MGR_ADD_SERVICE,
    SVC_MGR_LIST_SERVICES,
};

typedef int (*binder_handler)(struct binder_state *bs,
                              struct binder_transaction_data *txn,
                              struct binder_io *msg,
                              struct binder_io *reply);

struct binder_state *binder_open(size_t mapsize);
void binder_close(struct binder_state *bs);

/* 发起阻塞式 binder 调用
 * - 成功返回零
 */
int binder_call(struct binder_state *bs,
                struct binder_io *msg, struct binder_io *reply,
                uint32_t target, uint32_t code);

/* 释放与 binder_io 关联的任何状态
 * - 在 binder_call() 返回后，一旦从 binder_io 中提取了所有必要数据后调用
 * - 即使 binder_call() 失败也可以安全调用
 */
void binder_done(struct binder_state *bs,
                 struct binder_io *msg, struct binder_io *reply);

/* 操作强引用 */
void binder_acquire(struct binder_state *bs, uint32_t target);
void binder_release(struct binder_state *bs, uint32_t target);

void binder_link_to_death(struct binder_state *bs, uint32_t target, struct binder_death *death);

void binder_loop(struct binder_state *bs, binder_handler func);

int binder_become_context_manager(struct binder_state *bs);

/* 分配一个 binder_io，提供栈分配的工作缓冲区、工作缓冲区大小，以及要从缓冲区预留多少个对象偏移条目 */
void bio_init(struct binder_io *bio, void *data,
           size_t maxdata, size_t maxobjects);

void bio_put_obj(struct binder_io *bio, void *ptr);
void bio_put_ref(struct binder_io *bio, uint32_t handle);
void bio_put_uint32(struct binder_io *bio, uint32_t n);
void bio_put_string16(struct binder_io *bio, const uint16_t *str);
void bio_put_string16_x(struct binder_io *bio, const char *_str);

uint32_t bio_get_uint32(struct binder_io *bio);
uint16_t *bio_get_string16(struct binder_io *bio, size_t *sz);
uint32_t bio_get_ref(struct binder_io *bio);

void binder_set_maxthreads(struct binder_state *bs, int threads);

#endif
