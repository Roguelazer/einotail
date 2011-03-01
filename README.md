einotail
=======
* Have you ever thought that `tail -F` is too heavyweight and really shouldn't be doing all that polling?
* Have you ever wanted to plug `tail -F` into xinetd, only to find that it won't terminate until it successfully reads data?
* Have you ever wanted to use [epoll(7)](http://www.kernel.org/doc/man-pages/online/pages/man4/epoll.4.html), [inotify(7)](http://www.kernel.org/doc/man-pages/online/pages/man7/inotify.7.html), [C (99)](http://en.wikipedia.org/wiki/C99) in the same program?

If you answered "yes" to all three of these questions, `einotail` is for you!

If you think **EINOTAIL** is an errno value raised when `tail` doesn't work, `einotail` may also be for you.

Usage
-----
`einotail /path/to/file`
Behaves roughly the same as `tail -F`. More specifically, this will cause `einotail` to behave like `tail -f` if `/path/to/file` is a regular file, like
`tail --follow=name` if `/path/to/file` is a symlink, and like `tail --follow=name --retry` if `/path/to/file` is a dangling symlink.

License
-------
`einotail` is available under the ISC (OpenBSD) license. The full contents of this
license are available as LICENSE
