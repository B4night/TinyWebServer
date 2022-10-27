#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <map>
#include <dirent.h>

extern void sys_err(const char*);

// remenber to change cwd
std::string server = "/mnt/hgfs/TinyWebServer/src"; // current working directory
std::map<std::string, std::string> content_type_table = {   // 转换表
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


int get_line(char* src, char* dst, int len) {   // src中存储http报文共len字节, 将start line存储到dst中
    if (len <= 0)
        return -1;
    int i = 0;
    dst[i] = src[i];
    for (i = 1; i < len; i++) {
        dst[i] = src[i];
        if (dst[i] == '\n' && dst[i - 1] == '\r') {
            dst[i - 1] = '\0';
            return i;
        }
    }    
    return -1;
}

void unimplemented(int client)  // 未实现method
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

void header(int fd, const char* type) { // http报文的start line和headers
    char buf[1024];

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: %s\r\n", type);
    send(fd, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(fd, buf, strlen(buf), 0);
}

bool find_type(const char* filename, char* type) {  // 找到filename的类型, 在content_type_table中有就返回true
    int len = strlen(filename);
    int i, k;
    for (i = 0; i < len; i++)
        if (filename[i] == '.') // 如abc.txt, '.'后面的就是文件类型
            break;
    if (i == len)
        return false;
    i++;
    for (k = 0; i < len; i++, k++)  // type存储文件类型
        type[k] = filename[i]; 
    type[k] = 0;

    std::string tmp = type;
    if (content_type_table.count(tmp) == 0) // 在map中查找有无文件类型的对应类型
        return false;
    tmp = content_type_table[tmp];
    strcpy(type, tmp.c_str());
    return true;
}

void type_not_support(int fd) { // 不支持的文件类型
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Supported</TITLE>\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>This file type is not supported.\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(fd, buf, strlen(buf), 0);
}

void file_not_found(int fd) {   // 未找到文件
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not found</TITLE>\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>This file is not found.\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(fd, buf, strlen(buf), 0);
}

void serve_file(int fd, const char* filename) { // 传文件数据给fd
    int filefd = open(filename, O_RDWR);    // 根据文件名打开
    char type[256];
    if (find_type(filename, type) == false) {
        type_not_support(fd);
        return;
    }
    header(fd, type);   // 传start line和header
    char tmp[4096];
    int n;
    while ((n = read(filefd, tmp, 4096)) != 0)  // 传数据
        write(fd, tmp, n);
    close(filefd);
}

void serve_directory(int fd, const char* dirname) { // 传目录给fd
    char buf[4096] = { 0 };
    strcat(buf, "HTTP/1.1 200 OK\r\n");
    strcat(buf, "Content-Type: text/html; charset=utf-8\r\n\r\n");
    strcat(buf, "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"utf-8\">\n<title>main page</title>\n</head>\n<body>\n");
    write(fd, buf, strlen(buf));
    
    DIR* d = opendir(dirname);
    if (d == NULL)
        sys_err("opendir error");
    struct dirent* di;

    server = dirname;
    while ((di = readdir(d)) != NULL) {     // 使用readdir得到目录中的文件属性
        sprintf(buf, "<a href=\"%s\">%s</a><br>", (server + "/" + di->d_name).c_str(), di->d_name); // 链接
        write(fd, buf, strlen(buf));
    }

    sprintf(buf, "</body>\n</html>\n");
    write(fd, buf, strlen(buf));
}

void solve_request(int fd, char* buf, int len) {    // 处理http报文
    char method[64];    // 存储http start line的method
    char url[256];      // 存储http start line的url
    char tmp[1024];
    bzero(url, sizeof(url));
    bzero(method, sizeof(method));

    int tmp_len = get_line(buf, tmp, len);
    int i;
    for (i = 0; i < tmp_len && tmp[i] != ' '; i++) {
        method[i] = tmp[i];
    }
    i++;
    for (int k = 0; i < tmp_len && tmp[i] != ' '; i++, k++)
        url[k] = tmp[i];
    
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
        unimplemented(fd);
    if (strcasecmp(method, "GET") == 0) {    // method is GET
        if (strcmp(url, "/") == 0) {    // 此时说明是主目录
            serve_directory(fd, server.c_str());
            return;
        }

        struct stat st;
        if (stat(url, &st) == -1) {
            file_not_found(fd);
            return;
        }
        if (S_IFREG & st.st_mode)   // 根据url的类型选择传文件还是传目录
            serve_file(fd, url);    
        if (S_IFDIR & st.st_mode)
            serve_directory(fd, url);
    }
    if (strcasecmp(method, "POST") == 0) {
        // unimplemented
    }
}

void inactive_handler(int fd) { // 定时器回调函数调用的
    char buf[1024];
    sprintf(buf, "You are not active.\r\n");
    send(fd, buf, strlen(buf), 0);
}