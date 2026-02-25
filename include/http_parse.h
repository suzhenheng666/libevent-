#ifndef HTTP_PARSE_H
#define HTTP_PARSE_H

#include<stddef.h>

/* HTTP头部链表节点 */
typedef struct header_node_s
{
	char* name;
	char* value;
	struct header_node_s* next;
}header_node_t;

/* HTTP请求结构体 */
typedef struct http_request_s
{
	char method[16];
	char uri[256];
	char path[256];
	char query[256];
	char http_version[16];

	header_node_t* headers;

	char* body;
	size_t body_length;
}http_request_t;

//解析原始HTTP请求
http_request_t* http_parse_request(const char* raw,size_t len);

//释放HTTP请求对象
void http_free_request(http_request_t* req);

//获取HTTP头部字段值
const char* http_get_header(const http_request_t* req,const char* key);

//从查询字符串中获取参数值
char* http_get_query_param(const http_request_t* req,const char* key);

#endif