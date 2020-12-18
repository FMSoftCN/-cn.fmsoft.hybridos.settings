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

#undef  DAEMON
//#define DAEMON

//#undef  TEST_INTERFACE
#define TEST_INTERFACE

static char * scan_wifi_handler(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
{
    char * ret_code = malloc(100);
    memset(ret_code, 0, 100);
    sprintf(ret_code, "{\"hello world\":9000}");
    return ret_code;
}


int main(void)
{
    int fd_socket = -1;
    hibus_conn * hibus_context = NULL;
    int ret_code = 0;

#ifdef	DAEMON
    int pid = 0;

    pid = fork();
    if(pid < 0)    
        exit(1);  		        // fork error, son process quits
    else if(pid > 0) 	        // parent process quits
        exit(0);

    setsid();  
    pid = fork();
    if(pid > 0)
        exit(0); 		        // quits again. close terminal
    else if(pid < 0)    
        exit(1);                // fork error, son process quits

    for(i = 0; i < NOFILE; i++) // close all file
        close(i);

    chdir(WORKING_DIRECTORY);   // change working directory
    umask(0);					// reset mask
#endif

    // step 1: start other device monitor. One device one thread.
    // start_bluetooth_monitor();


    // step 2: read configure file, get the default parameters 


    // step 3: connect to hibus server
    fd_socket = hibus_connect_via_unix_socket(SOCKET_PATH, APP_INETD_NAME, RUNNER_WIFI_NAME, &hibus_context);
    if(fd_socket <= 0)
    {
        printf("WIFI DAEMON: connect to HIBUS server error!\n");
        exit(1);
    }


    // test of interface
#ifdef TEST_INTERFACE
    const char * test_name = NULL;
    int test_int = -1;

    test_name = hibus_conn_srv_host_name(hibus_context);
    printf("INET test: server host name is %s\n", test_name);

    test_name = hibus_conn_own_host_name(hibus_context);
    printf("INET test: host name is %s\n", test_name);

    test_name = hibus_conn_app_name(hibus_context);
    printf("INET test: app name is %s\n", test_name);

    test_name = hibus_conn_runner_name(hibus_context);
    printf("INET test: runner name is %s\n", test_name);

    test_int = hibus_conn_socket_fd(hibus_context);
    printf("INET test: socket is %d\n", test_int);

    test_int = hibus_conn_socket_type(hibus_context);
    switch(test_int)
    {
        case CT_UNIX_SOCKET:
            printf("INET test: socket type is CT_UNIX_SOCKET\n");
            break;
        case CT_WEB_SOCKET:
            printf("INET test: socket type is CT_WEB_SOCKET\n");
            break;
    }
#endif

    // step 4: register remote invocation
    // for_host and for_app are NULL, means for all hosts and applications
    ret_code = hibus_register_procedure(hibus_context, METHOD_WIFI_SCAN, NULL, NULL, scan_wifi_handler);
    if(ret_code)
    {
        printf("Error for hibus_register_procedure, return code is %d\n", ret_code);
        exit(1);
    }

    // step 5: register an event
    // to_host and to_app are NULL, means for all hosts and applications
    hibus_register_event(hibus_context, EVENT_WIFI_SIGNAL, NULL, NULL);



    // step 6: check wifi status periodically
    int timeout = 1000;
    while(1)
    {
        hibus_fire_event(hibus_context, EVENT_WIFI_SIGNAL, "");
        hibus_wait_and_dispatch_packet(hibus_context, timeout);
        sleep(1);
    }

//int hibus_fire_event (hibus_conn* conn,
//        const char* bubble_name, const char* bubble_data);



    // step 7: free the resource
    hibus_revoke_event(hibus_context, EVENT_WIFI_SIGNAL);
    hibus_revoke_procedure(hibus_context, METHOD_WIFI_SCAN);
    hibus_disconnect(hibus_context);

	return 0;
}
