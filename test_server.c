
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
 * @brief 向 Service Manager 注册/发布服务
 * 
 * 1. 构造 binder_io，调用 SVC_MGR_ADD_SERVICE
 * 2. 将本地处理函数指针（ptr）作为 binder 对象放入 msg
 * 3. 从 reply 中获取注册结果
 * 
 * @param bs binder 状态
 * @param target Service Manager 句柄
 * @param name 服务名称（如 "hello"、"goodbye"）
 * @param ptr 服务处理函数指针（发布后客户端可通过 binder 调用）
 * @return 成功返回 0，失败返回 -1
 */
int svcmgr_publish(struct binder_state *bs, uint32_t target, const char *name, void *ptr)
{
    int status;
    unsigned iodata[512/4];
    struct binder_io msg, reply;

    bio_init(&msg, iodata, sizeof(iodata), 4);
    bio_put_uint32(&msg, 0);  // 严格模式头
    bio_put_string16_x(&msg, SVC_MGR_NAME);
    bio_put_string16_x(&msg, name);
    bio_put_obj(&msg, ptr);

    if (binder_call(bs, &msg, &reply, target, SVC_MGR_ADD_SERVICE))
        return -1;

    status = bio_get_uint32(&reply);

    binder_done(bs, &msg, &reply);

    return status;
}

/** @brief hello 服务的 sayhello() 实现：打印 "say hello : N"  */
void sayhello(void)
{
	static int cnt = 0;
	fprintf(stderr, "say hello : %d\n", ++cnt);
}


/** @brief hello 服务的 sayhello_to(name) 实现：打印名字并返回调用计数 */
int sayhello_to(char *name)
{
	static int cnt = 0;
	fprintf(stderr, "say hello to %s : %d\n", name, ++cnt);
	return cnt;
}

/** @brief goodbye 服务的 saygoodbye() 实现：打印 "say goodbye : N"  */
void saygoodbye(void)
{
	static int cnt = 0;
	fprintf(stderr, "say goodbye : %d\n", ++cnt);
}


/** @brief goodbye 服务的 saygoodbye_to(name) 实现：打印名字并返回调用计数 */
int saygoodbye_to(char *name)
{
	static int cnt = 0;
	fprintf(stderr, "say goodbye to %s : %d\n", name, ++cnt);
	return cnt;
}


/**
 * @brief Hello 服务的 Binder 事务处理函数
 * 
 * 根据 txn->code 分发到不同的本地方法：
 * - HELLO_SVR_CMD_SAYHELLO → sayhello()
 * - HELLO_SVR_CMD_SAYHELLO_TO → sayhello_to(name)
 * 
 * 返回格式：[exception: uint32] [return_value: uint32]
 * 
 * @param bs binder 状态
 * @param txn 驱动传入的事务数据
 * @param msg 客户端请求参数的 binder_io
 * @param reply 用于填充回复的 binder_io
 * @return 成功返回 0，失败返回 -1
 */
int hello_service_handler(struct binder_state *bs,
                   struct binder_transaction_data *txn,
                   struct binder_io *msg,
                   struct binder_io *reply)
{
	/* 根据txn->code知道要调用哪一个函数
	 * 如果需要参数, 可以从msg取出
	 * 如果要返回结果, 可以把结果放入reply
	 */

	/* sayhello
	 * sayhello_to
	 */
	
    uint16_t *s;
	char name[512];
    size_t len;
    uint32_t handle;
    uint32_t strict_policy;
	int i;


    // 等同于 Parcel::enforceInterface()，读取带有严格模式策略掩码和接口名称的 RPC 头。
    // 注意：我们忽略 strict_policy 且不进一步传播它（因为我们不执行任何出站 RPC）。
    strict_policy = bio_get_uint32(msg);


    switch(txn->code) {
    case HELLO_SVR_CMD_SAYHELLO:
		sayhello();
		bio_put_uint32(reply, 0); /* 无异常 */
        return 0;

    case HELLO_SVR_CMD_SAYHELLO_TO:
		/* 从msg里取出字符串 */
		s = bio_get_string16(msg, &len);  //"IHelloService"
		s = bio_get_string16(msg, &len);  // 名称
		if (s == NULL) {
			return -1;
		}
		for (i = 0; i < len; i++)
			name[i] = s[i];
		name[i] = '\0';

		/* 处理 */
		i = sayhello_to(name);

		/* 把结果放入reply */
		bio_put_uint32(reply, 0); /* 无异常 */
		bio_put_uint32(reply, i);
		
        break;

    default:
        fprintf(stderr, "unknown code %d\n", txn->code);
        return -1;
    }

