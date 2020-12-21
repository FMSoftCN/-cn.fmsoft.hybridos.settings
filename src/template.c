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
#include <stdint.h>        /* Definition of uint64_t */

#include <hibus.h>

#include "../include/inetd.h"
#include "../include/hibus_template.h"


static char * scan_template_handler(hibus_conn* conn, const char* from_endpoint, const char* to_method, \
                                const char* method_param, int *err_code)
{
    return "{\"hello world\":9000}";
}

void * start_function(void * args)
{
    // for hibus
    int fd_hibus = -1;                      // socket for communication with hibus
    hibus_conn * hibus_context = NULL;      // context of communication
    int ret_code = 0;

    // for timer
    int fd_timer = -1;                      // fd for timer
    struct itimerspec new_value;            // start and interval time for timer
    struct timespec now;                    // time now
    uint64_t exp = 0;                       // for timer read

    // for select
    fd_set rfds;
    int maxfd = 0;
//    struct timeval tv;

    // for device
//    int fd_device = -1;

    // step 1: open the device with /dev/xxxxx if necessary
//    fd_device = open( "/dev/xxxxx, O_RDWR);

    // step 2: read configure file, get the default parameters, and set the device to initial status


    // step 3: connect to hibus server
    fd_hibus = hibus_connect_via_unix_socket(SOCKET_PATH, APP_INETD_NAME, RUNNER_TEMPLATE_NAME, &hibus_context);
    if(fd_hibus <= 0)
    {
        fprintf(stderr, "TEMPLATE DAEMON: connect to HIBUS server error!\n");
        return NULL;
    }

    // step 4: register remote invocation
    ret_code = hibus_register_procedure(hibus_context, METHOD_TEMPLATE_SCAN, NULL, NULL, scan_template_handler);
    if(ret_code)
    {
        fprintf(stderr, "TEMPLATE DAEMON: Error for hibus_register_procedure, return code is %d\n", ret_code);
        return NULL;
    }

    // step 5: register an event
    hibus_register_event(hibus_context, EVENT_TEMPLATE_SIGNAL, NULL, NULL);

    // step 6: check device status periodically
    // set timer
    if(clock_gettime(CLOCK_REALTIME, &now) == -1)
    {
        fprintf(stderr, "TEMPLATE DAEMON: Get now time error!\n");
        return NULL;
    }

    // start from now
    new_value.it_value.tv_sec = now.tv_sec;
    new_value.it_value.tv_nsec = now.tv_nsec;

    // set the interval
    new_value.it_interval.tv_sec = 1;
    new_value.it_interval.tv_nsec = 0;
    
    // create timer
    fd_timer = timerfd_create(CLOCK_REALTIME, 0);
    if(fd_timer == -1)
    {
        fprintf(stderr, "TEMPLATE DAEMON: Create timer error!\n");
        return NULL;
    }

    // set timer
    if(timerfd_settime(fd_timer, TFD_TIMER_ABSTIME, &new_value, NULL) == -1)
    {
        fprintf(stderr, "TEMPLATE DAEMON: Set timer error!\n");
        return NULL;
    }

    FD_ZERO(&rfds);
    FD_SET(fd_timer, &rfds);
    maxfd = fd_timer;
    FD_SET(fd_hibus, &rfds);
    maxfd = (maxfd > fd_hibus)? maxfd: fd_hibus;
//    FD_SET(fd_device, &rfds);
//    maxfd = (maxfd > fd_device)? maxfd: fd_device;
    maxfd ++;

//    tv.tv_sec = xxxx;
//    tv.tv_usec = 0;

    while(1)
    {
//        ret_code = select(maxfd, &rfds, NULL, NULL, &tv);
        ret_code = select(maxfd, &rfds, NULL, NULL, NULL);

        if(ret_code == -1)
        {
            fprintf(stderr, "TEMPLATE DAEMON: Select function error!\n");
        }
        else if(ret_code > 0)
        {
            if(fd_timer != -1 && FD_ISSET(fd_timer, &rfds))
            {
                read(fd_timer, &exp, sizeof(uint64_t));
                // check the device status, or read device port, then send the event to hibus
                hibus_fire_event(hibus_context, EVENT_TEMPLATE_SIGNAL, "{\"param0\":\"abcd\"}");
            }
            else if(fd_hibus != -1 && FD_ISSET(fd_hibus, &rfds))
            {
                ret_code = hibus_wait_and_dispatch_packet(hibus_context, 1000);
                fprintf(stderr, "TEMPLATE DAEMON: hibus_wait_and_dispatch_packet return code is %d\n", ret_code);
            }
/*
            else if(fd_device != -1 && FD_ISSET(fd_device, &rfds))
            {
                read(fd_device, content, sizeof(content));
                // analyze the content
                hibus_fire_event(hibus_context, EVENT_XXXX_XXXX, "{\"param0\":\"abcd\"}")
            }
*/
        }
        else            // timeout
        {
        }

        FD_ZERO(&rfds);
        FD_SET(fd_timer, &rfds);
        maxfd = fd_timer;
        FD_SET(fd_hibus, &rfds);
        maxfd = (maxfd > fd_hibus)? maxfd: fd_hibus;
//        FD_SET(fd_device, &rfds);
//        maxfd = (maxfd > fd_device)? maxfd: fd_device;
        maxfd ++;
//        tv.tv_sec = xxxx;
//        tv.tv_usec = 0;
    }


    // step 7: free the resource
    hibus_revoke_event(hibus_context, EVENT_TEMPLATE_SIGNAL);
    hibus_revoke_procedure(hibus_context, METHOD_TEMPLATE_SCAN);
    hibus_disconnect(hibus_context);

//    close(fd_device);

	return NULL;
}
