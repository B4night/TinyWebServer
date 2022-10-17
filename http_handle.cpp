#include "http_handle.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <map>
#include <dirent.h>

std::string http_handle::path;


void http_handle::get_line(char* tmp, int size)
{
    char ch;
    int n;
    for (int i = 0; i < size; i++) {
        n = recv(this->connfd, &ch, 1, 0);
        if (n > 0) {
            tmp[i] = ch;
            if (ch == '\n' && i && tmp[i - 1] == '\r') {
                tmp[i + 1] = 0;
                return;
            } else {
                fprintf(stderr, "get_line recv() error\n");
                exit(1);
            }
        }
    }
    
    fprintf(stderr, "http_handle http request is out of limit 1024B\n");
    exit(1);
}

void http_handle::unimpelented() {
    int client = this->connfd;
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

void http_handle::_handle_http_request() {
    char tmp[1024];
    int n = read(this->connfd, tmp, 1024);
    if (n == -1) {
        fprintf(stderr, "read error\n");
        exit(1);
    }
    int i, j;
    for (i = 0; tmp[i] != ' '; i++)
        method[i] = tmp[i];
    method[i] = 0;

    for (j = 0, ++i; tmp[i] != ' '; i++, j++)
        url[j] = tmp[i];
    url[j] = 0;

    if (!strcasecmp(method, "GET")) {
        while ((n = read(this->connfd, tmp, 1024)) != 0)
            ;
    }
    if (!strcasecmp(method, "POST")) {
        while (true) {
            get_line(tmp, sizeof(tmp) / sizeof(tmp[0]));
            if (strcmp(tmp, "\r\n") == 0)
                break;
            
            n = recv(this->connfd, this->post_data, 2048, 0);
            if (n <= 0) {
                fprintf(stderr, "_handle_http_request error\n");
                exit(1);
            }
        }
    }
}

void http_handle::_response_http_request() {
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST")) {
        unimpelented();
        return;
    }
    if (!strcasecmp(method, "GET")) {
        if (strcmp(url, "/") == 0) {
            char tmp[256];
            http_handle::path = getcwd(tmp, 256);
            trans_file("src");
            return;
        }
        std::string filepath = http_handle::path + "\\" + url;
        struct stat filestat;
        if ((stat(filepath.c_str(), &filestat)) != 0) {
            perror("_response_http_request stat error");
            exit(1);
        }
        switch (filestat.st_mode & S_IFMT) {
            case S_IFREG:
                trans_file(filepath);
                break;
            case S_IFDIR:
                trans_dir(filepath);
                break;
            default:
                break;
        }
        return;    
    }
    if (!strcasecmp(method, "POST")) {  //to be implemented

        return;
    }
}

void* http_handle::handle_http_request(void* arg) {
    http_handle* hp = (http_handle*)arg;
    hp->_handle_http_request();
    return hp;
}

void* http_handle::response_http_request(void* arg) {
    http_handle* hp = (http_handle*)arg;
    hp->_response_http_request();
    return hp;
}

void http_handle::file_not_supported() {
    char buf[1024];
    int client = this->connfd;

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>file requested is not supported\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request file type not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

bool http_handle::send_http_header(const std::string& filepath) {
    char buf[1024];
    int idx = filepath.find('.');
    if (idx == -1) {
        file_not_supported();
        return false;
    }
    std::string sub = filepath.substr(idx + 1, filepath.size() - idx);
    if (content_type_table.count(sub) == 0) {
        file_not_supported();
        return false;
    }

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(this->connfd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: ");
    strcat(buf + strlen(buf), content_type_table[sub].c_str());
    sprintf(buf, "\r\n");
    send(this->connfd, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(this->connfd, buf, strlen(buf), 0);

    return true;
}

void http_handle::trans_file(const std::string& filepath) {
    char buf[1024];
    FILE* f = fopen(filepath.c_str(), "r");
    
    if (send_http_header(filepath) == false) {
        fclose(f);
        return;
    }
    
    do {
        fgets(buf, 1024, f);
        send(this->connfd, buf, strlen(buf), 0);
    } while (!feof(f));

    fclose(f);
}

void http_handle::trans_dir(const std::string& filepath) {
    char buf[1024];
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    send(this->connfd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html; charset=utf-8\r\n\r\n");
    send(this->connfd, buf, strlen(buf), 0);

    sprintf(buf, "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"utf-8\">\n<title>main page</title>\n</head>\n<body>\n");
    send(this->connfd, buf, strlen(buf), 0);

    DIR* dir = opendir(filepath.c_str());
    struct dirent* d;
    while ((d = readdir(dir)) != NULL) {
        std::string str = http_handle::path + "/" + d->d_name;
        sprintf(buf, "<a href=\"%s\">%s</a><br>", str.c_str(), d->d_name);
        send(this->connfd, buf, strlen(buf), 0);
    }
    sprintf(buf, "</body>\n</html>\n");
    send(this->connfd, buf, strlen(buf), 0);
    http_handle::path = filepath;
}