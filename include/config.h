#ifndef CONFIG_H
#define CONFIG_H

//定义服务器配置结构体
typedef struct
{
	int server_port;
	int db_port;
	char db_host[64];
	char db_user[32];
	char db_password[32];
	char db_name[32];
}server_config_t;

int load_config(server_config_t* config);

#endif