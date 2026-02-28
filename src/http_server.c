#include"http_server.h"
#include"http_parse.h"
#include"http_routes.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<event2/listener.h>
#include<event2/buffer.h>
#include<event2/bufferevent.h>
#include<arpa/inet.h>

typedef struct client_ctx_s
{
	struct bufferevent* bev;

	char read_buf[8192];//读取缓冲区8MB
	size_t read_len;//记录当前buf中实际存储的字节数

	//INET_ADDRSTRLEN ipv4地址最大长度16
	char client_ip[INET_ADDRSTRLEN]; //例如：192.168.1.100

	int should_close; // 标志位，指示是否需要关闭连接 
}client_ctx_t;//存储每个客户端连接的状态信息

static void process_full_http_request(client_ctx_t *ctx)
{
	ctx->read_buf[ctx->read_len]='\0';

	//解析http请求
	http_request_t* req=http_parse_request(ctx->read_buf,ctx->read_len);
	if(!req)
	{
		printf("[WARN]解析HTTP请求失败 (客户端：%s)\n",ctx->client_ip);

		const char* bad_request=
			"HTTP/1.1 400 Bad Request\r\n"
			"Content-Length: 11\r\n"
			"\r\n"
			"Bad Request";
		bufferevent_write(ctx->bev,bad_request,strlen(bad_request));

		ctx->should_close=1;
		return;
	}

	printf("[INFO] %s %s 来自 %s\n",req->method,req->uri,ctx->client_ip );

	http_dispatch_request(ctx->bev,req);

	http_free_request(req);

	ctx->should_close=1;
	printf("[INFO] 请求处理完成，等待数据发送后关闭连接 (客户端：%s)\n",ctx->client_ip);
}

struct http_server_s
{
	struct event_base*base;//libevent事件基础，管理所有I/O事件，监听socket的accept事件
	struct evconnlistener* listener;//监听指定端口，接收新的客户端连接
	int port;//服务器端口
	int running;//服务器运行状态,防止重复启动
};

static void on_read(struct bufferevent* bev,void *arg)
{
	client_ctx_t* ctx=(client_ctx_t*)arg;
	struct evbuffer* input=bufferevent_get_input(bev);
	//计算本次可以读取的最大字节数
	//剩余空间=总容量-已用空间-1 给'\0'预留
	size_t to_read=sizeof(ctx->read_buf - ctx->read_len - 1);

	if(to_read == 0)
	{
		printf("[WARN]缓冲区满,关闭连接\n");
		bufferevent_free(bev);
		free(ctx);
		return;
	}
	int n=evbuffer_remove(input,ctx->read_buf+ctx->read_len,to_read);
	if(n<=0)
	{
		return;
	}

	ctx->read_len+=n;
	
	if(ctx->read_len>0)
	{
		process_full_http_request(ctx);
	}
}

//Todo代码不完整
static void on_write(struct buffervevnt* bev,void *arg)
{
	client_ctx_t* ctx=(client_ctx_t*)arg;
	if(ctx->should_close)
	{
		struct evbuffer* output=bufferevent_get_output(bev);
		size_t pending=evbuffer_get_length(output);

		if(pending>0)
		{
			printf("[DEBUG]还有%zu 字节待发送，等待下一次回调\n",pending);
			return;
		}
		printf("[INFO]数据发送完成，关闭连接(客户端：%s)\n",ctx->client_ip);

		bufferevent_free(bev);
		free(ctx);
	}
}

