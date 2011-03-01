#define _XOPEN_SOURCE 600

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFSIZE 1024

long path_len;

struct fdata {
    int fd;
    const char *filename;
    const char *full_filename;
    off_t offset;
    off_t size;
};

int open_file(struct fdata* fd)
{
    if (fd->fd != -1)
        close(fd->fd);
    printf("opening %s\n", fd->filename);
    fd->fd = open(fd->filename, O_RDONLY);
    return fd->fd;
}

bool data_to_read(struct fdata* fd)
{
    char fnbuf[path_len];
    ssize_t fnsiz;
    fnsiz = readlink(fd->filename, fnbuf, path_len - 1);
    if (fnsiz == -1) {
        perror("readlink");
        return -1;
    }
    fnbuf[fnsiz] = '\0';
    return fnsiz;
}

int tail_file(struct fdata* fd)
{
    struct stat stbuf;
    ssize_t read_s;
    char read_buf[BUFSIZE];

    fstat(fd->fd, &stbuf);
    if (fd->offset == -1) {
        fd->offset = 0;
    }
    if (fd->size == -1) {
        fd->size = stbuf.st_size;
    }

    lseek(fd->fd, fd->offset, SEEK_SET);
    read_s = read(fd->fd, &read_buf, BUFSIZE);
    if (read_s <  0) {
        perror("read");
        return -1;
    }
    write(STDOUT_FILENO, read_buf, read_s);
    fd->offset += read_s;
    if (fd->offset >= fd->size) {
        if (open_file(fd) < 0) {
            perror("open");
        }
    }
    return read_s;
}

int main(int argc, char** argv)
{
    int ep;
    struct epoll_event ev;
    struct fdata fd;
    int rv = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: ytail filename\n"
                "Behaves like tail -F filename\n");
    }

    path_len = _PC_PATH_MAX;
    printf("path_len=%d\n", path_len);

    fd.filename = argv[1];
    fd.fd = -1;
    fd.offset = -1;
    fd.size = -1;

    if ((ep = epoll_create(2)) == -1) {
        perror("epoll_crate");
        exit(EXIT_FAILURE);
    }
    ev.events = EPOLLIN;
    ev.data.fd = STDOUT_FILENO;
    if (epoll_ctl(ep, EPOLL_CTL_ADD, STDOUT_FILENO, &ev) == -1) {
        perror("epoll_ctl: stdout");
        exit(EXIT_FAILURE);
    }

    if (open_file(&fd) < 0) {
        perror("open");
    }
    while (rv >= 0) {
        rv = tail_file(&fd);
    }
}
