#ifndef UTILS_H
#define UTILS_H

#include<string.h>
#include <event2/bufferevent.h>
#include <stddef.h>

void send_json_ok(struct bufferevent* bev, const char* json_body);

void send_json_error(struct bufferevent* bev, int code, const char* message);

char* trim_string(char *str);

#endif