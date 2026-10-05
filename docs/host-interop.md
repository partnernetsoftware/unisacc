# Calling the host's libraries from a unisacc program

unisacc compiles your C into its own runtime, but a program can still call functions of the system
it runs on (macOS and Linux; Windows is not covered yet).

## Forwarding: declare, do not define

A function that your program declares with a prototype but never defines is forwarded to the system
C library by name. This works under `-run`, in images written with `-o`, and, since 0.0.26, in
programs made of several source files.

```c
long sysconf(int);                 /* no body anywhere: forwarded */
int main(void) { return sysconf(29) > 0 ? 0 : 1; }
```

The bundled headers declare `getaddrinfo`, `dlopen`, `getpwuid`, `mmap` and others this way on Linux and
macOS, so including the header is enough.

## Other libraries: load them with RTLD_GLOBAL first

The forwarder looks names up in the libraries already loaded into the process. To reach another
library, load it with `RTLD_GLOBAL`; after that its functions are forwarded like libc's.

```c
#include <dlfcn.h>
void *curl_easy_init(void);
int main(void) {
    dlopen("/usr/lib/libcurl.4.dylib", RTLD_NOW | RTLD_GLOBAL);   /* Linux: "libcurl.so.4" */
    return curl_easy_init() ? 0 : 1;
}
```

`examples/https/post.c` makes a complete HTTPS POST this way. Certificates are checked by libcurl
against the system's trust store.

## What does not cross the boundary (yet)

- **Variadic functions** (such as `curl_easy_setopt`) are not forwarded. On macOS arm64, variadic
  arguments travel on the stack, so call them through `uffi_call` with the fixed-argument count
  (see the example). On Linux, a plain prototype with the actual argument types works.
- **Host stdio buffers.** A unisacc `FILE *` is a plain file descriptor, and its `fflush` does not
  reach the host C library. Output that a host library writes with its own `printf` or `fwrite` stays
  in the host's buffer until you flush it there. Call the host's `fflush(NULL)`:
  ```c
  static void host_fflush_all(void) {
      static void *fn; long a[10] = {0};
      if (!fn) fn = uffi_dlsym((void *)UFFI_RTLD_DEFAULT, "fflush");
      if (fn) __hostcall(fn, a);
  }
  ```
- **Handing your own objects to the host.** Do not pass a unisacc `FILE *` as a host stream (for
  example `CURLOPT_WRITEDATA`), and do not pass a unisacc function as a host callback (for example
  `CURLOPT_WRITEFUNCTION`). unisacc code uses its own calling convention, so the program crashes with
  status 139. This is an ABI limitation, not a bug in the library you call.
- **Data symbols** such as `environ` are not forwarded; only functions are.
- **Returning a struct by value** from a forwarded function is not supported.
