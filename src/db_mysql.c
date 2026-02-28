#include "db_mysql.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* MySQL连接句柄 */
static MYSQL* mysql_conn = NULL;

/* ==================== 数据库连接管理 ==================== */

int db_mysql_init(const char* host, 
    const char* user, 
    const char* password, 
    const char* database)
{
    if (mysql_conn != NULL) {
        printf("[DB] 数据库已连接\n");
        return 0;
    }
    
    /* 初始化MySQL连接 */
    mysql_conn = mysql_init(NULL);
    if (mysql_conn == NULL) {
        fprintf(stderr, "[DB ERROR] 初始化MySQL失败\n");
        return -1;
    }
    
    /* 连接到数据库 */
    printf("[DB] 正在连接到数据库...\n");
    
    if (mysql_real_connect(mysql_conn, host, user, password, 
                          database, 0, NULL, 0) == NULL) {
        fprintf(stderr, "[DB ERROR] 连接数据库失败: %s\n", mysql_error(mysql_conn));
        mysql_close(mysql_conn);
        mysql_conn = NULL;
        return -1;
    }
    
    printf("[DB] 数据库连接成功\n");
    return 0;
}

void db_mysql_close(void)
{
    if (mysql_conn != NULL) {
        mysql_close(mysql_conn);
        mysql_conn = NULL;
    }
}

/* ==================== 设备数据插入 ==================== */

int db_mysql_insert_device_data(const char* device_id,
                                double longitude,
                                double latitude,
                                double temperature,
                                double humidity,
                                double distance)
{
    if (mysql_conn == NULL) {
        fprintf(stderr, "[DB ERROR] 数据库未连接\n");
        return -1;
    }
    
    char query[1024];
    
    /* 插入或更新最新数据表 */
    snprintf(query, sizeof(query),
        "INSERT INTO device_latest_data "
        "(device_id, longitude, latitude, temperature, humidity, obstacle_distance) "
        "VALUES ('%s', %.6f, %.6f, %.2f, %.2f, %.2f) "
        "ON DUPLICATE KEY UPDATE "
        "longitude = %.6f, "
        "latitude = %.6f, "
        "temperature = %.2f, "
        "humidity = %.2f, "
        "obstacle_distance = %.2f, "
        "report_time = NOW()",
        device_id, longitude, latitude, temperature, humidity, distance,
        longitude, latitude, temperature, humidity, distance);
    
    if (mysql_query(mysql_conn, query) != 0) {
        fprintf(stderr, "[DB ERROR] 更新最新数据失败: %s\n", mysql_error(mysql_conn));
        return -1;
    }
    
    /* 插入历史数据表 */
    snprintf(query, sizeof(query),
        "INSERT INTO device_data_history "
        "(device_id, longitude, latitude, temperature, humidity, obstacle_distance) "
        "VALUES ('%s', %.6f, %.6f, %.2f, %.2f, %.2f)",
        device_id, longitude, latitude, temperature, humidity, distance);
    
    if (mysql_query(mysql_conn, query) != 0) {
        fprintf(stderr, "[DB ERROR] 插入历史数据失败: %s\n", mysql_error(mysql_conn));
        return -1;
    }
    
    return 0;
}

/* ==================== 固件版本查询 ==================== */

int db_mysql_get_latest_firmware(char* version, int version_len,
                                 char* download_url, int url_len,
                                 long* file_size, char* md5_hash, int md5_len)
{
    if (mysql_conn == NULL) {
        fprintf(stderr, "[DB ERROR] 数据库未连接\n");
        return -1;
    }
    
    const char* query = 
        "SELECT version, download_url, file_size, md5_hash "
        "FROM firmware_versions "
        "ORDER BY created_at DESC "
        "LIMIT 1";
    
    if (mysql_query(mysql_conn, query) != 0) {
        fprintf(stderr, "[DB ERROR] 查询固件版本失败: %s\n", mysql_error(mysql_conn));
        return -1;
    }
    
    MYSQL_RES* result = mysql_store_result(mysql_conn);
    if (result == NULL) {
        fprintf(stderr, "[DB ERROR] 获取查询结果失败\n");
        return -1;
    }
    
    int row_count = mysql_num_rows(result);
    if (row_count == 0) {
        mysql_free_result(result);
        return -1;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        if (version && row[0]) {
            strncpy(version, row[0], version_len - 1);
            version[version_len - 1] = '\0';
        }
        
        if (download_url && row[1]) {
            strncpy(download_url, row[1], url_len - 1);
            download_url[url_len - 1] = '\0';
        }
        
        if (file_size && row[2]) {
            *file_size = atol(row[2]);
        }
        
        if (md5_hash && row[3]) {
            strncpy(md5_hash, row[3], md5_len - 1);
            md5_hash[md5_len - 1] = '\0';
        }
    }
    
    mysql_free_result(result);
    return row ? 0 : -1;
}

