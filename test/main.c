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
#include <hibus.h>

#include "../include/inetd.h"

static void wifi_signal_handler(hibus_conn* conn, const char* from_endpoint, const char* bubble_name, const char* bubble_data)
{
    printf("================================================================================================================================== get signal\n");
}

static int wifi_scan_handler(hibus_conn* conn, const char* from_endpoint, const char* method_name, int ret_code, const char* ret_value)
{
    printf("============================================================================================================================== get scan result\n");
    return 0;
}

int main(void)
{
    int fd_socket = -1;
    hibus_conn * hibus_context = NULL;
    char * endpoint = NULL;
    int ret_code = 0;
//    char * ret_value = NULL;

    // connect to hibus server
    fd_socket = hibus_connect_via_unix_socket(SOCKET_PATH, AGENT_NAME, AGENT_RUNNER_NAME, &hibus_context);
    if(fd_socket <= 0)
    {
        printf("WIFI DAEMON: connect to HIBUS server error!\n");
        exit(1);
    }


    endpoint = hibus_assemble_endpoint_name_alloc(HIBUS_LOCALHOST, APP_INETD_NAME, RUNNER_WIFI_NAME);
    //hibus_call_procedure_and_wait(hibus_context, endpoint, METHOD_WIFI_SCAN, "{\"abcd\":1234}", 1000, &ret_code, &ret_value);
    ret_code = hibus_call_procedure(hibus_context, endpoint, METHOD_WIFI_SCAN, "{\"abcd\":1234}", 1000, wifi_scan_handler);
    ret_code ++;
    hibus_subscribe_event(hibus_context, endpoint, EVENT_WIFI_SIGNAL, wifi_signal_handler);

    while(1) 
    { 
        hibus_wait_and_dispatch_packet(hibus_context, 1000);
        sleep(1);
    }


    free(endpoint);
    hibus_disconnect(hibus_context);

	return 0;
}
