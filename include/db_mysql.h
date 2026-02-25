#ifndef DB_MYSQL_H
#define DB_MYSQL_H

#include<mysql/mysql.h>

/* 设备数据结构 */
typedef struct{
	char device_id[32];
	double longitude;
	double latitude;
	double temperature;
    double humidity;
	double obstacle_distance;
    char timestamp[20];
}device_data_t;

/* 时间序列数据结构 */
typedef struct 
{
	char time[8];		/* 时间格式：HH:MM */
	char value[16];
}time_series_data_t;



int db_mysql_init(const char* host,
	const char* user,
	const char* password,
	const char* database);

int db_mysql_close();

//Todo
//还缺少结构体的定义，数据库的查询和插入操作

#endif