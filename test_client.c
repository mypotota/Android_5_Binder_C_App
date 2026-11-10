/* Copyright 2008 The Android Open Source Project
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <linux/types.h>
#include<stdbool.h>
#include <string.h>

#include <private/android_filesystem_config.h>

#include "binder.h"
#include "test_server.h"



/**
 * @brief 通过 Service Manager 查找指定名称的服务并获取其句柄
 * 
 * 1. 构造 binder_io，调用 SVC_MGR_CHECK_SERVICE
 * 2. 从 reply 中读取返回的 handle
 * 3. 对 handle 执行 binder_acquire 增加引用计数
 * 
 * @param bs binder 状态
 * @param target Service Manager 句柄（通常为 BINDER_SERVICE_MANAGER）
 * @param name 服务名称（如 "hello"、"goodbye"）
 * @return 成功返回服务句柄，失败返回 0
 */
uint32_t svcmgr_lookup(struct binder_state *bs, uint32_t target, const char *name)
{
    uint32_t handle;
    unsigned iodata[512/4];
    struct binder_io msg, reply;

    bio_init(&msg, iodata, sizeof(iodata), 4);
    bio_put_uint32(&msg, 0);  // 严格模式头
    bio_put_string16_x(&msg, SVC_MGR_NAME);
    bio_put_string16_x(&msg, name);

    if (binder_call(bs, &msg, &reply, target, SVC_MGR_CHECK_SERVICE))
        return 0;

    handle = bio_get_ref(&reply);

    if (handle)
        binder_acquire(bs, handle);

    binder_done(bs, &msg, &reply);

    return handle;
}


/* 全局 binder 状态和服务句柄，供全局函数使用 */
struct binder_state *g_bs;
uint32_t g_hello_handle;    /* hello 服务的句柄 */
uint32_t g_goodbye_handle;  /* goodbye 服务的句柄 */

/**
 * @brief 调用 hello 服务的 sayhello() 方法（无参数）
 * 
 * 步骤：构造 binder_io → 写入严格模式头 → 写入接口名 → 发起调用 → 清理
 * 对应服务端的 HELLO_SVR_CMD_SAYHELLO
 */
void sayhello(void)
{
    unsigned iodata[512/4];
    struct binder_io msg, reply;

	/* 构造binder_io */
    bio_init(&msg, iodata, sizeof(iodata), 4);
    bio_put_uint32(&msg, 0);  // 严格模式头
    bio_put_string16_x(&msg, "IHelloService");

	/* 放入参数 */

	/* 调用binder_call */
    if (binder_call(g_bs, &msg, &reply, g_hello_handle, HELLO_SVR_CMD_SAYHELLO))
        return ;
	
	/* 从reply中解析出返回值 */

    binder_done(g_bs, &msg, &reply);
	
}

/**
 * @brief 调用 hello 服务的 sayhello_to(name) 方法
 * 
 * 对应服务端的 HELLO_SVR_CMD_SAYHELLO_TO
 * 
 * @param name 要打招呼的名字
 * @return 服务端返回的计数结果，调用失败返回 0
 */
int sayhello_to(char *name)
{
	unsigned iodata[512/4];
	struct binder_io msg, reply;
	int ret;
	int exception;

	/* 构造binder_io */
	bio_init(&msg, iodata, sizeof(iodata), 4);
	bio_put_uint32(&msg, 0);  // 严格模式头
    bio_put_string16_x(&msg, "IHelloService");

	/* 放入参数 */
    bio_put_string16_x(&msg, name);

	/* 调用binder_call */
	if (binder_call(g_bs, &msg, &reply, g_hello_handle, HELLO_SVR_CMD_SAYHELLO_TO))
		return 0;
	
	/* 从reply中解析出返回值 */
	exception = bio_get_uint32(&reply);
	if (exception)
		ret = -1;
	else
		ret = bio_get_uint32(&reply);

	binder_done(g_bs, &msg, &reply);

	return ret;
	
}


/**
 * @brief 调用 goodbye 服务的 saygoodbye() 方法（无参数）
 * 对应服务端的 GOODBYE_SVR_CMD_SAYGOODBYE
 */
