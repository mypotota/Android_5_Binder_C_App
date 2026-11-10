/* Copyright 2008 The Android Open Source Project
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/types.h>
#include<stdbool.h>

#include <private/android_filesystem_config.h>

//#include <selinux/android.h>
//#include <selinux/avc.h>

#include "binder.h"

#if 1
#define ALOGI(x...) fprintf(stderr, "svcmgr: " x)
#define ALOGE(x...) fprintf(stderr, "svcmgr: " x)
#else
#define LOG_TAG "ServiceManager"
#include <cutils/log.h>
#endif

/* 全局变量：Service Manager 的固定句柄（始终为 0） */
uint32_t svcmgr_handle;

/**
 * @brief 将 UTF-16 字符串转换为 char*（调试输出用）
 * 
 * 使用静态缓冲区存储结果，非线程安全，仅用于日志打印。
 * 
 * @param x UTF-16 字符串指针
 * @param x_len 字符串长度（UTF-16 元素个数）
 * @return 转换后的 char* 字符串指针
 */
const char *str8(const uint16_t *x, size_t x_len)
{
    static char buf[128];
    size_t max = 127;
    char *p = buf;

    if (x_len < max) {
        max = x_len;
    }

    if (x) {
        while ((max > 0) && (*x != '\0')) {
            *p++ = *x++;
            max--;
        }
    }
    *p++ = 0;
    return buf;
}

/**
 * @brief 比较 UTF-16 字符串与 char* 字符串是否相等
 * @param a UTF-16 字符串
 * @param b char* 字符串
 * @return 相等返回 1，否则返回 0
 */
int str16eq(const uint16_t *a, const char *b)
{
    while (*a && *b)
        if (*a++ != *b++) return 0;
    if (*a || *b)
        return 0;
    return 1;
}

/* ============================================================
 * SELinux 权限检查（本项目中全部返回 true 跳过权限检查）
 * ============================================================ */
static char *service_manager_context;

/**
 * @brief 检查调用者是否有权限执行某操作
 * @param spid 调用者进程 PID
 * @param tctx 目标安全上下文
 * @param perm 权限名称
 * @param name 服务名称
 * @return 有权限返回 true
 */
static bool check_mac_perms(pid_t spid, const char *tctx, const char *perm, const char *name)
{
    return true;
}

/**
 * @brief 通过 getcon 获取上下文后检查权限
 */
static bool check_mac_perms_from_getcon(pid_t spid, const char *perm)
{
    return true;
}

/**
 * @brief 通过 lookup 获取上下文后检查权限
 */
static bool check_mac_perms_from_lookup(pid_t spid, const char *perm, const char *name)
{
    return true;
}

/** @brief 检查调用者是否可注册服务（perm="add"） */
static int svc_can_register(const uint16_t *name, size_t name_len, pid_t spid)
{
    const char *perm = "add";
    return check_mac_perms_from_lookup(spid, perm, str8(name, name_len)) ? 1 : 0;
}

/** @brief 检查调用者是否可列出服务（perm="list"） */
static int svc_can_list(pid_t spid)
{
    const char *perm = "list";
    return check_mac_perms_from_getcon(spid, perm) ? 1 : 0;
}

/** @brief 检查调用者是否可查找服务（perm="find"） */
static int svc_can_find(const uint16_t *name, size_t name_len, pid_t spid)
{
    const char *perm = "find";
    return check_mac_perms_from_lookup(spid, perm, str8(name, name_len)) ? 1 : 0;
}

/* ============================================================
 * 服务链表管理
 * ============================================================ */

/**
 * @brief 服务信息结构：保存一个已注册服务的完整信息
 * 
 * 使用零长度数组作为服务名存储（柔性数组），
 * 通过 malloc(sizeof(*si) + (len+1)*sizeof(uint16_t)) 一次分配。
 */
struct svcinfo
{
    struct svcinfo *next;   /* 链表下一个节点 */
    uint32_t handle;        /* Binder 句柄 */
    struct binder_death death; /* 死亡通知（服务死亡时自动清理） */
    int allow_isolated;     /* 是否允许隔离进程访问 */
    size_t len;             /* 服务名长度 */
    uint16_t name[0];       /* 服务名（UTF-16，柔性数组） */
};

struct svcinfo *svclist = NULL; /* 服务链表头指针 */

/**
 * @brief 在服务链表中按名称查找服务
 * @param s16 服务名（UTF-16）
 * @param len 服务名长度
 * @return 找到返回 svcinfo 指针，未找到返回 NULL
 */
struct svcinfo *find_svc(const uint16_t *s16, size_t len)
{
    struct svcinfo *si;

