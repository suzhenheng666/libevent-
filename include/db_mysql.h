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


//插入设备数据
int db_mysql_insert_device_data(const char* device_id,
                                double longitude,
                                double latitude,
                                double temperature,
                                double humidity,
                                double distance);

/* 固件版本查询 */
int db_mysql_get_latest_firmware(char* version, int version_len,
                                 char* download_url,
                                  int url_len,
                                 long* file_size, 
                                 char* md5_hash, 
                                 int md5_len);

/* 设备数据查询 */
device_data_t* db_mysql_get_all_devices_latest(int* count);
time_series_data_t* db_mysql_get_device_temperature_history(const char* device_id, int* count);
time_series_data_t* db_mysql_get_device_humidity_history(const char* device_id, int* count);


#endif