/* ==================== 设备数据查询 ==================== */

device_data_t* db_mysql_get_all_devices_latest(int* count)
{
    if (mysql_conn == NULL) {
        fprintf(stderr, "[DB ERROR] 数据库未连接\n");
        return NULL;
    }
    
    const char* query = 
        "SELECT d.device_id, "
        "ld.longitude, ld.latitude, ld.temperature, ld.humidity, "
        "ld.obstacle_distance, ld.report_time "
        "FROM devices d "
        "LEFT JOIN device_latest_data ld ON d.device_id = ld.device_id "
        "ORDER BY d.device_id";
    
    if (mysql_query(mysql_conn, query) != 0) {
        fprintf(stderr, "[DB ERROR] 查询设备数据失败: %s\n", mysql_error(mysql_conn));
        return NULL;
    }
    
    MYSQL_RES* result = mysql_store_result(mysql_conn);
    if (result == NULL) {
        fprintf(stderr, "[DB ERROR] 获取查询结果失败\n");
        return NULL;
    }
    
    int num_rows = mysql_num_rows(result);
    if (num_rows == 0) {
        mysql_free_result(result);
        *count = 0;
        return NULL;
    }
    
    /* 分配内存存储结果 */
    device_data_t* devices = (device_data_t*)calloc(num_rows, sizeof(device_data_t));
    if (devices == NULL) {
        fprintf(stderr, "[DB ERROR] 内存分配失败\n");
        mysql_free_result(result);
        return NULL;
    }
    
    /* 遍历结果集 */
    int i = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) && i < num_rows) {
        if (row[0]) {
            strncpy(devices[i].device_id, row[0], sizeof(devices[i].device_id) - 1);
            devices[i].device_id[sizeof(devices[i].device_id) - 1] = '\0';
        }
        devices[i].longitude = row[1] ? atof(row[1]) : 0.0;
        devices[i].latitude = row[2] ? atof(row[2]) : 0.0;
        devices[i].temperature = row[3] ? atof(row[3]) : 0.0;
        devices[i].humidity = row[4] ? atof(row[4]) : 0.0;
        devices[i].obstacle_distance = row[5] ? atof(row[5]) : 0.0;
        
        if (row[6]) {
            strncpy(devices[i].timestamp, row[6], sizeof(devices[i].timestamp) - 1);
            devices[i].timestamp[sizeof(devices[i].timestamp) - 1] = '\0';
        } else {
            strcpy(devices[i].timestamp, "");
        }
        
        i++;
    }
    
    *count = i;
    mysql_free_result(result);
    return devices;
}

/* ==================== 温度历史数据查询 ==================== */

