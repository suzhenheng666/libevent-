#ifndef HTTP_ROUTES_H
#define HTTP_ROUTES_H

#include <event2/bufferevent.h>
#include "http_parse.h"

typedef void (*route_handler_t)(struct bufferevent* bev,const http_request_t* req);

void http_routes_init(void);

void http_dispatch_request(struct bufferevent* bev,const http_request_t* req);

#endif