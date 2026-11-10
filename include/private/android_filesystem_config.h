/*
 * Copyright (C) 2007 The Android Open Source Project
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

/* 此文件用于定义由构建工具（mkbootfs 和 mkyaffs2image）及 adb 设备端生成的文件系统镜像的属性 */

#ifndef _ANDROID_FILESYSTEM_CONFIG_H_
#define _ANDROID_FILESYSTEM_CONFIG_H_

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>

#ifdef HAVE_ANDROID_OS
#include <linux/capability.h>
#else
#include "android_filesystem_capability.h"
#endif

/* 这是平台的主用户和用户组配置。
 * 严禁重新编号
 */

#define AID_ROOT             0  /* 传统 unix root 用户 */

#define AID_SYSTEM        1000  /* system server */

#define AID_RADIO         1001  /* 电话子系统, RIL */
#define AID_BLUETOOTH     1002  /* 蓝牙子系统 */
#define AID_GRAPHICS      1003  /* 图形设备 */
#define AID_INPUT         1004  /* 输入设备 */
#define AID_AUDIO         1005  /* 音频设备 */
#define AID_CAMERA        1006  /* 相机设备 */
#define AID_LOG           1007  /* 日志设备 */
#define AID_COMPASS       1008  /* 罗盘设备 */
#define AID_MOUNT         1009  /* mountd 套接字 */
#define AID_WIFI          1010  /* wifi 子系统 */
#define AID_ADB           1011  /* android debug bridge (adbd) */
#define AID_INSTALL       1012  /* 安装包用户组 */
#define AID_MEDIA         1013  /* mediaserver 进程 */
#define AID_DHCP          1014  /* dhcp 客户端 */
#define AID_SDCARD_RW     1015  /* 外部存储写权限 */
#define AID_VPN           1016  /* vpn 系统 */
#define AID_KEYSTORE      1017  /* keystore 子系统 */
#define AID_USB           1018  /* USB 设备 */
#define AID_DRM           1019  /* DRM 服务器 */
#define AID_MDNSR         1020  /* 组播 DNS 响应器 (服务发现) */
#define AID_GPS           1021  /* GPS 守护进程 */
#define AID_UNUSED1       1022  /* 已弃用，请勿使用 */
#define AID_MEDIA_RW      1023  /* 内部媒体存储写权限 */
#define AID_MTP           1024  /* MTP USB 驱动访问 */
#define AID_UNUSED2       1025  /* 已弃用，请勿使用 */
#define AID_DRMRPC        1026  /* drm rpc 用户组 */
#define AID_NFC           1027  /* nfc 子系统 */
#define AID_SDCARD_R      1028  /* 外部存储读权限 */
#define AID_CLAT          1029  /* nat464 中的 clat 部分 */
#define AID_LOOP_RADIO    1030  /* loop radio 设备 */
#define AID_MEDIA_DRM     1031  /* MediaDrm 插件 */
#define AID_PACKAGE_INFO  1032  /* 访问已安装包详情 */
#define AID_SDCARD_PICS   1033  /* 外部存储图片访问 */
#define AID_SDCARD_AV     1034  /* 外部存储音视频访问 */
#define AID_SDCARD_ALL    1035  /* 访问所有用户的外部存储 */
#define AID_LOGD          1036  /* logd 守护进程 */
#define AID_SHARED_RELRO  1037  /* 共享 GNU RELRO 文件的创建者 */

#define AID_SHELL         2000  /* adb 和 debug shell 用户 */
#define AID_CACHE         2001  /* 缓存访问 */
#define AID_DIAG          2002  /* 诊断资源访问 */

/* 3000 系列仅用于补充用户组 ID。
 * 它们表示内核已知的 Android 特殊能力。 */
#define AID_NET_BT_ADMIN  3001  /* 蓝牙：可以创建任意套接字 */
#define AID_NET_BT        3002  /* 蓝牙：可以创建 sco、rfcomm 或 l2cap 套接字 */
#define AID_INET          3003  /* 可以创建 AF_INET 和 AF_INET6 套接字 */
#define AID_NET_RAW       3004  /* 可以创建原始 INET 套接字 */
#define AID_NET_ADMIN     3005  /* 可以配置网络接口和路由表。 */
#define AID_NET_BW_STATS  3006  /* 读取带宽统计 */
#define AID_NET_BW_ACCT   3007  /* 更改带宽统计记账 */
#define AID_NET_BT_STACK  3008  /* 蓝牙：访问配置文件 */

#define AID_EVERYBODY     9997  /* 同一配置文件中所有应用共享 */
#define AID_MISC          9998  /* 杂项存储访问 */
#define AID_NOBODY        9999

