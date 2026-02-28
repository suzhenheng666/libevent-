#include "device_api.h"
#include "utils.h"
#include "db_mysql.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>

//设备数据上报处理
void device_report_handler(struct bufferevent* bev, const http_request_t* req)
{
	if(strcmp(req->method,"POST")!=0)
	{
		send_json_error(bev, 405, "只支持POST方法");
        return;
	}

	if(!req->body || req->body_length == 0)
	{
		send_json_error(bev, 400, "请求体为空");
        return;
	}

	cJSON* root = cJSON_Parse(req->body);
	if (!root) {
        send_json_error(bev, 400, "无效的JSON格式");
        return;
    }

	cJSON* device_id_json=cJSON_GetObjectItem(root,"device_id");
	cJSON* longitude_json=cJSON_GetObjectItem(root,"longitude");
	cJSON* latitude_json=cJSON_GetObjectItem(root,"latitude");
	cJSON* temperature_json=cJSON_GetObjectItem(root,"temperature");
	cJSON* humidity_json = cJSON_GetObjectItem(root, "humidity");
    cJSON* distance_json = cJSON_GetObjectItem(root, "distance");

	char device_id[32]="unknown";
	if(device_id_json && device_id_json->valuestring)
	{
		strncpy(device_id,device_id_json->valuestring,sizeof(device_id)-1);
	}

	double longitude = longitude_json ? longitude_json->valuedouble : 0.0;
	double latitude = latitude_json ? latitude_json->valuedouble : 0.0;
	double temperature = temperature_json ? temperature_json->valuedouble : 0.0;
    double humidity = humidity_json ? humidity_json->valuedouble : 0.0;
    double distance = distance_json ? distance_json->valuedouble : 0.0;

	printf("[INFO] 设备上报数据: device_id=%s\n", device_id);
    printf("  经度: %.6f\n", longitude);
    printf("  纬度: %.6f\n", latitude);
    printf("  温度: %.2f°C\n", temperature);
    printf("  湿度: %.2f%%\n", humidity);
    printf("  障碍物距离: %.2f米\n", distance);

	int db_result = db_mysql_insert_device_data(device_id, longitude, latitude, 
                                               temperature, humidity, distance);

	if (db_result != 0) {
        printf("[ERROR] 设备数据保存到数据库失败\n");
        send_json_error(bev, 500, "数据保存失败");
        cJSON_Delete(root);
        return;
    }

	char response_json[256];
    snprintf(response_json, sizeof(response_json),
        "{\"code\":0,\"msg\":\"data_received\",\"device_id\":\"%s\"}",
        device_id);

	send_json_ok(bev, response_json);
    printf("[INFO] 设备上报处理完成\n");

	cJSON_Delete(root);
}

//设备软件更新处理
void device_update_handler(struct bufferevent* bev, const http_request_t* req)
{
	if (strcmp(req->method, "GET") != 0) {
        send_json_error(bev, 405, "只支持GET方法");
        return;
    }

	printf("[INFO] 设备请求软件更新\n");

	char version[32] = {0};
    char download_url[512] = {0};
    long file_size = 0;
    char md5_hash[33] = {0};

	int db_result = db_mysql_get_latest_firmware(version, sizeof(version),
                                                download_url, sizeof(download_url),
                                                &file_size, md5_hash, sizeof(md5_hash));
	
	if(db_result != 0)
	{
		printf("[ERROR] 获取固件信息失败，使用默认值\n");
		strcpy(version, "1.0.0");
        strcpy(download_url, "http://server-ip:8000/files/update_v1.0.0.bin");
        file_size = 1048576;
        strcpy(md5_hash, "d41d8cd98f00b204e9800998ecf8427e");
	}

	char response_json[1024];
	snprintf(response_json,sizeof(response_json),
		"{\"code\":0,\"msg\":\"ok\",\"data\":{"
        "\"latest_version\":\"%s\","
        "\"download_url\":\"%s\""
        "}}",
		version,download_url);

	send_json_ok(bev, response_json);

	printf("[INFO] 返回固件信息: 版本=%s\n", version);
}

void device_download_handler(struct bufferevent* bev, const http_request_t* req)
{
	if (strcmp(req->method, "GET") != 0) {
        send_json_error(bev, 405, "只支持GET方法");
        return;
    }

	char* filename = NULL;

	const char* path = req->uri;
	if(strncmp(path,"/files/",7)==0)
	{
		filename=strdup(path+7);
	}

	if(!filename || strlen(filename) == 0)
	{
		send_json_error(bev, 400, "缺少文件名");
        printf("[WARN] 文件下载：缺少文件名\n");
        if (filename) free(filename);
        return;
	}

	printf("[INFO] 设备请求下载文件: %s\n", filename);

	char filepath[1024];
	snprintf(filepath,sizeof(filepath),"files/%s",filename);

	//检查文件是否存在
	FILE* file = fopen(filepath,"rb");
	if (!file) {
        send_json_error(bev, 404, "文件不存在");
        printf("[ERROR] 文件不存在: %s\n", filepath);
        free(filename);
        return;
    }

	fseek(file,0,SEEK_END);
	long file_size = ftell(file);
	fseek(file,0,SEEK_SET);

	char header[512];
	snprintf(header,sizeof(header),
		"HTTP/1.1 200 OK\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n\r\n",
		file_size);

	bufferevent_write(bev,header,strlen(header));

	char buffer[8192];
	size_t bytes_read;

	while((bytes_read = fread(buffer,1,sizeof(buffer),file))>0)
	{
		bufferevent_write(bev,buffer,bytes_read);
	}

	fclose(file);
	free(filename);

    printf("[INFO] 文件发送完成: %s, 大小=%ld字节\n", filepath, file_size);
}