#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/timerfd.h>
#include <hibus.h>

#include "../include/inetd.h"
//#include "../include/hibus_wifi.h"

static hibus_conn * hibus = NULL;
static HibusInvokeOps hibusOps;

int open_device(int * fd_device)
{
    *fd_device = -1;
    return 0;
}

int close_device(int fd_device)
{
    if(fd_device == -1)
        close(fd_device);

    return 0;
}

void device_read(void)
{
}

void wifi_scan(void)
{
    int ret_code = 0;
    ret_code = hibus_fire_event(hibus, EVENT_WIFI_SCAN, "");
    if(ret_code)
        fprintf(stderr, "======================================= WIFI DAEMON: Error for hibus_fire_event %s, %s.\n", EVENT_WIFI_SCAN, hibus_get_err_message(ret_code));
}

void wifi_signal(void)
{
    int ret_code = 0;
    ret_code = hibus_fire_event(hibus, EVENT_WIFI_SIGNAL, "");
    if(ret_code)
        fprintf(stderr, "======================================= WIFI DAEMON: Error for hibus_fire_event %s, %s.\n", EVENT_WIFI_SIGNAL, hibus_get_err_message(ret_code));
}

char * wifi_scan_handler(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
{
    char * ret_code = malloc(100);
    memset(ret_code, 0, 100);
    sprintf(ret_code, "{\"hello world\":9000}");
    return ret_code;
}

char * wifi_connect_handler(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
{
    char * ret_code = malloc(100);
    memset(ret_code, 0, 100);
    sprintf(ret_code, "{\"hello world\":9000}");
    return ret_code;
}

char * wifi_disconnect_handler(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
{
    char * ret_code = malloc(100);
    memset(ret_code, 0, 100);
    sprintf(ret_code, "{\"hello world\":9000}");
    return ret_code;
}

// initialize device.
HibusInvokeOps * wifi_ops_get(hibus_conn* conn)
{
    hibus = conn;

    // initialize hibusOps
    hibusOps.open_device = open_device;
    hibusOps.close_device = close_device;
    hibusOps.device_read = device_read;
    hibusOps.wifi_scan = wifi_scan;
    hibusOps.wifi_signal = wifi_signal;
    hibusOps.wifi_scan_handler = wifi_scan_handler;
    hibusOps.wifi_connect_handler = wifi_connect_handler;
    hibusOps.wifi_disconnect_handler = wifi_disconnect_handler;

    return &hibusOps;
}