#define AID_APP          10000  /* 第一个应用用户 */

#define AID_ISOLATED_START 99000 /* 完全隔离沙箱进程的 uid 起始 */
#define AID_ISOLATED_END   99999 /* 完全隔离沙箱进程的 uid 结束 */

#define AID_USER        100000  /* 每个用户 uid 范围的偏移量 */

#define AID_SHARED_GID_START 50000 /* 每个用户应用共享的 gid 起始 */
#define AID_SHARED_GID_END   59999 /* 每个用户应用共享的 gid 结束 */

#if !defined(EXCLUDE_FS_CONFIG_STRUCTURES)
struct android_id_info {
    const char *name;
    unsigned aid;
};

static const struct android_id_info android_ids[] = {
    { "root",          AID_ROOT, },

    { "system",        AID_SYSTEM, },

    { "radio",         AID_RADIO, },
    { "bluetooth",     AID_BLUETOOTH, },
    { "graphics",      AID_GRAPHICS, },
    { "input",         AID_INPUT, },
    { "audio",         AID_AUDIO, },
    { "camera",        AID_CAMERA, },
    { "log",           AID_LOG, },
    { "compass",       AID_COMPASS, },
    { "mount",         AID_MOUNT, },
    { "wifi",          AID_WIFI, },
    { "adb",           AID_ADB, },
    { "install",       AID_INSTALL, },
    { "media",         AID_MEDIA, },
    { "dhcp",          AID_DHCP, },
    { "sdcard_rw",     AID_SDCARD_RW, },
    { "vpn",           AID_VPN, },
    { "keystore",      AID_KEYSTORE, },
    { "usb",           AID_USB, },
    { "drm",           AID_DRM, },
    { "mdnsr",         AID_MDNSR, },
    { "gps",           AID_GPS, },
    // AID_UNUSED1 已弃用
    { "media_rw",      AID_MEDIA_RW, },
    { "mtp",           AID_MTP, },
    // AID_UNUSED2 已弃用
    { "drmrpc",        AID_DRMRPC, },
    { "nfc",           AID_NFC, },
    { "sdcard_r",      AID_SDCARD_R, },
    { "clat",          AID_CLAT, },
    { "loop_radio",    AID_LOOP_RADIO, },
    { "mediadrm",      AID_MEDIA_DRM, },
    { "package_info",  AID_PACKAGE_INFO, },
    { "sdcard_pics",   AID_SDCARD_PICS, },
    { "sdcard_av",     AID_SDCARD_AV, },
    { "sdcard_all",    AID_SDCARD_ALL, },
    { "logd",          AID_LOGD, },
    { "shared_relro",  AID_SHARED_RELRO, },

    { "shell",         AID_SHELL, },
    { "cache",         AID_CACHE, },
    { "diag",          AID_DIAG, },

    { "net_bt_admin",  AID_NET_BT_ADMIN, },
    { "net_bt",        AID_NET_BT, },
    { "inet",          AID_INET, },
    { "net_raw",       AID_NET_RAW, },
    { "net_admin",     AID_NET_ADMIN, },
    { "net_bw_stats",  AID_NET_BW_STATS, },
    { "net_bw_acct",   AID_NET_BW_ACCT, },
    { "net_bt_stack",  AID_NET_BT_STACK, },

    { "everybody",     AID_EVERYBODY, },
    { "misc",          AID_MISC, },
    { "nobody",        AID_NOBODY, },
};

#define android_id_count \
    (sizeof(android_ids) / sizeof(android_ids[0]))

/**
 * @brief 文件系统路径配置规则结构体
 * 
 * 用于定义文件或目录的权限、所有者和能力属性。
 * android_dirs[] 和 android_files[] 数组中的每一项都是
 * 一个 fs_path_config 实例，按"首个匹配"原则查找。
 */
struct fs_path_config {
    unsigned mode;           /* 权限模式（如 00755、02750 等，包含 setuid/setgid 位） */
    unsigned uid;            /* 所有者用户 ID（AID_* 常量） */
    unsigned gid;            /* 所有者用户组 ID（AID_* 常量） */
    uint64_t capabilities;   /* Linux capability 位掩码（如 CAP_SETUID | CAP_SETGID） */
    const char *prefix;      /* 路径前缀（以 '/' 为根的相对路径，结尾 '*' 表示通配符） */
};

/* 目录规则。
** 这些规则基于"首个匹配"原则应用，因此应从最具体的路径开始，逐步向上至根目录。
** 最后一条以 { 00755, AID_ROOT, AID_ROOT, 0, 0 } 结尾作为默认兜底规则。
*/

