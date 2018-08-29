/* https://www.linux.org.ru/forum/general/10574293 */
#include <stdint.h>
#include <sys/io.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define EC_SC 0x66
#define EC_DATA 0x62
#define IBF 1
#define OBF 0
#define EC_SC_READ_CMD 0x80
#define EC_SC_WRITE_CMD 0x81
#define EC_SC_SCI_CMD 0x84

#define BATT_PORT 0xED
#define BATT_LIMIT 0x42
#define BATT_FULL 0x40

#define BATT_PORT_ORIG 0x0a
#define BATT_LIMIT_ORIG 0x21
#define BATT_FULL_ORIG 0x41

static void init()
{
    if (ioperm(EC_DATA, 1, 1) != 0)
    {
        perror("ioperm(EC_DATA, 1, 1)");
        exit(1);
    }

    if (ioperm(EC_SC, 1, 1) != 0)
    {
        perror("ioperm(EC_SC, 1, 1)");
        exit(1);
    }
}

static void print_usage(){
  printf("btconsv command\n");
  printf("command:\n");
  printf("    f         full charge\n");
  printf("    l         limited charge\n");
  printf("    g         get current status\n");
  printf("    d         dump ec reg\n");
  printf("\n");
}

static void wait_ec(const uint32_t port, const uint32_t flag, const char value)
{
    uint8_t data;
    int i;

    i = 0;
    data = inb(port);

    while ( (((data >> flag) & 0x1) != value) && (i++ < 100) )
    {
        usleep(1000);
        data = inb(port);
    }

    if (i >= 100)
    {
        fprintf(stderr, "wait_ec error on port 0x%x, data=0x%x, flag=0x%x, value=0x%x\n", port, data, flag, value);
        exit(1);
    }
}

static uint8_t read_ec(const uint32_t port)
{
    uint8_t value;

    wait_ec(EC_SC, IBF, 0);
    outb(EC_SC_READ_CMD, EC_SC);
    wait_ec(EC_SC, IBF, 0);
    outb(port, EC_DATA);
    //wait_ec(EC_SC, EC_SC_IBF_FREE);
    wait_ec(EC_SC, OBF, 1);
    value = inb(EC_DATA);

    return value;
}

static void write_ec(const uint32_t port, const uint8_t value)
{
    wait_ec(EC_SC, IBF, 0);
    outb(EC_SC_WRITE_CMD, EC_SC);
    wait_ec(EC_SC, IBF, 0);
    outb(port, EC_DATA);
    wait_ec(EC_SC, IBF, 0);
    outb(value, EC_DATA);
    wait_ec(EC_SC, IBF, 0);
}

static void dump_all_regs(void)
{
    uint8_t val;
    int i;

    printf("EC reg dump:");

    for (i = 0x00; i <= 0xff; i++)
    {
        if ((i % 16) == 0)
        {
            printf("\n 0x%02x: ", i);
        }

        val = read_ec(i);
        printf("%02x ", val);
    }

    printf("\n");
}

static void set_value(const uint8_t value)
{
    uint8_t rval;

    rval = read_ec(BATT_PORT);
    printf("old value %02x\n", rval);
    write_ec(BATT_PORT, value);
    rval = read_ec(BATT_PORT);
    printf("new value %02x\n", rval);
}

static void print_status()
{
    if (read_ec(BATT_PORT) == BATT_FULL)
    {
      printf("Full charge mode\n");
    }
    else
    {
      printf("Limited carge mode\n");
    }
}

int main(int argc, char *argv[])
{
    init();

    if (argc < 2)
    {
      print_usage();
    }
    else
    {
        if (argv[1][0] == 'f')
        {
            printf("set full charge\n");
            set_value(BATT_FULL);
        }
        else if (argv[1][0] == 'l')
        {
            printf("set limited charge\n");
            set_value(BATT_LIMIT);
        }
        else if (argv[1][0] == 'g')
        {
            print_status();
        }
        else if (argv[1][0] == 'd')
        {
            dump_all_regs();
        }
        else
        {
            //printf("unknown option\n");
            print_usage();
        }
    }

    return 0;
}
