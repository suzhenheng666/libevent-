#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int load_config(server_config_t* config)
{
    if (!config) {
        return -1;
    }
    
    /* 设置默认值 */
    config->server_port = 8080;
    strcpy(config->db_host, "127.0.0.1");
    strcpy(config->db_user, "root");
    strcpy(config->db_password, "123456");
    strcpy(config->db_name, "iot_monitoring");

    const char* filename = "../config/server.conf";
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        /* 文件不存在，使用默认值 */
        return -1;
    }
    
    char line[256];
     
    while (fgets(line, sizeof(line), fp)) {
        /* 去除换行符 */
        // 在字符串 line 中查找换行符 \n 第一次出现的位置, 返回一个指向 \n 字符的指针
        char* newline = strchr(line, '\n');
        if (newline) *newline = '\0';               // 将 \n 替换为字符串结束符
        
        /* 跳过空行和注释 */
        if (strlen(line) == 0 || line[0] == '#') {
            continue;
        }
        
        /* 分割 key=value */
        char* equals = strchr(line, '=');
        // 如果没找到, equals 为 NULL
        if (!equals) {
            continue;  /* 跳过格式错误的行 */
        }
        
        *equals = '\0';  /* 临时截断 */
        char* key = line;
        char* value = equals + 1;
        
        /* 解析配置项 */
        if (strcmp(key, "server_port") == 0) {
            config->server_port = atoi(value);  // 将字符串转换为整数
            if (config->server_port < 1 || config->server_port > 65535) {
                config->server_port = 8000;  /* 无效则使用默认值 */
            }
        }
        else if (strcmp(key, "db_host") == 0) {
            // 在TCP/IP协议头中，端口号使用 16位（2字节） 的无符号整数表示
            strncpy(config->db_host, value, sizeof(config->db_host) - 1);
            config->db_host[sizeof(config->db_host) - 1] = '\0';
        }
        else if (strcmp(key, "db_user") == 0) {
            strncpy(config->db_user, value, sizeof(config->db_user) - 1);
            config->db_user[sizeof(config->db_user) - 1] = '\0';
        }
        else if (strcmp(key, "db_password") == 0) {
            strncpy(config->db_password, value, sizeof(config->db_password) - 1);
            config->db_password[sizeof(config->db_password) - 1] = '\0';
        }
        else if (strcmp(key, "db_name") == 0) {
            strncpy(config->db_name, value, sizeof(config->db_name) - 1);
            config->db_name[sizeof(config->db_name) - 1] = '\0';
        }
    }
    
    fclose(fp);
    return 0;
}

// TODO 加trim_string