static const struct fs_path_config android_dirs[] = {
    { 00770, AID_SYSTEM, AID_CACHE,  0, "cache" },
    { 00771, AID_SYSTEM, AID_SYSTEM, 0, "data/app" },
    { 00771, AID_SYSTEM, AID_SYSTEM, 0, "data/app-private" },
    { 00771, AID_ROOT,   AID_ROOT,   0, "data/dalvik-cache" },
    { 00771, AID_SYSTEM, AID_SYSTEM, 0, "data/data" },
    { 00771, AID_SHELL,  AID_SHELL,  0, "data/local/tmp" },
    { 00771, AID_SHELL,  AID_SHELL,  0, "data/local" },
    { 01771, AID_SYSTEM, AID_MISC,   0, "data/misc" },
    { 00770, AID_DHCP,   AID_DHCP,   0, "data/misc/dhcp" },
    { 00771, AID_SHARED_RELRO, AID_SHARED_RELRO, 0, "data/misc/shared_relro" },
    { 00775, AID_MEDIA_RW, AID_MEDIA_RW, 0, "data/media" },
    { 00775, AID_MEDIA_RW, AID_MEDIA_RW, 0, "data/media/Music" },
    { 00771, AID_SYSTEM, AID_SYSTEM, 0, "data" },
    { 00750, AID_ROOT,   AID_SHELL,  0, "sbin" },
    { 00755, AID_ROOT,   AID_SHELL,  0, "system/bin" },
    { 00755, AID_ROOT,   AID_SHELL,  0, "system/vendor" },
    { 00755, AID_ROOT,   AID_SHELL,  0, "system/xbin" },
    { 00755, AID_ROOT,   AID_ROOT,   0, "system/etc/ppp" },
    { 00755, AID_ROOT,   AID_SHELL,  0, "vendor" },
    { 00777, AID_ROOT,   AID_ROOT,   0, "sdcard" },
    { 00755, AID_ROOT,   AID_ROOT,   0, 0 },
};

/* 文件规则。
** 这些规则基于"首个匹配"原则应用，因此应从最具体的路径开始，逐步向上至根目录。
** 以 * 结尾的前缀表示通配符，允许部分匹配。
*/
static const struct fs_path_config android_files[] = {
    { 00440, AID_ROOT,      AID_SHELL,     0, "system/etc/init.goldfish.rc" },
    { 00550, AID_ROOT,      AID_SHELL,     0, "system/etc/init.goldfish.sh" },
    { 00550, AID_ROOT,      AID_SHELL,     0, "system/etc/init.tiny4412.sh" },
    { 00440, AID_ROOT,      AID_SHELL,     0, "system/etc/init.trout.rc" },
    { 00550, AID_ROOT,      AID_SHELL,     0, "system/etc/init.ril" },
    { 00550, AID_ROOT,      AID_SHELL,     0, "system/etc/init.testmenu" },
    { 00550, AID_DHCP,      AID_SHELL,     0, "system/etc/dhcpcd/dhcpcd-run-hooks" },
    { 00444, AID_RADIO,     AID_AUDIO,     0, "system/etc/AudioPara4.csv" },
    { 00555, AID_ROOT,      AID_ROOT,      0, "system/etc/ppp/*" },
    { 00555, AID_ROOT,      AID_ROOT,      0, "system/etc/rc.*" },
    { 00644, AID_SYSTEM,    AID_SYSTEM,    0, "data/app/*" },
    { 00644, AID_MEDIA_RW,  AID_MEDIA_RW,  0, "data/media/*" },
    { 00644, AID_SYSTEM,    AID_SYSTEM,    0, "data/app-private/*" },
    { 00644, AID_APP,       AID_APP,       0, "data/data/*" },
    { 00755, AID_ROOT,      AID_ROOT,      0, "system/bin/ping" },

    /* 以下文件刻意设置为 set-gid 而非 set-uid。请勿修改。 */
    { 02750, AID_ROOT,      AID_INET,      0, "system/bin/netcfg" },

    /* 以下五个文件刻意设置为 set-uid，但它们不在 user 版本中。 */
    { 04750, AID_ROOT,      AID_SHELL,     0, "system/xbin/su" },
    { 06755, AID_ROOT,      AID_ROOT,      0, "system/xbin/librank" },
    { 06755, AID_ROOT,      AID_ROOT,      0, "system/xbin/procrank" },
    { 06755, AID_ROOT,      AID_ROOT,      0, "system/xbin/procmem" },
    { 06755, AID_ROOT,      AID_ROOT,      0, "system/xbin/tcpdump" },
    { 04770, AID_ROOT,      AID_RADIO,     0, "system/bin/pppd-ril" },

    /* 以下文件具有增强能力，并且包含在 user 版本中。 */
    { 00750, AID_ROOT,      AID_SHELL,     (1 << CAP_SETUID) | (1 << CAP_SETGID), "system/bin/run-as" },

    { 00750, AID_ROOT,      AID_ROOT,      0, "system/bin/uncrypt" },
    { 00750, AID_ROOT,      AID_ROOT,      0, "system/bin/install-recovery.sh" },
    { 00755, AID_ROOT,      AID_SHELL,     0, "system/bin/*" },
    { 00755, AID_ROOT,      AID_ROOT,      0, "system/lib/valgrind/*" },
    { 00755, AID_ROOT,      AID_ROOT,      0, "system/lib64/valgrind/*" },
    { 00755, AID_ROOT,      AID_SHELL,     0, "system/xbin/*" },
    { 00755, AID_ROOT,      AID_SHELL,     0, "system/vendor/bin/*" },
    { 00755, AID_ROOT,      AID_SHELL,     0, "system/busybox/*" },
    { 00755, AID_ROOT,      AID_SHELL,     0, "vendor/bin/*" },
    { 00750, AID_ROOT,      AID_SHELL,     0, "sbin/*" },
    { 00755, AID_ROOT,      AID_ROOT,      0, "bin/*" },
    { 00750, AID_ROOT,      AID_SHELL,     0, "init*" },
    { 00750, AID_ROOT,      AID_SHELL,     0, "sbin/fs_mgr" },
    { 00640, AID_ROOT,      AID_SHELL,     0, "fstab.*" },
    { 00666, AID_SYSTEM,    AID_MISC,      0, "data/system/ethernet.conf" },
    { 00644, AID_ROOT,      AID_ROOT,      0, 0 },
};

