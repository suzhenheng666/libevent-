#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
void send_json_ok(struct bufferevent* bev,const char* json_body)
{
	if(!bev || !json_body)
	{
		return;
	}

	size_t body_len=strlen(json_body);

	/* 构建HTTP响应头部 */
	char header[256];
	snprintf(header,sizeof(256),
	"HTTP/1.1 200 OK\r\n"
	"Content-Type: application/json\r\n"
	"Content-Lenngth: %zu\r\n"
	"\r\n",
	body_len);

	/* 发送响应 */
	bufferevent_write(bev,header,strlen(header));
	bufferevent_write(bev,json_body,body_len);

	printf("[DEBUG] 发送JSON响应，长度: %zu\n", body_len);

}

void send_json_error(struct bufferevent* bev, int code, const char* message)
{
	if (!bev || !message) {
        return;
    }

	char json_body[512];
	snprintf(json_body, sizeof(json_body),
        "{\"error\":%d,\"message\":\"%s\"}",
        code, message);
	
	size_t body_len = strlen(json_body);

	/* 构建HTTP响应头部 */
	char header[256];
	snprintf(header, sizeof(header),
        "HTTP/1.1 %d Error\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "\r\n",
        code, body_len);
	
	/* 发送响应 */
    bufferevent_write(bev, header, strlen(header));
	bufferevent_write(bev, json_body, body_len);

	printf("[DEBUG] 发送错误响应: %d - %s\n", code, message);
}

char* trim_string(char *str)
{
	if (!str) {
        return NULL;
    }

	while(*str==' '||*str=='\t')
	{
		str++;
	}
	if(*str=='\0')
	{
		return NULL;
	}

	char *end=str+strlen(str)-1;
	while(end>str && (*end==' ' || *end== '\t'))
	{
		*end='\0';
		end--;
	}

	return str;
}