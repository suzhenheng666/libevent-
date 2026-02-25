#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

typedef struct http_server_s http_server_t;//服务器句柄,隐藏内部结构

//创建http服务器实例
http_server_t* http_server_new(int port);

//启动http服务，阻塞运行
int http_server_start(http_server_t* server);

//停止http服务器
void http_server_stop(http_server_t* server);

//释放http资源
void http_server_free(http_server_t* server);

#endif