    for (si = svclist; si; si = si->next) {
        if ((len == si->len) &&
            !memcmp(s16, si->name, len * sizeof(uint16_t))) {
            return si;
        }
    }
    return NULL;
}

/**
 * @brief 服务死亡回调函数：释放句柄并记录日志
 * 
 * 当服务端进程死亡时，驱动触发 BR_DEAD_BINDER，
 * 此处释放 handle 以便服务可被重新注册。
 * 
 * @param bs binder 状态
 * @param ptr 指向 svcinfo 结构的 cookie 指针
 */
void svcinfo_death(struct binder_state *bs, void *ptr)
{
    struct svcinfo *si = (struct svcinfo* ) ptr;

    ALOGI("service '%s' died\n", str8(si->name, si->len));
    if (si->handle) {
        binder_release(bs, si->handle);
        si->handle = 0;
    }
}

/* Service Manager 的接口 ID（android.os.IServiceManager 的 UTF-16 编码） */
uint16_t svcmgr_id[] = {
    'a','n','d','r','o','i','d','.','o','s','.',
    'I','S','e','r','v','i','c','e','M','a','n','a','g','e','r'
};


/**
 * @brief 实现 SVC_MGR_GET_SERVICE / SVC_MGR_CHECK_SERVICE 命令
 * 
 * 检查调用者权限、查找服务、验证隔离进程访问控制后返回句柄。
 * 
 * @param bs binder 状态
 * @param s 服务名（UTF-16）
 * @param len 服务名长度
 * @param uid 调用者 uid
 * @param spid 调用者 pid
 * @return 成功返回服务句柄，失败返回 0
 */
uint32_t do_find_service(struct binder_state *bs, const uint16_t *s, size_t len, uid_t uid, pid_t spid)
{
    struct svcinfo *si;

    if (!svc_can_find(s, len, spid)) {
        ALOGE("find_service('%s') uid=%d - PERMISSION DENIED\n",
             str8(s, len), uid);
        return 0;
    }
    si = find_svc(s, len);
    //ALOGI("check_service('%s') handle = %x\n", str8(s, len), si ? si->handle : 0);
    if (si && si->handle) {
        if (!si->allow_isolated) {
            // 如果此服务不允许隔离进程访问，则检查 uid 是否为隔离进程
            uid_t appid = uid % AID_USER;
            if (appid >= AID_ISOLATED_START && appid <= AID_ISOLATED_END) {
                return 0;
            }
        }
        return si->handle;
    } else {
        return 0;
    }
}

/**
 * @brief 实现 SVC_MGR_ADD_SERVICE 命令：注册新服务
 * 
 * 1. 权限检查（spid 是否有 add 权限）
 * 2. 如果同名服务已存在则先调用 svcinfo_death 释放旧 handle
 * 3. 分配新的 svcinfo 节点并插入链表头部
 * 4. 通过 binder_acquire 增加引用计数
 * 5. 通过 binder_link_to_death 注册死亡通知
 * 
 * @param bs binder 状态
 * @param s 服务名（UTF-16）
 * @param len 服务名长度
 * @param handle 服务的 binder 句柄
 * @param uid 调用者 uid
 * @param allow_isolated 是否允许隔离进程访问
 * @param spid 调用者 pid
 * @return 成功返回 0，失败返回 -1
 */
int do_add_service(struct binder_state *bs,
                   const uint16_t *s, size_t len,
                   uint32_t handle, uid_t uid, int allow_isolated,
                   pid_t spid)
{
    struct svcinfo *si;

    //ALOGI("add_service('%s',%x,%s) uid=%d\n", str8(s, len), handle,
    //        allow_isolated ? "allow_isolated" : "!allow_isolated", uid);

    if (!handle || (len == 0) || (len > 127))
        return -1;

    if (!svc_can_register(s, len, spid)) {
        ALOGE("add_service('%s',%x) uid=%d - PERMISSION DENIED\n",
             str8(s, len), handle, uid);
        return -1;
    }

    si = find_svc(s, len);
    if (si) {
        if (si->handle) {
            ALOGE("add_service('%s',%x) uid=%d - ALREADY REGISTERED, OVERRIDE\n",
                 str8(s, len), handle, uid);
            svcinfo_death(bs, si);
        }
        si->handle = handle;
    } else {
        si = malloc(sizeof(*si) + (len + 1) * sizeof(uint16_t));
        if (!si) {
            ALOGE("add_service('%s',%x) uid=%d - OUT OF MEMORY\n",
                 str8(s, len), handle, uid);
            return -1;
        }
        si->handle = handle;
        si->len = len;
        memcpy(si->name, s, (len + 1) * sizeof(uint16_t));
        si->name[len] = '\0';
        si->death.func = (void*) svcinfo_death;
        si->death.ptr = si;
        si->allow_isolated = allow_isolated;
        si->next = svclist;
        svclist = si;
    }

    ALOGI("add_service('%s'), handle = %d\n", str8(s, len), handle);

    binder_acquire(bs, handle);
    binder_link_to_death(bs, handle, &si->death);
    return 0;
}