    return 0;
}



/**
 * @brief Goodbye 服务的 Binder 事务处理函数
 * 
 * 根据 txn->code 分发到不同的本地方法：
 * - GOODBYE_SVR_CMD_SAYGOODBYE → saygoodbye()
 * - GOODBYE_SVR_CMD_SAYGOODBYE_TO → saygoodbye_to(name)
 * 
 * @param bs binder 状态
 * @param txn 驱动传入的事务数据
 * @param msg 客户端请求参数的 binder_io
 * @param reply 用于填充回复的 binder_io
 * @return 成功返回 0，失败返回 -1
 */
int goodbye_service_handler(struct binder_state *bs,
                   struct binder_transaction_data *txn,
                   struct binder_io *msg,
                   struct binder_io *reply)
{
	/* 根据txn->code知道要调用哪一个函数
	 * 如果需要参数, 可以从msg取出
	 * 如果要返回结果, 可以把结果放入reply
	 */

	/* sayhello
	 * sayhello_to
	 */
	
    uint16_t *s;
	char name[512];
    size_t len;
    uint32_t handle;
    uint32_t strict_policy;
	int i;


    // 等同于 Parcel::enforceInterface()，读取带有严格模式策略掩码和接口名称的 RPC 头。
    // 注意：我们忽略 strict_policy 且不进一步传播它（因为我们不执行任何出站 RPC）。
    strict_policy = bio_get_uint32(msg);


    switch(txn->code) {
    case GOODBYE_SVR_CMD_SAYGOODBYE:
		saygoodbye();
		bio_put_uint32(reply, 0); /* 无异常 */
        return 0;

    case GOODBYE_SVR_CMD_SAYGOODBYE_TO:
		/* 从msg里取出字符串 */
		s = bio_get_string16(msg, &len);  //"IGoodbyeService"
		s = bio_get_string16(msg, &len);  // 名称
		if (s == NULL) {
			return -1;
		}
		for (i = 0; i < len; i++)
			name[i] = s[i];
		name[i] = '\0';

		/* 处理 */
		i = saygoodbye_to(name);

		/* 把结果放入reply */
		bio_put_uint32(reply, 0); /* 无异常 */
		bio_put_uint32(reply, i);
		
        break;

    default:
        fprintf(stderr, "unknown code %d\n", txn->code);
        return -1;
    }

    return 0;
}


/**
 * @brief 顶层分发器：根据 txn->target.ptr 调用对应的具体 handler
 * 
 * 这是一个通用的路由函数，客户端通过 bio_put_obj 传入的 ptr（函数指针）
 * 会存到 target.ptr 中，此处通过强制类型转换后直接调用。
 * 这样设计可以让多个服务复用同一个入口点。
 */
int test_server_handler(struct binder_state *bs,
                   struct binder_transaction_data *txn,
                   struct binder_io *msg,
                   struct binder_io *reply)
{
	int (*handler)(struct binder_state *bs,
                   struct binder_transaction_data *txn,
                   struct binder_io *msg,
                   struct binder_io *reply);

	handler = (int (*)(struct binder_state *bs,
                   struct binder_transaction_data *txn,
                   struct binder_io *msg,
                   struct binder_io *reply))txn->target.ptr;
	
	return handler(bs, txn, msg, reply);
}

/**
 * @brief 测试服务器主函数
 * 
 * 使用流程：
 * 1. 打开 binder 驱动
 * 2. 向 Service Manager 注册 hello 和 goodbye 服务
 * 3. 设置驱动允许的最大线程数为 10
 * 4. 进入 binder 主循环，等待客户端请求
 */
int main(int argc, char **argv)
{
    int fd;
    struct binder_state *bs;
    uint32_t svcmgr = BINDER_SERVICE_MANAGER;
    uint32_t handle;
	int ret;

    bs = binder_open(128*1024);
    if (!bs) {
        fprintf(stderr, "failed to open binder driver\n");
        return -1;
    }

	/* add service */
	ret = svcmgr_publish(bs, svcmgr, "hello", hello_service_handler);
    if (ret) {
        fprintf(stderr, "failed to publish hello service\n");
        return -1;
    }
	ret = svcmgr_publish(bs, svcmgr, "goodbye", goodbye_service_handler);
    if (ret) {
        fprintf(stderr, "failed to publish goodbye service\n");
    }

#if 0
	while (1)
	{
		/* read data */
		/* parse data, and process */
		/* reply */
	}
#endif

	binder_set_maxthreads(bs, 10);

    binder_loop(bs, test_server_handler);

    return 0;
}
