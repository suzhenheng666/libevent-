#include <stdio.h>
#include <stdlib.h>
#include "signal_handler.h"
#include "http_server.h"
#include "db_mysql.h"

static void* g_server_instance=NULL;

//信号处理函数

void signal_handler(int sig)
{
	printf("\n[INFO] Received signal %d, shutting down...\n", sig);
    
    /* 停止HTTP服务器 */
    if (g_server_instance) {
        http_server_stop(g_server_instance);
    }
    
    /* 清理数据库连接 */
    db_mysql_close();
    
    printf("[INFO] Server shutdown completed.\n");
    exit(0);
}

//初始化信号处理器
void signal_handler_init(void* server_instance)
{
	g_server_instance=server_instance;

	signal(SIGINT, signal_handler);   /* Ctrl+C */
    signal(SIGTERM, signal_handler);  /* kill命令 */
    signal(SIGQUIT, signal_handler);  /* Ctrl+\ */

	signal(SIGPIPE, SIG_IGN);
}