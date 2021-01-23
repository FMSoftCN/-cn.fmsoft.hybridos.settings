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

#include "../include/wifi_intf.h"
#include "../include/inetd.h"

static void wifi_signal_handler(hibus_conn* conn, const char* from_endpoint, const char* bubble_name, const char* bubble_data)
{
    printf("================================================================================================================================== get signal, %s\n", bubble_data);
}

static int wifi_scan_handler(hibus_conn* conn, const char* from_endpoint, const char* method_name, int ret_code, const char* ret_value)
{
    printf("============================================================================================================================== %s\n", ret_value);
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


    endpoint = hibus_assemble_endpoint_name_alloc(HIBUS_LOCALHOST, APP_NAME_SETTINGS, RUNNER_NAME_INETD);
    ret_code = hibus_call_procedure(hibus_context, endpoint, METHOD_NET_OPEN_DEVICE, "{\"device\":\"wlp5s0\"}", 1000, wifi_scan_handler);
//    ret_code = hibus_call_procedure(hibus_context, endpoint, METHOD_NET_CLOSE_DEVICE, "{\"device\":\"wlp5s0\"}", 1000, wifi_scan_handler);
//    ret_code = hibus_call_procedure(hibus_context, endpoint, METHOD_NET_GET_DEVICES_STATUS, "{\"device\":\"wlp5s0\"}", 1000, wifi_scan_handler);
    //hibus_call_procedure_and_wait(hibus_context, endpoint, METHOD_WIFI_START_SCAN, "{\"abcd\":1234}", 1000, &ret_code, &ret_value);
//    ret_code = hibus_call_procedure(hibus_context, endpoint, METHOD_WIFI_START_SCAN, "{\"device\":\"wlp5s0\"}", 1000, wifi_scan_handler);
    ret_code = hibus_call_procedure(hibus_context, endpoint, METHOD_WIFI_CONNECT_AP, "{\"device\":\"wlp5s0\", \"ssid\":\"fmsoft_dev_7\", \"password\":\"suzhoujie123456\"}", 1000, wifi_scan_handler);
//    ret_code = hibus_call_procedure(hibus_context, endpoint, METHOD_WIFI_GET_NETWORK_INFO, "{\"device\":\"wlp5s0\"}", 1000, wifi_scan_handler);
    ret_code ++;
    hibus_subscribe_event(hibus_context, endpoint, WIFISIGNALSTRENGTHCHANGED, wifi_signal_handler);

    while(1) 
    { 
        hibus_wait_and_dispatch_packet(hibus_context, 1000);
        sleep(1);
    }


    free(endpoint);
    hibus_disconnect(hibus_context);

	return 0;
}
