#include "http_parse.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

const char* http_get_header(const http_request_t* req, const char* key)
{
    if (!req || !key) return NULL;
	for(header_node_t* p=req->headers;p;p=p->next)
	{
		if(strcmp(p->name,key)==0)
		{
			return p->value;
		}
	}
}

static void add_header(header_node_t** head, const char* name, const char* value)
{
	header_node_t* node=malloc(sizeof(header_node_t));
	node->name=name;
	node->value=value;
	node->next=*head;
	*head=node;
}

static int parse_request_line(http_request_t* req, char* line)
{
	char* method=strtok(line," ");
	char* uri=strtok(NULL," ");
	char* version=strtok(NULL,"\r\n");
	if (!method || !uri || !version) {
        return -1;
    }

	strncpy(req->method,method,sizeof(req->method)-1);
	req->method[sizeof(req->method)-1]='\0';

	strncpy(req->uri, uri, sizeof(req->uri) - 1);
    req->uri[sizeof(req->uri) - 1] = '\0';

    strncpy(req->http_version, version, sizeof(req->http_version) - 1);
    req->http_version[sizeof(req->http_version) - 1] = '\0';

	char* qmark=strchr(line,'?');
	if(qmark)
	{
		*qmark='\0';
		strncpy(req->path,uri,sizeof(req->path)-1);
		strncpy(req->query,qmark+1,sizeof(req->query)-1);
		*qmark='?';
		req->path[sizeof(req->path) - 1] = '\0';
    	req->query[sizeof(req->query) - 1] = '\0';
	}
	else
	{
		strncpy(req->path, uri, sizeof(req->path) - 1);
		req->query[0] = '\0';
	}

	return 0;
}

http_request_t* http_parse_request(const char* raw, size_t len)
{
    /* 1. 创建请求对象 */
	http_request_t* req=calloc(1,sizeof(http_request_t));
	if(!req)
	{
		return NULL;
	}
	/* 2. 复制数据到临时缓冲区（方便处理） */
	char* data=malloc(len+1);
	if (!data) {
        free(req);
        return NULL;
    }
	memcpy(data,raw,len);
	data[len]='\0';

	/* 3. 查找头部结束位置（\r\n\r\n） */
	char* header_end=strstr(data,"\r\n\r\n");
	if(!header_end)
	{
        printf("[ERROR] HTTP头部不完整\n");
        free(data);
        free(req);
        return NULL;
    }
	/* 4. 标记头部结束，方便解析 */
	*header_end='\0';
	char* body_start=header_end+4;

	/* 5. 解析请求行（第一行） */
	char* line=data;
	char* next_line=line;
	char* line_end=strstr(line,"\r\n");
	if (!line_end) {
        printf("[ERROR] 找不到请求行结束\n");
        free(data);
        free(req);
        return NULL;
    }

	*line_end='\0';
	if(parse_request_line(req,line)<0)
	{
		printf("[ERROR] 解析请求行失败\n");
        free(data);
        free(req);
        return NULL;
	}
	*line_end='\r';

	/* 6. 解析头部字段 */
	next_line=line+2;
	while(next_line<header_end)
	{
		char* current_line_end=strstr(next_line,"\r\n");

		if(!current_line_end || current_line_end >= header_end)
		{
			current_line_end=header_end;
		}

		size_t line_len=current_line_end-next_line;
		if(line_len == 0)
		{
			break;
		}

		char* header_line = malloc(line_len+1);
		if (!header_line) {
            break;
        }
		memcpy(header_line,next_line,line_len);
		header_line[line_len]='\0';

		char* colon=strchr(header_line,':');
		if(colon)
		{
			*colon='\0';
			char* name=trim_string(header_line);
			char* value=trim_string(colon+1);

			if(*name && *value)
			{
				add_header(&req->headers,name,value);
			}

			*colon=':';
		}
		free(header_line);
		if(current_line_end >= header_end)
		{
			break;
		}

		next_line=current_line_end+2;
	}

	/* 7. 解析请求体（如果有） */
	const char* contnent_len = http_get_header(req,"Content-Length");
	if(contnent_len)
	{
		size_t body_len=atoi(contnent_len);
		size_t body_available = len - (body_start-data);

		if(body_len <= body_available)
		{
			req->body=malloc(body_len+1);
			if(req->body)
			{
				memcpy(req->body,body_start,body_len);
				req->body[body_len]='\0';
				req->body_length=body_len;
			}
		}
	}

	/* 8. 清理临时缓冲区 */
	free(data);
	return req;
}

void http_free_request(http_request_t* req)
{
    if (!req) return;
	
	header_node_t* current=req->headers;
	while(current)
	{
		header_node_t* next=current->next;
		free(current->name);
		free(current->value);
		current=next;
	}

	if(req->body)
	{
		free(req->body);
	}
	free(req);
}

char* http_get_query_param(const http_request_t* req, const char* key)
{
	if (!req || !key || req->query[0] == '\0') {
        return NULL;
    }

	char query_copy[256];
	strncpy(query_copy,req->query,sizeof(query_copy)-1);
	query_copy[sizeof(query_copy)-1]='\0';

	char* token=strtok(query_copy,"&");
	while (token)
	{
		char* equals=strchr(token,'=');
		if(equals)
		{
			*equals='\0';
			if(strcmp(token,key)==0)
			{
				char *value=strdup(equals+1);
				*equals='=';
				return value;
			}

			*equals='=';
		}
		token=strtok(NULL,"&");
	}

	return NULL;
}