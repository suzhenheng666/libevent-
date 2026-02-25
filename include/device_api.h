#ifndef DEVICE_API_H
#define DEVICE_API_H

#include <event2/bufferevent.h>
#include "http_parse.h"

/*
	设备数据上报处理
	对应路由：POST /device/report
	请求体JSON格式：{"device_id":101,"longitude":120.12,...}
*/
void device_report_handler(struct bufferevent* bev, const http_request_t* req);

/*
	设备软件更新处理
	对应路由：GET /device/update
 	返回更新信息：{"version":"1.2.3","url":"..."}
*/
void device_update_handler(struct bufferevent* bev, const http_request_t* req);

/*
	设备文件下载处理
	对应路由：GET /device/download?file=xxx
	查询参数：file=文件名
*/
void device_download_handler(struct bufferevent* bev, const http_request_t* req);


#endif