/**
 * @file _syscalls.c
 * @author 孙鸣淼
 * @brief 覆写所有可能触发半主机模式的底层函数
 * @version 0.1
 * @date 2025-12-21
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <stdio.h>
#include <time.h>
#include <rt_sys.h>

#ifdef __cplusplus
extern "C" {
#endif

// 定义标准文件句柄
struct __FILE {
    int handle;
};

FILE __stdout;
FILE __stdin;
FILE __stderr;

// 必须实现的函数
int fputc(int ch, FILE *f) {
    return ch;
}

int fgetc(FILE *f) {
    return -1;
}

// 系统调用函数实现
void _sys_exit(int returncode) {
    // 禁用半主机退出
    while(1);
}

void _ttywrch(int ch) {

}

// 其他必须的函数（返回错误或空实现）
int _sys_open(const char *name, int openmode) {
    return -1;
}

int _sys_close(int file) {
    return -1;
}

int _sys_write(FILEHANDLE /*fh*/ a, const unsigned char * /*buf*/ b,
                      unsigned /*len*/ c, int /*mode*/ d){return 0;}

int _sys_read(FILEHANDLE /*fh*/ a,  unsigned char * /*buf*/ b,
                      unsigned /*len*/ c, int /*mode*/ d){return 0;}

int _sys_istty(int file) {
    return (file == 0 || file == 1 || file == 2) ? 1 : 0;
}

int _sys_seek(int file, long pos) {
    return -1;
}

long _sys_flen(int file) {
    return 0;
}

void _sys_tmpnam(char * /*name*/ a, int /*sig*/ b, unsigned /*maxlen*/ c) {
    return ;
}

char *_sys_command_string(char *cmd, int len) {
    return NULL;
}

clock_t clock(void) {
    return (clock_t)-1;
}

time_t time(time_t *timer) {
    if (timer) *timer = 0;
    return 0;
}

void _clock_init(void) {
    // 空实现
}

#ifdef __cplusplus
}
#endif