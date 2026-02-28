#include "frontend_api.h"
#include "utils.h"
#include "db_mysql.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>

void frontend_devices_latest_handler(struct bufferevent* bev, const http_request_t* req)
{
	if(strcmp(req->method,"GET")!=0)
	{
		send_json_error(bev, 405, "只支持GET方法");
        return;
	}

	printf("[INFO] 前端请求所有设备最新数据\n");

	int device_count = 0;
    device_data_t* devices = db_mysql_get_all_devices_latest(&device_count);

	if (devices == NULL && device_count == 0) {
        /* 没有设备数据，返回空数组 */
        send_json_ok(bev, "{\"code\":0,\"msg\":\"ok\",\"data\":[]}");
        printf("[INFO] 数据库中没有设备数据，返回空数组\n");
        return;
    }

	if (devices == NULL) {
        /* 数据库查询失败 */
        send_json_error(bev, 500, "数据库查询失败");
        printf("[ERROR] 数据库查询失败\n");
        return;
    }

	cJSON* root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root,"code",0);
	cJSON_AddStringToObject(root,"msg","ok");

	cJSON* data_arry=cJSON_CreateArray();

	for(int i=0;i<device_count;i++)
	{
		cJSON* device_obj=cJSON_CreateObject();

		cJSON_AddStringToObject(device_obj,"device_id",devices[i].device_id);
		cJSON_AddNumberToObject(device_obj, "longitude", devices[i].longitude);
        cJSON_AddNumberToObject(device_obj, "latitude", devices[i].latitude);
        cJSON_AddNumberToObject(device_obj, "temperature", devices[i].temperature);
        cJSON_AddNumberToObject(device_obj, "humidity", devices[i].humidity);  /* 改为湿度 */
        cJSON_AddNumberToObject(device_obj, "obstacle_distance", devices[i].obstacle_distance);
        cJSON_AddStringToObject(device_obj, "timestamp", devices[i].timestamp);

		cJSON_AddItemToArray(data_arry,device_obj);
	}

	cJSON_AddItemToObject(root,"data",data_arry);

	char* json_str=cJSON_PrintUnformatted(root);
	if (!json_str) {
        send_json_error(bev, 500, "JSON生成失败");
        cJSON_Delete(root);
        free(devices);
        return;
    }

	send_json_ok(bev, json_str);

	printf("[INFO] 返回 %d 个设备的最近数据\n", device_count);

	free(json_str);
    cJSON_Delete(root);
    free(devices);
}

void frontend_device_temperature_handler(struct bufferevent* bev, const http_request_t* req)
{
	if (strcmp(req->method, "GET") != 0) {
        send_json_error(bev, 405, "只支持GET方法");
        return;
    }

	char* device_id = http_get_query_param(req, "device_id");
    if (!device_id || strlen(device_id) == 0) {
        send_json_error(bev, 400, "缺少device_id参数");
        return;
    }

	printf("[INFO] 前端请求设备温度历史: device_id=%s\n", device_id);

	int data_count = 0;
	time_series_data_t* data=db_mysql_get_device_temperature_history(device_id, &data_count);

	cJSON* root=cJSON_CreateObject();
	cJSON_AddNumberToObject(root,"code",0);
	cJSON_AddStringToObject(root,"msg","ok");

	cJSON* data_array=cJSON_CreateArray();
	if(!data)
	{
		for(int i=0;i<data_count;i++)
		{
			cJSON* item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "time", data[i].time);
            cJSON_AddStringToObject(item, "temperature", data[i].value);
            cJSON_AddItemToArray(data_array, item);
		}
	}

	cJSON_AddItemToObject(root,"data",data_array);

	char* json_str=cJSON_PrintUnformatted(root);
	if (!json_str) {
        send_json_error(bev, 500, "JSON生成失败");
        cJSON_Delete(root);
        free(device_id);
        if (data) free(data);
        return;
    }

	send_json_ok(bev,json_str);
	printf("[INFO] 返回设备 %s 的 %d 条温度历史数据\n", device_id, data_count);
	free(json_str);
    cJSON_Delete(root);
    free(device_id);
    if (data) free(data);
}

void frontend_device_humidity_handler(struct bufferevent* bev, const http_request_t* req)
{
	if (strcmp(req->method, "GET") != 0) {
        send_json_error(bev, 405, "只支持GET方法");
        return;
    }

	char* device_id = http_get_query_param(req, "device_id");
    if (!device_id || strlen(device_id) == 0) {
        send_json_error(bev, 400, "缺少device_id参数");
        return;
    }

	printf("[INFO] 前端请求设备湿度历史: device_id=%s\n", device_id);

	int data_count = 0;
    time_series_data_t* data = db_mysql_get_device_humidity_history(device_id, &data_count);

	cJSON* root=cJSON_CreateObject;
	cJSON_AddNumberToObject(root,"code",0);
	cJSON_AddStringToObject(root,"msg","ok");

	cJSON* data_array=cJSON_CreateArray();
	if(!data)
	{
		for(int i=0;i<data_count;i++)
		{
			cJSON* item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "time", data[i].time);
            cJSON_AddStringToObject(item, "humidity", data[i].value);
            cJSON_AddItemToArray(data_array, item);
		}
	}

	cJSON_AddItemToObject(root,"data",data_array);

	char* json_str=cJSON_PrintUnformatted(root);
	if (!json_str) {
        send_json_error(bev, 500, "JSON生成失败");
        cJSON_Delete(root);
        free(device_id);
        if (data) free(data);
        return;
    }

	send_json_ok(bev, json_str);

	printf("[INFO] 返回设备 %s 的 %d 条湿度历史数据\n", device_id, data_count);

	free(json_str);
    cJSON_Delete(root);
    free(device_id);
    if (data) free(data);
}