#define _XOPEN_SOURCE 600

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFSIZE 1024
#define MAX_EVENTS 16
#define TRAILING_LINES 40

long path_len;
char* readlink_buf;

struct fdata {
    int fd;
    const char *filename;
    const char *full_filename;
    off_t offset;
    off_t size;
};

long max(long a, long b)
{
    return (a > b) ? a : b;
}

char* readlink_full(const char* path)
{
    char buf[path_len + 1];
    ssize_t read_b;
    read_b = readlink(path, buf, path_len);
    buf[read_b] = '\0';
    strncpy(readlink_buf, buf, read_b+1);
    return readlink_buf;
}

int open_file(struct fdata* fd)
{
    if (fd->fd != -1)
        close(fd->fd);
    printf("opening '%s'\n", fd->filename);
    if ((fd->fd = open(fd->filename, O_RDONLY)) < 0) {
        perror("open");
        return -1;
    }
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

#define FIND_LINE_BUFSIZE 1024

/* Find the offset that is either TRAILING_LINES behind the
 * end of the file or is at the beginning of the file
 */
off_t find_lines_offset(struct fdata* fd, struct stat* stbuf)
{
    ssize_t read_size = -1;
    char buf[FIND_LINE_BUFSIZE];
    int num_newlines_found = 0;
    off_t offset = stbuf->st_size;
    off_t init_offset;
    while(num_newlines_found <= TRAILING_LINES && offset > 0) {
        char* found;
        offset = max(offset - FIND_LINE_BUFSIZE, 0);
        init_offset = offset;
        printf("Reading at %d\n", offset);
        lseek(fd->fd, offset, SEEK_SET);
        if ((read_size = read(fd->fd, buf, FIND_LINE_BUFSIZE)) < 0) {
            perror("find_lines_offset read");
            return -1;
        }
        do { 
            found = strrchr(buf, '\n');
            if (found != NULL) {
                printf("Found at index %d\n", (found - buf));
                *found = '\0';
                offset = (found - buf) + init_offset + 1;
                num_newlines_found++;
                printf("Found %d newlines\n", num_newlines_found);
            }
        } while(num_newlines_found <= TRAILING_LINES && found != NULL);
    }
    printf("Returning offset %d\n", offset);
    return offset;
}

int tail_file(struct fdata* fd)
{
    struct stat stbuf;
    ssize_t read_s;
    char read_buf[BUFSIZE];

    fstat(fd->fd, &stbuf);
    if (fd->offset == -1) {
        fd->offset = find_lines_offset(fd, &stbuf);
    }
    if (fd->size == -1) {
        fd->size = stbuf.st_size;
    }

    lseek(fd->fd, fd->offset, SEEK_SET);
    while (fd->offset < fd->size) {
        read_s = read(fd->fd, &read_buf, BUFSIZE);
        if (read_s <  0) {
            perror("read");
            return -1;
        } else if (read_s == 0) {
            break;
        }
        write(STDOUT_FILENO, read_buf, read_s);
    }
    fd->offset += read_s;
    close(fd->fd);
    fd->fd = -1;
    return read_s;
}

int main(int argc, char** argv)
{
    int ep, in, in_watch, link_watch;
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];
    struct fdata fd;
    struct stat stbuf;

    int rv = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: ytail filename\n"
                "Behaves like tail -F filename\n");
        return 2;
    }

    fd.filename = argv[1];
    fd.fd = -1;
    fd.offset = -1;
    fd.size = -1;

    if (access(fd.filename, R_OK|F_OK) != 0) {
        fprintf(stderr, "Unable to access %s\n", fd.filename);
        return EXIT_FAILURE;
    }

    path_len = pathconf(fd.filename, _PC_PATH_MAX);
    readlink_buf = malloc(path_len * sizeof(char));

    if ((ep = epoll_create(2)) == -1) {
        perror("epoll_crate");
        return EXIT_FAILURE;
    }
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    if (epoll_ctl(ep, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1) {
        perror("epoll_ctl: stdin");
        return EXIT_FAILURE;
    }

    in = inotify_init1(IN_NONBLOCK|IN_CLOEXEC);
    ev.events = EPOLLIN;
    ev.data.fd = in;
    if (epoll_ctl(ep, EPOLL_CTL_ADD, in, &ev) == -1) {
        perror("epoll_ctl: inotify");
        return EXIT_FAILURE;
    }

    open_file(&fd);
    tail_file(&fd);

    stat(fd.filename, &stbuf);
    if (S_ISLNK(stbuf.st_mode)) {
        link_watch = inotify_add_watch(in, fd.filename, IN_ALL_EVENTS|IN_DONT_FOLLOW);
        in_watch = inotify_add_watch(in, readlink_full(fd.filename), IN_ALL_EVENTS);
    } else {
        link_watch = -1;
        in_watch = inotify_add_watch(in, fd.filename, IN_ALL_EVENTS);
    }
    while (rv >= 0) {
        int num_events;
        int i;
        if ((num_events = epoll_wait(ep, events, MAX_EVENTS, 1000)) < 0) {
            perror("epoll_wait");
            return EXIT_FAILURE;
        }
        for (i=0; i < num_events; ++i) {
            if (events[i].data.fd == in) {
                struct inotify_event d;
                int err;
                while(1) {
                    if ((err = read(in, &d, sizeof(struct inotify_event))) < 1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                        else {
                            perror("read from inotify fd");
                            return EXIT_FAILURE;
                        }
                    } else if (err != sizeof(struct inotify_event)) {
                        printf("Got %d bytes, expected %d\n", err, sizeof(struct inotify_event));
                    }
                    if (d.wd == link_watch) {
                        printf("Adding watch for %s\n", readlink_full(fd.filename));
                        inotify_rm_watch(in, in_watch);
                        in_watch = inotify_add_watch(in, readlink_full(fd.filename), IN_ALL_EVENTS);
                        open_file(&fd);
                    } else {
                        printf("Activity on actual file\n");
                        tail_file(&fd);
                    }

                    if (d.mask & IN_DELETE_SELF) {
                        printf("DELETE\n");
                    } else if (d.mask & IN_MOVE_SELF) {
                        printf("MOVE\n");
                    } else if (d.mask & IN_MODIFY) {
                        printf("MODIFY\n");
                    } else if (d.mask & IN_CREATE) {
                        printf("CREATE\n");
                    }
                }
            } else {
                printf("activity on stdin\n");
                char buf[1024];
                ssize_t dsize = read(STDIN_FILENO, &buf, 1024);
                write(STDOUT_FILENO, &buf, dsize);
            }
        }
        printf("%d\n", num_events);
    }

    close(in);
    close(ep);
    free(readlink_buf);
    return EXIT_SUCCESS;
}