/**
 * @brief 为指定路径查找并应用文件系统配置规则
 * 
 * 按 android_dirs[]（dir=true）或 android_files[]（dir=false）数组
 * 从上到下查找首个匹配项，将匹配到的 uid/gid/mode/capabilities
 * 通过输出参数返回。
 * 
 * 匹配规则：
 * - 目录：路径以 prefix 开头即算匹配
 * - 文件：prefix 以 '*' 结尾则路径以 prefix 前半段开头算匹配；
 *         否则要求路径完全相等
 * - 默认兜底：两个数组最后一条 prefix=0 的条目匹配所有剩余路径
 * 
 * @param path 文件或目录的绝对路径（函数内部会去掉开头的 '/'）
 * @param dir  1 表示查目录规则，0 表示查文件规则
 * @param uid  输出参数：匹配规则的 uid
 * @param gid  输出参数：匹配规则的 gid
 * @param mode 输入/输出参数：输入原始 mode，输出叠加了规则权限位后的 mode
 * @param capabilities 输出参数：匹配规则的 Linux capability 位掩码
 */
static inline void fs_config(const char *path, int dir,
                             unsigned *uid, unsigned *gid, unsigned *mode, uint64_t *capabilities)
{
    const struct fs_path_config *pc;
    int plen;

    /* 去掉绝对路径开头的 '/'，因为规则中 prefix 都是相对根目录的 */
    if (path[0] == '/') {
        path++;
    }

    /* 根据 dir 标志选择目录规则表还是文件规则表 */
    pc = dir ? android_dirs : android_files;
    plen = strlen(path);
    for(; pc->prefix; pc++){
        int len = strlen(pc->prefix);
        if (dir) {
            /* 目录匹配：路径必须以 prefix 开头且至少等长 */
            if(plen < len) continue;
            if(!strncmp(pc->prefix, path, len)) break;
            continue;
        }
        /* 文件匹配：prefix 以 '*' 结尾允许部分匹配，否则要求完全相等 */
        if (pc->prefix[len -1] == '*') {
            if(!strncmp(pc->prefix, path, len - 1)) break;
        } else if (plen == len){
            if(!strncmp(pc->prefix, path, len)) break;
        }
    }
    /* 应用匹配到的规则 */
    *uid = pc->uid;
    *gid = pc->gid;
    /* mode: 保留原文件的部分模式位，叠加规则中的权限位 */
    *mode = (*mode & (~07777)) | pc->mode;
    *capabilities = pc->capabilities;

#if 0
    fprintf(stderr,"< '%s' '%s' %d %d %o >\n",
            path, pc->prefix ? pc->prefix : "", *uid, *gid, *mode);
#endif
}
#endif
#endif
