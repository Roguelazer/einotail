einotail
=======
* Have you ever thought that `tail -F` is too heavyweight?
* Have you ever wanted to plug `tail -F` into xinetd, only to find that it won't terminate until it successfully reads data?
* Have you ever wanted to use `epoll(7)`, `inotify(7)`, and `C (99)` in the same program?

If you answered "yes" to all three of these questions, `einotail` is for you!

Usage
-----
`einotail /path/to/file`
Behaves roughly the same as `tail -F`. More specifically, this will cause `einotail` to behave like `tail -f` if `/path/to/file` is a regular file, like
`tail --follow=name` if `/path/to/file` is a symlink, and like `tail --follow=name --retry` if `/path/to/file` is a dangling symlink.

License
-------
`einotify` is available under the ISC (OpenBSD) license. The full contents of this
license are available as LICENSE
