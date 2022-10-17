#ifndef _HTTP_HANDLE_H
#define _HTTP_HANDLE_H
#include <string>
#include <map>

std::map<std::string, std::string> content_type_table = {
    { "txt", "text/plain; charset=utf-8" },
	{ "c", "text/plain; charset=utf-8" },
	{ "h", "text/plain; charset=utf-8" },
	{ "html", "text/html" },
	{ "htm", "text/htm" },
	{ "css", "text/css" },
	{ "gif", "image/gif" },
	{ "jpg", "image/jpeg" },
	{ "jpeg", "image/jpeg" },
	{ "png", "image/png" },
	{ "pdf", "application/pdf" },
	{ "ps", "application/postscript" }
};

class http_handle {
private:   
    char method[256];
    char url[256];
    char post_data[2048];

    int connfd;
public:
    http_handle(int fd) : connfd(fd) {}
    static void* handle_http_request(void*);
    static void* response_http_request(void*);
private:
    void _handle_http_request();
    void _response_http_request();
    void trans_file(const std::string&);
    void trans_dir(const std::string&);
    void get_line(char*, int);

    bool send_http_header(const std::string&);    //send start line & headers
    void file_not_supported();

    

    void unimpelented();
public:
    static std::string path;
};

#endif