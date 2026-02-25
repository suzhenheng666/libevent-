#include<stdio.h>
#include"config.h"
#include"db_mysql.h"
#include"http_routes.h"
#include"http_server.h"
#include"signal_handler.h"

int main()
{
	//1.加载配置文件
	server_config_t config;
	if(load_config(&config)!=0)
	{
		printf("[INFO]使用默认配置\n");
	}
	else
	{
		printf("[INFO]配置文件加载成功\n");
	}

	//2.初始化数据库
	if(db_mysql_init(config.db_host,config.db_user,config.db_password,config.db_name)!=0)
	{
		fprintf(stderr,"[ERROR]数据库初始化失败\n");
		return 1;
	}

	//3.初始化路由系统
	http_routes_init();

	//4.创建http服务器
	http_server_t* server=http_server_new(config.server_port);
	if(!server)
	{
		fprintf(stderr,"[ERROR]创建http服务器失败\n");
		db_mysql_close();
		return 1;
	}

	//5.初始化信号处理器
	signal_handler_init(server);

	//6.启动服务器（阻塞运行）
	if(http_server_start(server)!=0)
	{
		fprintf(stderr,"[ERROR]启动http服务器失败\n");
		http_server_free(server);
		db_mysql_close();
		return 1;
	}
	
	//7.清理工作(理论上不会到达这里)
	http_server_free(server);
	db_mysql_close();

	return 0;
}