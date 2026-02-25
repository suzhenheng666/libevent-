#include "http_routes.h"
#include "device_api.h"
#include "frontend_api.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct route_entry_s
{
	const char* method;
	const char* path;
	route_handler_t handler;
	const char* desc;
}route_entry_t;

static route_entry_t g_routes[32];
static int g_route_count=0;

//内部函数：注册一个路由
static void register_route(const char* method,
						   const char* path,
                           route_handler_t handler,
                           const char* desc)
{
	if (g_route_count >= 32) {
        printf("[WARN] 路由表已满，无法注册: %s %s\n", method, path);
        return;
    }

    g_routes[g_route_count].method = method;
    g_routes[g_route_count].path = path;
    g_routes[g_route_count].handler = handler;
    g_routes[g_route_count].desc = desc;
    g_route_count++;
    
    printf("[DEBUG] 注册路由: %-6s %-30s -> %s\n",  method, path, desc);
}

static void handle_not_found(struct bufferevent* bev, const http_request_t* req)
{
	char response[1024];
	char body[512];

	snprintf(body,sizeof(body),
	"{\"error\":404,\"message\":\"路由未找到: %s %s\"}",
	req->method,req->uri);

	size_t body_length=strlen(body);

	snprintf(response,sizeof(response),
		"HTTP/1.1 404 Not Found\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "%s",
		body_length,
		body);
	
	bufferevent_write(bev,response,strlen(response));

	printf("[WARN] 404: %s %s\n", req->method, req->uri);
}

//路由初始化函数
void http_routes_init(void)
{
	printf("[INFO] 初始化路由系统...\n");

	g_route_count=0;

    printf("\n[INFO] 注册设备API路由:\n");
	register_route("POST", "/device/report", device_report_handler, "设备数据上报");
    register_route("GET", "/device/update", device_update_handler, "设备软件更新");
    register_route("GET", "/device/download", device_download_handler, "设备文件下载");

	printf("\n[INFO] 注册前端API路由:\n");
    register_route("GET", "/frontend/devices/latest", frontend_devices_latest_handler, "获取所有设备最新数据");
    register_route("GET", "/frontend/device/temperature", frontend_device_temperature_handler, "获取设备温度历史");
    register_route("GET", "/frontend/device/humidity", frontend_device_humidity_handler, "获取设备湿度历史");

    printf("[INFO] 路由初始化完成，共注册 %d 个路由\n\n", g_route_count);
}

//路由分发函数
void http_dispatch_request(struct bufferevent* bev, const http_request_t* req)
{
	const char* method=req->method;
	const char* path=req->path;

	printf("[DEBUG] 路由分发: %s %s\n", method, path);

	for (int i = 0; i < g_route_count; i++)
	{
		if(strcasecmp(method,g_routes[i].method)!=0)
		{
			continue;
		}

		if(strcasecmp(path,g_routes[i].path)==0)
		{
			printf("[INFO] 路由匹配: %s %s -> %s\n", 
                   method, path, g_routes[i].desc);
			
			g_routes[i].handler(bev, req);
			return;
		}
	}

	printf("[WARN] 路由未找到: %s %s\n", method, path);
    handle_not_found(bev, req);
}