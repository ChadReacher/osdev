#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>
#include <dirent.h>

#define ASSERT_OK(expr) do { \
    i32 _err = (expr); \
    if (_err < 0) { \
        printf("(%s:%s:%d) ", __func__, __FILE__, __LINE__); \
        perror(#expr " failed"); \
        _exit(1); \
    } \
} while (0)

#define ASSERT_PTR(expr) ({ \
    void *_ptr = (expr); \
    if (_ptr == NULL) { \
        printf("(%s:%s:%d) ", __func__, __FILE__, __LINE__); \
        perror(#expr " failed"); \
        _exit(1); \
    } \
    _ptr; \
})

void test_dir(const char *dirname, const char **files, int sz) {
    DIR *dirp;
    struct dirent *entry;
    int i = 0;

    dirp = ASSERT_PTR(opendir(dirname));
    while ((entry = readdir(dirp)) != 0) {
        if (i >= sz) {
            printf("TEST FAILED: too much files in the directory\n");
            _exit(1);
        }
        if (strcmp(files[i], entry->name) != 0) {
            printf("TEST FAILED: expected=%s actual=%s\n", files[i], entry->name);
            _exit(1);
        }
        ++i;
    }
    ASSERT_OK(closedir(dirp));
}

int test_func(void) {
    printf("Testing VFS\n");

    {
        const char *at_root[14] = { ".", "..", "lost+found", "a", "bin", "dev", "etc", "home", "lib", "mnt", "tmp", "usr", "var",
            "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
        };
        // getcwd(2) == '/'
        test_dir("/", at_root, 14);
        test_dir("./", at_root, 14);
        test_dir("../", at_root, 14);
        test_dir(".", at_root, 14);
        test_dir("..", at_root, 14);
        test_dir("./a/..", at_root, 14);
        test_dir("./a/../", at_root, 14);
        test_dir("../a/../", at_root, 14);
        test_dir("../a/..", at_root, 14);
        test_dir("/a/../", at_root, 14);
        test_dir("/a/..", at_root, 14);
        printf("TEST PASSED: access root directory in rootfs\n");
    }

    {
        char buf[512] = {0};
        const char *at_rootfs[14] = { ".", "..", "lost+found", "a", "bin", "dev", "etc", "home", "lib", "mnt", "tmp", "usr", "var",
            "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
        };
        const char *at_root[6] = { ".", "..", "lost+found", "much", "test", "to_del" };
        const char *at_much[3] = { ".", "..", "more" };
        const char *at_more[3] = { ".", "..", "inner" };
        const char *at_inner[2] = { ".", ".." };
        ASSERT_OK(mount("/dev/hdb1", "/mnt"));

        // getcwd(2) == '/'
        test_dir("/mnt", at_root, 6);
        test_dir("./mnt", at_root, 6);
        test_dir("../mnt", at_root, 6);
        test_dir("/mnt/much", at_much, 3);
        test_dir("/mnt/much/more", at_more, 3);
        test_dir("/mnt/much/more/inner", at_inner, 2);

        // getcwd(2) == '/mnt'
        const char *expected = "/mnt";
        ASSERT_OK(chdir(expected));
        ASSERT_PTR(getcwd(buf, sizeof(buf)));
        if (strcmp(buf, expected) != 0) {
            printf("TEST FAILED: getcwd: expected=%s actual=%s\n", expected, buf);
            _exit(1);
        }
        test_dir("../", at_rootfs, 14);
        test_dir("../../", at_rootfs, 14);
        test_dir("./", at_root, 14);
        test_dir("../mnt", at_root, 6);
        test_dir("/mnt", at_root, 6);
        test_dir("./much", at_much, 3);
        test_dir("/mnt/much", at_much, 3);
        test_dir("./much/more", at_more, 3);
        test_dir("/mnt/much/more", at_more, 3);
        test_dir("./much/more/inner", at_inner, 2);
        test_dir("/mnt/much/more/inner", at_inner, 2);

        // getcwd(2) == '/mnt/much'
        expected = "/mnt/much";
        ASSERT_OK(chdir("much"));
        ASSERT_PTR(getcwd(buf, sizeof(buf)));
        if (strcmp(buf, expected) != 0) {
            printf("TEST FAILED: expected=%s actual=%s\n", expected, buf);
            _exit(1);
        }
        test_dir("./", at_much, 3);
        test_dir("../much", at_much, 3);
        test_dir("../../mnt/much", at_much, 3);
        test_dir("/mnt/much", at_much, 3);

        test_dir("./more", at_more, 3);
        test_dir("../much/more", at_more, 3);
        test_dir("../../mnt/much/more", at_more, 3);
        test_dir("../../../mnt/much/more", at_more, 3);
        test_dir("/mnt/much/more", at_more, 3);

        test_dir("more/inner", at_inner, 2);
        test_dir("./more/inner", at_inner, 2);
        test_dir("../much/more/inner", at_inner, 2);
        test_dir("../../mnt/much/more/inner", at_inner, 2);
        test_dir("/mnt/much/more/inner", at_inner, 2);

        ASSERT_OK(chdir("/"));
        ASSERT_OK(umount("/mnt"));

        printf("TEST PASSED: various paths across mounted directory\n");
    }

    {
        char buf[512] = {0};
        const char *expected = "/";
        ASSERT_PTR(getcwd(buf, sizeof(buf)));
        if (strcmp(buf, expected) != 0) {
            printf("TEST FAILED: getcwd1: expected=%s actual=%s\n", expected, buf);
            _exit(1);
        }

        ASSERT_OK(mount("/dev/hdb1", "/mnt"));

        expected = "/mnt";
        ASSERT_OK(chdir(expected));
        ASSERT_PTR(getcwd(buf, sizeof(buf)));
        if (strcmp(buf, expected) != 0) {
            printf("TEST FAILED: getcwd2: expected=%s actual=%s\n", expected, buf);
            _exit(1);
        }

        expected = "/";
        ASSERT_OK(chdir(".."));
        ASSERT_PTR(getcwd(buf, sizeof(buf)));
        if (strcmp(buf, expected) != 0) {
            printf("TEST FAILED: getcwd3: expected=%s actual=%s\n", expected, buf);
            _exit(1);
        }

        expected = "/";
        ASSERT_OK(chdir(".."));
        ASSERT_PTR(getcwd(buf, sizeof(buf)));
        if (strcmp(buf, expected) != 0) {
            printf("TEST FAILED: getcwd4: expected=%s actual=%s\n", expected, buf);
            _exit(1);
        }

        ASSERT_OK(chdir("/"));
        ASSERT_OK(umount("/mnt"));

        printf("TEST PASSED: getcwd(/); cd /mnt; getcwd(/mnt); cd ..; getcwd(/)\n");
    }

    return 0;
}

int main(void) {
    __asm__ volatile ("int3" ::: "memory");
    for (int i = 0; i < 100; ++i) {
        test_func();
    }
    __asm__ volatile ("int3" ::: "memory");

    return 0;
}