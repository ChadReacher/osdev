#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>

#define ASSERT_OK(expr) do { \
    i32 _err = (expr); \
    if (_err < 0) { \
        perror(#expr " failed"); \
        _exit(1); \
    } \
} while (0)

#define ASSERT_PTR(expr) ({ \
    void *_ptr = (expr); \
    if (_ptr == NULL) { \
        perror(#expr " failed"); \
        _exit(1); \
    } \
    _ptr; \
})

int main(void) {
    printf("Testing getcwd\n");

    {
        char buf[512];
        const char *expected = "/";
        ASSERT_OK(chdir(expected) == 0);
        i8 *path = ASSERT_PTR(getcwd(buf, sizeof(buf)));
        if (strcmp(buf, expected) != 0) {
            printf("test failed: expected %s, actual - %s\n", expected, buf);
            _exit(1);
        } else if (strcmp(path, expected) != 0) {
            printf("test failed: expected %s, actual - %s\n", expected, path);
            _exit(1);
        }
        printf("[PASSED]: only root when '%s'\n", expected);
    }

    {
        char buf[512];
        const char *expected = "/home";
        ASSERT_OK(chdir(expected) == 0);
        i8 *path = ASSERT_PTR(getcwd(buf, sizeof(buf)));
        if (strcmp(buf, expected) != 0) {
            printf("test failed: expected %s, actual - %s\n", expected, buf);
            _exit(1);
        } else if (strcmp(path, expected) != 0) {
            printf("test failed: expected %s, actual - %s\n", expected, path);
            _exit(1);
        }
        printf("[PASSED]: just works when '%s'\n", expected);
    }

    {
        char buf[512];
        const char *expected = "/a/b/c/d/e";
        ASSERT_OK(chdir(expected) == 0);
        i8 *path = ASSERT_PTR(getcwd(buf, sizeof(buf)));
        if (strcmp(buf, expected) != 0) {
            printf("test failed: expected %s, actual - %s\n", expected, buf);
            _exit(1);
        } else if (strcmp(path, expected) != 0) {
            printf("test failed: expected %s, actual - %s\n", expected, path);
            _exit(1);
        }
        printf("[PASSED]: just works when '%s'\n", expected);
    }

    {
        char buf[512] = {0};
        const char *expected = "/mnt/much/more/inner";

        ASSERT_OK(mount("/dev/hdb1", "/mnt"));
        ASSERT_OK(chdir(expected));
        i8 *path = ASSERT_PTR(getcwd(buf, sizeof(buf)));
        if (strcmp(buf, expected) != 0) {
            printf("test failed: expected %s, actual - %s\n", expected, buf);
            _exit(1);
        } else if (strcmp(path, expected) != 0) {
            printf("test failed: expected %s, actual - %s\n", expected, path);
            _exit(1);
        }
        printf("[PASSED]: inside mount point when '%s'\n", expected);
        ASSERT_OK(umount("/mnt"));
    }

    {
        i8 *path = getcwd(NULL, 123);
        if (path == NULL) {
            if (errno != EFAULT) {
                printf("error: expected EFAULT, received (%d) %s\n", errno, strerror(errno));
                _exit(1);
            }
        } else {
            printf("error: expected EFAULT, received successfull call %s\n", path);
            _exit(1);
        }
        printf("[PASSED]: EFAULT when buf points to a bad address\n");
    }

    {
        char buf[512];
        i8 *path = getcwd(buf, 0);
        if (path == NULL) {
            if (errno != EINVAL) {
                printf("error: expected EINVAL, received (%d) %s\n", errno, strerror(errno));
                _exit(1);
            }
        } else {
            printf("error: expected EINVAL, received successfull call %s\n", path);
            _exit(1);
        }
        printf("[PASSED]: EINVAL when size argument is zero and buf is not a null pointer\n");
    }

    {
        char buf[512];
        const char *expected = "/a/b/c/d/e";
        ASSERT_OK(chdir(expected) == 0);
        i8 *path = getcwd(buf, 1);
        if (path == NULL) {
            if (errno != ERANGE) {
                printf("error: expected ERANGE, received (%d) %s\n", errno, strerror(errno));
                _exit(1);
            }
        } else {
            printf("error: expected ERANGE, received successfull call %s\n", path);
            _exit(1);
        }
        printf("[PASSED]: ERANGE when the size argument is less than the length of the absolute pathname of the working directory\n");
    }

    {
        char buf[1024] = {0};
        ASSERT_OK(chdir("/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"));
        ASSERT_OK(chdir("yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy"));
        ASSERT_OK(chdir("zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"));
        ASSERT_OK(chdir("iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"));
        {
            // Pretty long string but inside PATH_MAX bounds
            ASSERT_OK(chdir("jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj"));
            const char *expected =
            "/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
            "/yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy"
            "/zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
            "/iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
            "/jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj";
            i8 *path = ASSERT_PTR(getcwd(buf, sizeof(buf)));
            if (strcmp(buf, expected) != 0) {
                printf("test failed for @buf: expected %s, actual - %s\n", expected, buf);
                _exit(1);
            } else if (strcmp(path, expected) != 0) {
                printf("test failed for @path: expected %s, actual - %s\n", expected, path);
                _exit(1);
            }
            printf("[PASSED]: 1005 length absolute path + null terminator\n");
        }

        {
            // 1022-length string + null terminator
            // 1005 + 1 + 16      + 1
            ASSERT_OK(chdir("achieve_1022_cha"));
            const char *expected =
            "/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
            "/yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy"
            "/zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
            "/iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
            "/jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj"
            "/achieve_1022_cha";
            i8 *path = ASSERT_PTR(getcwd(buf, sizeof(buf)));
            if (strlen(buf) != 1022) {
                printf("test failed for sz: expected 1022, actual %d\n", strlen(buf));
                _exit(1);
            }
            if (strcmp(buf, expected) != 0) {
                printf("test failed for @buf: expected %s, actual - %s\n", expected, buf);
                _exit(1);
            } else if (strcmp(path, expected) != 0) {
                printf("test failed for @path: expected %s, actual - %s\n", expected, path);
                _exit(1);
            }
            printf("[PASSED]: 1022 length absolute path + null terminator\n");
            ASSERT_OK(chdir(".."));
        }

        {
            // 1023-length string + null terminator
            // 1005 + 1 + 17      + 1
            ASSERT_OK(chdir("achieve_1023_char"));
            const char *expected =
            "/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
            "/yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy"
            "/zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
            "/iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
            "/jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj"
            "/achieve_1023_char";
            i8 *path = ASSERT_PTR(getcwd(buf, sizeof(buf)));
            if (strlen(buf) != 1023) {
                printf("test failed for sz: expected 1023, actual %d\n", strlen(buf));
                _exit(1);
            }
            if (strcmp(buf, expected) != 0) {
                printf("test failed for @buf: expected %s, actual - %s\n", expected, buf);
                _exit(1);
            } else if (strcmp(path, expected) != 0) {
                printf("test failed for @path: expected %s, actual - %s\n", expected, path);
                _exit(1);
            }
            printf("[PASSED]: 1023 length absolute path + null terminator\n");
            ASSERT_OK(chdir(".."));
        }

        {
            // 1024-length string + null terminator
            // 1005 + 1 + 18      + 1
            ASSERT_OK(chdir("achieve_1024_charl"));
            i8 *path = getcwd(buf, sizeof(buf));
            if (path == NULL) {
                if (errno != ENAMETOOLONG) {
                    printf("error: expected ENAMETOOLONG, received (%d) %s\n", errno, strerror(errno));
                    _exit(1);
                }
            } else {
                printf("error: expected ENAMETOOLONG, received successfull call %s\n", path);
                _exit(1);
            }
            printf("[PASSED]: 1024 length absolute path + null terminator\n");
            ASSERT_OK(chdir(".."));
        }

        {
            // 1025-length string + null terminator
            // 1005 + 1 + 19      + 1
            ASSERT_OK(chdir("achieve_1025_charli"));
            i8 *path = getcwd(buf, sizeof(buf));
            if (path == NULL) {
                if (errno != ENAMETOOLONG) {
                    printf("error: expected ENAMETOOLONG, received (%d) %s\n", errno, strerror(errno));
                    _exit(1);
                }
            } else {
                printf("error: expected ENAMETOOLONG, received successfull call %s\n", path);
                _exit(1);
            }
            printf("[PASSED]: 1025 length absolute path + null terminator\n");
            ASSERT_OK(chdir(".."));
        }
    }
    return 0;
}