void saygoodbye(void)
{
    unsigned iodata[512/4];
    struct binder_io msg, reply;

	/* 构造binder_io */
    bio_init(&msg, iodata, sizeof(iodata), 4);
    bio_put_uint32(&msg, 0);  // 严格模式头
    bio_put_string16_x(&msg, "IGoodbyeService");

	/* 放入参数 */

	/* 调用binder_call */
    if (binder_call(g_bs, &msg, &reply, g_goodbye_handle, GOODBYE_SVR_CMD_SAYGOODBYE))
        return ;
	
	/* 从reply中解析出返回值 */

    binder_done(g_bs, &msg, &reply);
	
}

/**
 * @brief 调用 goodbye 服务的 saygoodbye_to(name) 方法
 * 
 * 对应服务端的 GOODBYE_SVR_CMD_SAYGOODBYE_TO
 * 
 * @param name 要告别的名字
 * @return 服务端返回的计数结果，调用失败返回 0
 */
int saygoodbye_to(char *name)
{
	unsigned iodata[512/4];
	struct binder_io msg, reply;
	int ret;
	int exception;

	/* 构造binder_io */
	bio_init(&msg, iodata, sizeof(iodata), 4);
	bio_put_uint32(&msg, 0);  // 严格模式头
    bio_put_string16_x(&msg, "IGoodbyeService");

	/* 放入参数 */
    bio_put_string16_x(&msg, name);

	/* 调用binder_call */
	if (binder_call(g_bs, &msg, &reply, g_goodbye_handle, GOODBYE_SVR_CMD_SAYGOODBYE_TO))
		return 0;
	
	/* 从reply中解析出返回值 */
	exception = bio_get_uint32(&reply);
	if (exception)
		ret = -1;
	else
		ret = bio_get_uint32(&reply);

	binder_done(g_bs, &msg, &reply);

	return ret;
	
}





/* 用法：
 * ./test_client hello
 * ./test_client hello <name>
 */

/**
 * @brief 测试客户端主函数
 * 
 * 使用流程：
 * 1. 打开 binder 驱动
 * 2. 通过 svcmgr_lookup 获取 goodbye 和 hello 服务的句柄
 * 3. 根据命令行参数调用对应的服务方法
 * 
 * @param argc 参数个数
 * @param argv 命令行参数：argv[1] = "hello" 或 "goodbye"，argv[2] = 可选的名字
 */
int main(int argc, char **argv)
{
    int fd;
    struct binder_state *bs;
    uint32_t svcmgr = BINDER_SERVICE_MANAGER;
    uint32_t handle;
	int ret;

	if (argc < 2){
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "%s <hello|goodbye>\n", argv[0]);
        fprintf(stderr, "%s <hello|goodbye> <name>\n", argv[0]);
        return -1;
	}

    bs = binder_open(128*1024);
    if (!bs) {
        fprintf(stderr, "failed to open binder driver\n");
        return -1;
    }
	g_bs = bs;


	/* get service */
	handle = svcmgr_lookup(bs, svcmgr, "goodbye");
	if (!handle) {
        fprintf(stderr, "failed to get goodbye service\n");
        return -1;
	}
	g_goodbye_handle = handle;
	fprintf(stderr, "Handle for goodbye service = %d\n", g_goodbye_handle);

	handle = svcmgr_lookup(bs, svcmgr, "hello");
	if (!handle) {
        fprintf(stderr, "failed to get hello service\n");
        return -1;
	}
	g_hello_handle = handle;
	fprintf(stderr, "Handle for hello service = %d\n", g_hello_handle);

	/* send data to server */
	if (!strcmp(argv[1], "hello"))
	{
		if (argc == 2) {
			sayhello();
		} else if (argc == 3) {
			ret = sayhello_to(argv[2]);
	        fprintf(stderr, "get ret of sayhello_to = %d\n", ret);		
		}
	} else if (!strcmp(argv[1], "goodbye"))
	{
		if (argc == 2) {
			saygoodbye();
		} else if (argc == 3) {
			ret = saygoodbye_to(argv[2]);
	        fprintf(stderr, "get ret of sayhello_to = %d\n", ret);		
		}
	}

	binder_release(bs, handle);

    return 0;
}
