
#ifndef _TEST_SERVER_H
#define _TEST_SERVER_H

/* Hello 服务的事务码定义 */
#define HELLO_SVR_CMD_SAYHELLO     1  /* sayhello() 无参数调用 */
#define HELLO_SVR_CMD_SAYHELLO_TO  2  /* sayhello_to(name) 带参数调用 */

/* Goodbye 服务的事务码定义 */
#define GOODBYE_SVR_CMD_SAYGOODBYE     1  /* saygoodbye() 无参数调用 */
#define GOODBYE_SVR_CMD_SAYGOODBYE_TO  2  /* saygoodbye_to(name) 带参数调用 */

#endif // _TEST_SERVER_H

