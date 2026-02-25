#include"db_mysql.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

/* MySQL连接句柄 */
static MYSQL* mysql_conn=NULL;

int db_mysql_init(const char* host,
					const char* user,
					const char* password,
					const char* database)
{
	if(mysql_conn!=NULL)
	{
		printf("[DB] 数据库已连接\n");
        return 0;
	}

	/* 初始化MySQL连接 */
	mysql_conn = mysql_init(NULL);
	if(mysql_conn == NULL)
	{
		fprintf(stderr, "[DB ERROR] 初始化MySQL失败\n");
        return -1;
	}

	/* 连接到数据库 */
	printf("[DB] 正在连接到数据库...\n");

	if(mysql_real_connect(mysql_conn,host,user,password,database,0,NULL,0)==NULL)
	{
		fprintf(stderr, "[DB ERROR] 连接数据库失败: %s\n", mysql_error(mysql_conn));
        mysql_close(mysql_conn);
        mysql_conn = NULL;
        return -1;
	}
	printf("[DB] 数据库连接成功\n");
    return 0;
}

int db_mysql_close()
{
	
}