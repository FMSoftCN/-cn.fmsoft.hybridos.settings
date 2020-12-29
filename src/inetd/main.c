#include <unistd.h>
#include <stdio.h>
#include <sys/timerfd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <hibus.h>

#include "inetd.h"

#undef  DAEMON
//#define DAEMON

//extern void * start_wifi(void * args);

int main(void)
{
    // for configure
    char* etc_value = NULL;                     // get the configure file path from ENV
    char config_path[MAX_PATH];                 // configure file full path
    char library_path[MAX_PATH];                // storage path of libraries

    // for loading library
    int ret = 0;
    void * library_handle = NULL;               // handle of loaded library
	char * library_error = NULL;                // the error message during loading
    HibusInvokeOps * hibusOps;                  // procedure invocation structure.


    // for device
    int fd_device = -1;                         // device fd

    // for hibus
    int fd_hibus = -1;                          // socket for communication with hibus
    hibus_conn * hibus_context = NULL;          // context of communication
    int ret_code = 0;

    // for timer
    int fd_timer = -1;                          // fd for timer
    struct itimerspec new_value;                // start and interval time for timer
    struct timespec now;                        // time now
    uint64_t exp = 0;                           // for timer read

    // for select
    fd_set rfds;
    int maxfd = 0;

	HibusInvokeOps * (* wifi_ops_get)(hibus_conn *);     // get all invoke functions 

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

    // step 1: connect to hibus server
    fd_hibus = hibus_connect_via_unix_socket(SOCKET_PATH, APP_INETD_NAME, RUNNER_WIFI_NAME, &hibus_context);
    if(fd_hibus <= 0)
    {
        fprintf(stderr, "======================================= WIFI DAEMON: connect to HIBUS server error, %s.\n", hibus_get_err_message(fd_hibus));
        exit(1);
    }

    // step 2: get library setting from configure file
    memset(config_path, 0, MAX_PATH);
    memset(library_path, 0, MAX_PATH);
    if ((etc_value = getenv ("INETD_CFG_PATH")))
    {
        int len = strlen(etc_value);
        if (etc_value[len - 1] == '/')
            sprintf(config_path, "%s%s", etc_value, INETD_CONFIG_FILE);
        else
            sprintf(config_path, "%s/%s", etc_value, INETD_CONFIG_FILE);
    }
    else
        sprintf(config_path, "%s", INETD_CONFIG_FILE);

    if(GetValueFromEtcFile(config_path, "system", "library_path", library_path, ETC_MAXLINE) != ETC_OK)
    {
        fprintf(stderr, "INETD: read library path error, exit now!");
        exit(1);
    }

    fprintf(stderr, "INETD: read from configure file.\n");


    // step 3: get the library path
    strcat(library_path, "/lib");

    if(GetValueFromEtcFile(config_path, "library", "lib", library_path + strlen(library_path), ETC_MAXLINE) != ETC_OK)
    {
        fprintf(stderr, "INETD: get library name error, exit now!");
        exit(1);
    }
    strcat(library_path, ".so");

    if((access(library_path, F_OK)) == -1)   
    {   
        fprintf(stderr, "INETD: library file %s does not exist, ignore it!", library_path);
        exit(1);
    }

    fprintf(stderr, "INETD: library path is %s.\n", library_path);

    // step 4: load library
    library_handle = dlopen(library_path, RTLD_LAZY);  
    if(!library_handle) 
    {
        fprintf (stderr, "load %s error: %s\n", library_path, dlerror());
        exit(1);
    }

    fprintf(stderr, "INETD: load library %s ok.\n", library_path);


    // step 5: get functions collecition from library 
    wifi_ops_get = (HibusInvokeOps * (*) (hibus_conn *))dlsym(library_handle, "wifi_ops_get");
    if((library_error = dlerror()) != NULL)
    {
        fprintf (stderr, "get wifi_init pointer error: %s\n", library_error);
        exit(1);
    }
    hibusOps = wifi_ops_get(hibus_context);

    // step 6: register remote invocation
    ret_code = hibus_register_procedure(hibus_context, METHOD_WIFI_SCAN, NULL, NULL, hibusOps->wifi_scan_handler);
    if(ret_code)
    {
        fprintf(stderr, "======================================= WIFI DAEMON: Error for register procedure %s, %s.\n", METHOD_WIFI_SCAN, hibus_get_err_message(ret_code));
        exit(1);
    }

    ret_code = hibus_register_procedure(hibus_context, METHOD_WIFI_CONNECT, NULL, NULL, hibusOps->wifi_connect_handler);
    if(ret_code)
    {
        fprintf(stderr, "======================================= WIFI DAEMON: Error for register procedure %s, %s.\n", METHOD_WIFI_CONNECT, hibus_get_err_message(ret_code));
        exit(1);
    }

    ret_code = hibus_register_procedure(hibus_context, METHOD_WIFI_DISCONN, NULL, NULL, hibusOps->wifi_disconnect_handler);
    if(ret_code)
    {
        fprintf(stderr, "======================================= WIFI DAEMON: Error for register procedure %s, %s.\n", METHOD_WIFI_DISCONN, hibus_get_err_message(ret_code));
        exit(1);
    }

    // step 7: register an event
    ret_code = hibus_register_event(hibus_context, EVENT_WIFI_SIGNAL, NULL, NULL);
    if(ret_code)
    {
        fprintf(stderr, "======================================= WIFI DAEMON: Error for register event %s, %s.\n", EVENT_WIFI_SIGNAL, hibus_get_err_message(ret_code));
        exit(1);
    }

    ret_code = hibus_register_event(hibus_context, EVENT_WIFI_SCAN, NULL, NULL);
    if(ret_code)
    {
        fprintf(stderr, "======================================= WIFI DAEMON: Error for register event %s, %s.\n", EVENT_WIFI_SCAN, hibus_get_err_message(ret_code));
        exit(1);
    }

    ret_code = hibus_register_event(hibus_context, EVENT_WIFI_CONNECT, NULL, NULL);
    if(ret_code)
    {
        fprintf(stderr, "======================================= WIFI DAEMON: Error for register event %s, %s.\n", EVENT_WIFI_CONNECT, hibus_get_err_message(ret_code));
        exit(1);
    }

    ret_code = hibus_register_event(hibus_context, EVENT_WIFI_BROKEN, NULL, NULL);
    if(ret_code)
    {
        fprintf(stderr, "======================================= WIFI DAEMON: Error for register event %s, %s.\n", EVENT_WIFI_BROKEN, hibus_get_err_message(ret_code));
        exit(1);
    }

    ret_code = hibus_register_event(hibus_context, EVENT_WIFI_RESUME, NULL, NULL);
    if(ret_code)
    {
        fprintf(stderr, "======================================= WIFI DAEMON: error for register event %s, %s.\n", EVENT_WIFI_RESUME, hibus_get_err_message(ret_code));
        exit(1);
    }

    // step 8: initialize device
    ret_code = hibusOps->open_device(&fd_device);
    if(ret_code)
    {
        fprintf(stderr, "======================================= WIFI DAEMON: error for open device.\n");
        exit(1);
    }

    // step 9: check device status periodically
    // set timer
    if(clock_gettime(CLOCK_REALTIME, &now) == -1)
    {
        fprintf(stderr, "WIFI DAEMON: Get now time for template error!\n");
        exit(1);
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
        fprintf(stderr, "WIFI DAEMON: Create timer error!\n");
        exit(1);
    }

    // set timer
    if(timerfd_settime(fd_timer, TFD_TIMER_ABSTIME, &new_value, NULL) == -1)
    {
        fprintf(stderr, "WIFI DAEMON: Set timer error!\n");
        exit(1);
    }

    FD_ZERO(&rfds);
    FD_SET(fd_timer, &rfds);
    maxfd = fd_timer;
    FD_SET(fd_hibus, &rfds);
    maxfd = (maxfd > fd_hibus)? maxfd: fd_hibus;
    if(fd_device != -1)
    {
        FD_SET(fd_device, &rfds);
        maxfd = (maxfd > fd_device)? maxfd: fd_device;
    }
    maxfd ++;

    while(1)
    {
        ret_code = select(maxfd, &rfds, NULL, NULL, NULL);

        if(ret_code == -1)
        {
            fprintf(stderr, "======================================= WIFI DAEMON: Select function error!\n");
        }
        else if(ret_code > 0)
        {
            if(fd_timer != -1 && FD_ISSET(fd_timer, &rfds))
            {
                read(fd_timer, &exp, sizeof(uint64_t));
                hibusOps->wifi_scan();
                hibusOps->wifi_signal();
            }
            else if(fd_hibus != -1 && FD_ISSET(fd_hibus, &rfds))
            {
                ret_code = hibus_wait_and_dispatch_packet(hibus_context, 1000);
                if(ret_code)
                    fprintf(stderr, "======================================= WIFI DAEMON: Error for hibus_wait_and_dispatch_packet, %s.\n", hibus_get_err_message(ret_code));
            }
            else if(fd_device != -1 && FD_ISSET(fd_device, &rfds))
            {
                hibusOps->device_read();
            }
        }
        else            // timeout
        {
        }

        FD_ZERO(&rfds);
        FD_SET(fd_timer, &rfds);
        maxfd = fd_timer;
        FD_SET(fd_hibus, &rfds);
        maxfd = (maxfd > fd_hibus)? maxfd: fd_hibus;
        if(fd_device != -1)
        {
            FD_SET(fd_device, &rfds);
            maxfd = (maxfd > fd_device)? maxfd: fd_device;
        }
        maxfd ++;
    }

    // step 8: free the resource
    hibus_revoke_event(hibus_context, EVENT_WIFI_SIGNAL);
    hibus_revoke_event(hibus_context, EVENT_WIFI_SCAN);
    hibus_revoke_event(hibus_context, EVENT_WIFI_CONNECT);
    hibus_revoke_event(hibus_context, EVENT_WIFI_BROKEN);
    hibus_revoke_event(hibus_context, EVENT_WIFI_RESUME);

    hibus_revoke_procedure(hibus_context, METHOD_WIFI_SCAN);
    hibus_revoke_procedure(hibus_context, METHOD_WIFI_CONNECT);
    hibus_revoke_procedure(hibus_context, METHOD_WIFI_DISCONN);

    hibus_disconnect(hibus_context);

    hibusOps->close_device(fd_device);

    if(library_handle)
    {
        dlclose(library_handle);
        library_handle = NULL;
    }

	return 0;
}