/**
 * @brief Service Manager 的主事务处理函数
 * 
 * 根据 txn->code 分发到不同命令的处理逻辑：
 * - PING_TRANSACTION: 存活检测，直接返回 0
 * - SVC_MGR_GET_SERVICE / SVC_MGR_CHECK_SERVICE: do_find_service()
 * - SVC_MGR_ADD_SERVICE: do_add_service()
 * - SVC_MGR_LIST_SERVICES: 遍历链表返回第 n 个服务名
 * 
 * @param bs binder 状态
 * @param txn 驱动传入的事务数据
 * @param msg 包含请求参数的 binder_io
 * @param reply 用于填充回复的 binder_io
 * @return 成功返回 0，失败返回 -1
 */
int svcmgr_handler(struct binder_state *bs,
                   struct binder_transaction_data *txn,
                   struct binder_io *msg,
                   struct binder_io *reply)
{
    struct svcinfo *si;
    uint16_t *s;
    size_t len;
    uint32_t handle;
    uint32_t strict_policy;
    int allow_isolated;

    //ALOGI("target=%x code=%d pid=%d uid=%d\n",
    //  txn->target.handle, txn->code, txn->sender_pid, txn->sender_euid);

    if (txn->target.handle != svcmgr_handle)
        return -1;

    if (txn->code == PING_TRANSACTION)
        return 0;

    // 等同于 Parcel::enforceInterface()，读取带有严格模式策略掩码和接口名称的 RPC 头。
    // 注意：我们忽略 strict_policy 且不进一步传播它（因为我们不执行任何出站 RPC）。
    strict_policy = bio_get_uint32(msg);
    s = bio_get_string16(msg, &len);
    if (s == NULL) {
        return -1;
    }

    if ((len != (sizeof(svcmgr_id) / 2)) ||
        memcmp(svcmgr_id, s, sizeof(svcmgr_id))) {
        fprintf(stderr,"invalid id %s\n", str8(s, len));
        return -1;
    }


    switch(txn->code) {
    case SVC_MGR_GET_SERVICE:
    case SVC_MGR_CHECK_SERVICE:
        s = bio_get_string16(msg, &len);
        if (s == NULL) {
            return -1;
        }
        handle = do_find_service(bs, s, len, txn->sender_euid, txn->sender_pid);
        if (!handle)
            break;
        bio_put_ref(reply, handle);
        return 0;

    case SVC_MGR_ADD_SERVICE:
        s = bio_get_string16(msg, &len);
        if (s == NULL) {
            return -1;
        }
        handle = bio_get_ref(msg);
        allow_isolated = bio_get_uint32(msg) ? 1 : 0;
        if (do_add_service(bs, s, len, handle, txn->sender_euid,
            allow_isolated, txn->sender_pid))
            return -1;
        break;

    case SVC_MGR_LIST_SERVICES: {
        uint32_t n = bio_get_uint32(msg);

        if (!svc_can_list(txn->sender_pid)) {
            ALOGE("list_service() uid=%d - PERMISSION DENIED\n",
                    txn->sender_euid);
            return -1;
        }
        si = svclist;
        while ((n-- > 0) && si)
            si = si->next;
        if (si) {
            bio_put_string16(reply, si->name);
            return 0;
        }
        return -1;
    }
    default:
        ALOGE("unknown code %d\n", txn->code);
        return -1;
    }

    bio_put_uint32(reply, 0);
    return 0;
}



/**
 * @brief Service Manager 主函数
 * 
 * 1. 打开 binder 驱动
 * 2. 声明自己为 Context Manager（整个系统只能有一个）
 * 3. 进入 binder 主循环，等待并处理客户端请求
 */
int main(int argc, char **argv)
{
    struct binder_state *bs;

    bs = binder_open(128*1024);
    if (!bs) {
        ALOGE("failed to open binder driver\n");
        return -1;
    }

    if (binder_become_context_manager(bs)) {
        ALOGE("cannot become context manager (%s)\n", strerror(errno));
        return -1;
    }


    svcmgr_handle = BINDER_SERVICE_MANAGER;
    binder_loop(bs, svcmgr_handler);

    return 0;
}
