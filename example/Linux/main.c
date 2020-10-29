#include <stdio.h>
#include <unistd.h>
#include <termio.h>
#include <errno.h>

#include "zm_cli.h"

#define CLI_NAME "test cli>"

#define ECHOFLAGS (ECHO | ECHOE | ECHOK | ECHONL)

static int set_disp_mode(int option)
{
    int err;
    struct termios term;
    if (tcgetattr(STDIN_FILENO, &term) == -1)
    {
        perror("Cannot get the attribution of the terminal");
        return 1;
    }
    if (option)
        term.c_lflag |= ECHOFLAGS;
    else
        term.c_lflag &= ~ECHOFLAGS;
    err = tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
    if (err == -1 && err == EINTR)
    {
        perror("Cannot set the attribution of the terminal");
        return 1;
    }
    return 0;
}

static int scanKeyboard(void)
{
    int in;
    struct termios new_settings;
    struct termios stored_settings;
    tcgetattr(0, &stored_settings);
    new_settings = stored_settings;
    new_settings.c_lflag &= (~ICANON);
    new_settings.c_cc[VTIME] = 0;
    tcgetattr(0, &stored_settings);
    new_settings.c_cc[VMIN] = 1;
    tcsetattr(0, TCSANOW, &new_settings);

    in = getchar();

    tcsetattr(0, TCSANOW, &stored_settings);
    return in;
}

static int test_read(void *a_buf, uint16_t length)
{
    int cnt = 0;
    char *p_buf = (char *)a_buf;

    while(cnt < length)
    {
        p_buf[cnt++] = scanKeyboard();
    }
    return cnt;
}

static int test_write(const void *a_data, uint16_t length)
{
    int cnt = 0;
    char *p_buf = (char *)a_data;

    while(cnt < length)
    {
        putchar((char)p_buf[cnt++]);
    }
    return cnt;
}

static cli_trans_api_t cli_trans = {
    .write = test_write,
    .read = test_read
};

void *cli_malloc(size_t size)
{
    return malloc(size);
}
void cli_free(void *p)
{
    free(p);
}

ZM_CLI_DEF(zm_cli, CLI_NAME, cli_trans);

int main(void)
{
    set_disp_mode(0);
    zm_cli_init(&zm_cli);
    printf("zm cli start!");
    zm_cli_start(&zm_cli);
    while(1)
    {
        zm_cli_process(&zm_cli);
    }
    return 0;
}


static void test_exit(zm_cli_t const * p_cli, size_t argc, char **argv)
{
    set_disp_mode(1);
    printf("exit!\r\n");
    exit(0);
}


static void cli_print_hello(zm_cli_t const * p_cli, size_t argc, char **argv)
{
    printf("hello\n");
}

static void cli_print_world(zm_cli_t const * p_cli, size_t argc, char **argv)
{
    printf("world\n");
}
static void cli_print_help(zm_cli_t const * p_cli, size_t argc, char **argv)
{
    print_usage(p_cli, argv[0], "<-h : get help> [--help]");
}

CLI_CREATE_STATIC_SUBCMD_SET(sub_2)
{
    CLI_CMD_LOAD_PARA(print_world, NULL, "printf word", cli_print_world),
    CLI_CMD_LOAD_PARA(help, NULL, "test help", cli_print_help),

    CLI_SUBCMD_SET_END
};

CLI_CREATE_STATIC_SUBCMD_SET(sub_1)
{
    CLI_CMD_LOAD_PARA(print_hello, NULL, "print hello", cli_print_hello),
    CLI_CMD_LOAD_PARA(print_2, &sub_2, "test print", NULL),

    CLI_SUBCMD_SET_END
};

CLI_CMD_REGISTER(print, &sub_1, "print", NULL);

CLI_CMD_REGISTER(exit, NULL, "exit", &test_exit);