time_series_data_t* db_mysql_get_device_temperature_history(const char* device_id, int* count)
{
    if (mysql_conn == NULL) {
        fprintf(stderr, "[DB ERROR] 数据库未连接\n");
        return NULL;
    }
    
    char query[1024];
    snprintf(query, sizeof(query),
        "SELECT DATE_FORMAT(report_time, '%%H:%%i') AS time, "
        "temperature "
        "FROM device_data_history "
        "WHERE device_id = '%s' "
        "AND report_time >= DATE_SUB(NOW(), INTERVAL 10 MINUTE) "
        "ORDER BY report_time ASC",
        device_id);
    
    printf("[DEBUG] 执行查询: %s\n", query);
    
    if (mysql_query(mysql_conn, query) != 0) {
        fprintf(stderr, "[DB ERROR] 查询温度历史失败: %s\n", mysql_error(mysql_conn));
        return NULL;
    }
    
    MYSQL_RES* result = mysql_store_result(mysql_conn);
    if (result == NULL) {
        fprintf(stderr, "[DB ERROR] 获取查询结果失败\n");
        return NULL;
    }
    
    int num_rows = mysql_num_rows(result);
    printf("[DEBUG] 查询返回 %d 行数据\n", num_rows);
    
    if (num_rows == 0) {
        // 额外检查：是否存在该设备的数据（不限时间）
        char check_query[512];
        snprintf(check_query, sizeof(check_query),
            "SELECT COUNT(*) as total_count, "
            "MAX(report_time) as latest_time "
            "FROM device_data_history "
            "WHERE device_id = '%s'", device_id);
        
        if (mysql_query(mysql_conn, check_query) == 0) {
            MYSQL_RES* check_result = mysql_store_result(mysql_conn);
            if (check_result) {
                MYSQL_ROW row = mysql_fetch_row(check_result);
                if (row) {
                    printf("[DEBUG] 设备 %s 总数据条数: %s, 最新数据时间: %s\n", 
                           device_id, row[0], row[1] ? row[1] : "NULL");
                }
                mysql_free_result(check_result);
            }
        }
        
        mysql_free_result(result);
        *count = 0;
        return NULL;
    }
    
    /* 分配内存存储结果 */
    time_series_data_t* data = (time_series_data_t*)calloc(num_rows, sizeof(time_series_data_t));
    if (data == NULL) {
        fprintf(stderr, "[DB ERROR] 内存分配失败\n");
        mysql_free_result(result);
        return NULL;
    }
    
    /* 遍历结果集 */
    int i = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) && i < num_rows) {
        if (row[0]) {
            strncpy(data[i].time, row[0], sizeof(data[i].time) - 1);
            data[i].time[sizeof(data[i].time) - 1] = '\0';
        }
        
        if (row[1]) {
            snprintf(data[i].value, sizeof(data[i].value), "%.2f", atof(row[1]));
            printf("[DEBUG] 第%d条数据: 时间=%s, 温度=%s\n", i+1, row[0], row[1]);
        }
        
        i++;
    }
    
    *count = i;
    printf("[DEBUG] 实际处理 %d 条数据\n", i);
    mysql_free_result(result);
    return data;
}

/* ==================== 湿度历史数据查询 ==================== */

time_series_data_t* db_mysql_get_device_humidity_history(const char* device_id, int* count)
{
    if (mysql_conn == NULL) {
        fprintf(stderr, "[DB ERROR] 数据库未连接\n");
        return NULL;
    }
    
    char query[1024];
    snprintf(query, sizeof(query),
        "SELECT DATE_FORMAT(report_time, '%%H:%%i') AS time, "
        "humidity "
        "FROM device_data_history "
        "WHERE device_id = '%s' "
        "AND report_time >= DATE_SUB(NOW(), INTERVAL 10 MINUTE) "
        "ORDER BY report_time ASC",
        device_id);
    
    if (mysql_query(mysql_conn, query) != 0) {
        fprintf(stderr, "[DB ERROR] 查询湿度历史失败: %s\n", mysql_error(mysql_conn));
        return NULL;
    }
    
    MYSQL_RES* result = mysql_store_result(mysql_conn);
    if (result == NULL) {
        fprintf(stderr, "[DB ERROR] 获取查询结果失败\n");
        return NULL;
    }
    
    int num_rows = mysql_num_rows(result);
    if (num_rows == 0) {
        mysql_free_result(result);
        *count = 0;
        return NULL;
    }
    
    /* 分配内存存储结果 */
    time_series_data_t* data = (time_series_data_t*)calloc(num_rows, sizeof(time_series_data_t));
    if (data == NULL) {
        fprintf(stderr, "[DB ERROR] 内存分配失败\n");
        mysql_free_result(result);
        return NULL;
    }
    
    /* 遍历结果集 */
    int i = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) && i < num_rows) {
        if (row[0]) {
            strncpy(data[i].time, row[0], sizeof(data[i].time) - 1);
            data[i].time[sizeof(data[i].time) - 1] = '\0';
        }
        
        if (row[1]) {
            snprintf(data[i].value, sizeof(data[i].value), "%.2f", atof(row[1]));
        }
        
        i++;
    }
    
    *count = i;
    mysql_free_result(result);
    return data;
}