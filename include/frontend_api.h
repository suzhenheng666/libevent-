#ifndef FRONTEND_API_H
#define FRONTEND_API_H

#include <event2/bufferevent.h>
#include "http_parse.h"

/*
	获取所有设备的最新数据
	对应路由：GET /frontend/devices/latest
	返回所有设备的最新上报数据
*/
void frontend_devices_latest_handler(struct bufferevent* bev, const http_request_t* req);

/*
	获取指定设备的温度历史数据
	对应路由：GET /frontend/device/temperature?device_id=xxx
	查询参数：device_id=设备ID
*/
void frontend_device_temperature_handler(struct bufferevent* bev, const http_request_t* req);

/*
	获取设备湿度历史数据
	处理 GET /api/v1/device/humidity?device_id=xxx
 	返回指定设备最近10分钟的湿度变化数据
*/
void frontend_device_humidity_handler(struct bufferevent* bev, const http_request_t* req);

#endif