//实际需要调用bufferevent_free free
static void on_event(struct bufferevent* bev,short events,void* arg)
{
	client_ctx_t* ctx=(client_ctx_t*)arg;

	if(events & BEV_EVENT_ERROR)
	{
		printf("[ERROR]连接错误(客户端：%s)\n",ctx->client_ip);
	}

	if(events & BEV_EVENT_EOF)
	{
		printf("[INFO]客户端关闭连接(客户端：%s)\n",ctx->client_ip);
	}

	if(events & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
	{
		bufferevent_free(bev);
		free(ctx);
	}
}


static void on_accept(struct evconnlistener* listener,
	evutil_socket_t fd,//新连接socket的文件描述符
	struct sockaddr* addr,
	int socklen,//地址结构体长度
	void *arg//自定义参数
	)
{
	http_server_t* server=(http_server_t*)arg;
	struct bufferevent *bev=bufferevent_socket_new(
		server->base,
		fd,
		//BEV_OPT_CLOSE_ON_FREE当bufferevent被释放时，自动关闭底层的socket
		//BEV_OPT_DEFER_CALLBACKS延迟回调，优化性能，将多个事件合并，减少回调次数
		BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS
	);

	if(!bev)
	{
		fprintf(stderr,"[ERROR]创建bufferevent失败\n");
		evutil_closesocket(fd);
		return;
	}

	//创建客户端上下文
	client_ctx_t* ctx=(client_ctx_t*)calloc(1,sizeof(client_ctx_t));
	if(!ctx)
	{
		fprintf(stderr,"[ERROR]分配客户端上下文失败\n");
		return;
	}

	ctx->bev=bev;
	ctx->read_len=0;
	ctx->should_close=0;

	//记录客户端ip地址
	if(addr->sa_family==AF_INET) //判断是否为ipv4
	{
		struct sockaddr_in *sin=(struct sockaddr_in*)addr;
		inet_ntop(AF_INET,&sin->sin_addr,ctx->client_ip,sizeof(ctx->client_ip));
	}
	else
	{
		strcpy(ctx->client_ip,"Unknown");
	}
	printf("[INFO]新连接来自%s\n",ctx->client_ip);
	
	//设置回调函数
	bufferevent_setcb(bev,on_read,on_write,on_event,ctx);

	//启用监听事件（读/写）
	bufferevent_enable(bev,EV_READ | EV_WRITE);
}

http_server_t* http_server_new(int port)
{
	http_server_t* server=(http_server_t*)calloc(1,sizeof(http_server_t));
	if(!server)
	{
		fprintf(stderr,"[ERROR]分配服务器结构体失败\n");
		return NULL;
	}
	server->port=port;
	server->running=0;
	server->base=event_base_new();
	if(!server->base)
	{
		fprintf(stderr,"[ERROR]创建event_base失败\n");
		free(server);
		return NULL;
	}
	return server;
}

int http_server_start(http_server_t* server)
{
	if(!server)
	{
		fprintf(stderr,"[ERROR] 无效的服务器实例\n");
		return -1;
	}

	if(server->running)
	{
		fprintf(stderr,"[ERROR] 服务器已在运行\n");
		return -1;
	}

	/* 设置监听地址 */
	struct sockaddr_in sin;
	memset(&sin,0,sizeof(sin));
	sin.sin_family=AF_INET;
	sin.sin_addr.s_addr=htonl(INADDR_ANY);
	sin.sin_port=htons(server->port);

	/*创建TCP监听socket并绑定到指定端口，注册到事件循环中*/
	server->listener = evconnlistener_new_bind(
		server->base,
		on_accept,
		server,
		LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,
		-1,
		(struct sockaddr*)&sin,
		sizeof(sin)
	);

	if(!server->listener)
	{
		fprintf(stderr,"[ERROR] 创建监听器失败\n");
		return -1;
	}

	server->running = 1;
	printf("[INFO] HTTP服务器启动成功，监听端口: %d\n", server->port);

	/* 进入事件循环（阻塞） */
	event_base_dispatch(server->base);

	return 0;
}

void http_server_stop(http_server_t* server)
{
	if(!server || !server->running)
	{
		return;
	}
	printf("\n[INFO]正在停止服务器...\n");

	//停止事件循环
	if(server->base)
	{
		event_base_loopbreak(server->base);
	}
	server->running=0;
}

void http_server_free(http_server_t* server)
{
	if(!server)
	{
		return;
	}

	//释放监听器
	if(server->listener)
	{
		evconnlistener_free(server->listener);
	}

	//释放event_base示例
	if(server->base)
	{
		event_base_free(server->base);
	}
	free(server);
}