#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <net/if.h>
#include <sys/timerfd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <linux/wireless.h>
#include <arpa/inet.h>
#include <errno.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <hibus.h>
#include <hibox/json.h>

#include "wifi_intf.h"
#include "inetd.h"
#include "tools.h"
#include "wifi.h"
#include "mobile.h"
#include "ethernet.h"
#include "common.h"

#undef  DAEMON
//#define DAEMON

extern void report_wifi_scan_info(network_device * device_name, wifi_hotspot * hotspots, int number);

static int load_device_library(network_device * device, int device_index, char * lib_name)
{
    char library_path[MAX_PATH];
    void * library_handle = NULL;               // handle of loaded library
	char * library_error = NULL;                // the error message during loading

    memset(library_path, 0, MAX_PATH);
    sprintf(library_path, "%s/%s", INETD_LIBRARY_PATH, lib_name);

    if((access(library_path, F_OK)) == -1)   
    {   
        fprintf(stderr, "INETD: library file %s does not exist, ignore it!", library_path);
        return -1;
    }

    library_handle = dlopen(library_path, RTLD_LAZY);  
    if(!library_handle) 
    {
        fprintf (stderr, "INETD: load %s error: %s\n", library_path, dlerror());
        return -1;
    }

    if(device[device_index].type == DEVICE_TYPE_WIFI)
    {
	    hiWiFiDeviceOps * (* __wifi_device_ops_get)(void);   // get all invoke functions 
        hiWiFiDeviceOps * wifi_device_Ops = NULL;

        __wifi_device_ops_get = (hiWiFiDeviceOps * (*) (void))dlsym(library_handle, "__wifi_device_ops_get");
        if((library_error = dlerror()) != NULL)
        {
            fprintf(stderr, "INETD: get wifi_init pointer error: %s\n", library_error);
            dlclose(library_handle);
            return -1;
        }
        wifi_device_Ops = __wifi_device_ops_get();

        if(wifi_device_Ops)
        {
            wifi_device_Ops->report_wifi_scan_info = report_wifi_scan_info;
            wifi_device_Ops->device = &device[device_index];

            WiFi_device * wifi_device = malloc(sizeof(WiFi_device));
            if(wifi_device)
            {
                memset(wifi_device, 0, sizeof(WiFi_device));
                wifi_device->wifi_device_Ops = wifi_device_Ops;
                pthread_mutex_init(&wifi_device->list_mutex, NULL);
                device[device_index].device = (void *)wifi_device;
                device[device_index].lib_handle = library_handle;
            }
            else
            {
                dlclose(library_handle);
                return -1;
            }
        }
        else
        {
            dlclose(library_handle);
            return -1;
        }
    }

    return 0;
}

static int init_from_etc_file(network_device * device, int device_num)
{
    int i = 0;
    int result = 0;
    int device_index = 0;
    int library_load_num = 0;

    char config_path[MAX_PATH];             // configure file full path
    char config_item[64];
    char config_content[64];                // storage path of libraries

    memset(config_path, 0, MAX_PATH);
    sprintf(config_path, "%s", INETD_CONFIG_FILE);

    memset(config_item, 0, 64);
    memset(config_content, 0, 64);
    sprintf(config_item, "device%d_name", i);

    while(GetValueFromEtcFile(config_path, "device", config_item, config_content, ETC_MAXLINE) == ETC_OK)
    {
        // get device index in device array
        device_index = get_device_index(device, config_content);
        if(device_index >= 0)
        {
            // get device type
            memcpy(config_item, config_content, 64);
            memset(config_content, 0, 64);
            result = DEVICE_TYPE_DEFAULT;
            
            sprintf(config_path, "%s", INETD_CONFIG_FILE);
            if(GetValueFromEtcFile(config_path, config_item, "type", config_content, ETC_MAXLINE) == ETC_OK)
            {
                if(strncasecmp(config_content, "wifi", 4) == 0)
                   result = DEVICE_TYPE_WIFI; 
                else if(strncasecmp(config_content, "ethernet", 8) == 0)
                   result = DEVICE_TYPE_ETHERNET; 
                else if(strncasecmp(config_content, "mobile", 6) == 0)
                   result = DEVICE_TYPE_MOBILE; 
                else if(strncasecmp(config_content, "lo", 2) == 0)
                   result = DEVICE_TYPE_LO;
                else
                    result = DEVICE_TYPE_ETHERNET;
            }

            if(result == device[device_index].type)
            {
                // get library name
                memset(config_content, 0, 64);
            
                sprintf(config_path, "%s", INETD_CONFIG_FILE);
                if(GetValueFromEtcFile(config_path, config_item, "engine", config_content, ETC_MAXLINE) == ETC_OK)
                {
                    // load library
                    if(load_device_library(device, device_index, config_content) == 0)
                    {
                        library_load_num++;
                        if(result == DEVICE_TYPE_WIFI)
                        {
                            WiFi_device * wifi_device = (WiFi_device *)device[device_index].device;

                            sprintf(config_path, "%s", INETD_CONFIG_FILE);
                            GetIntValueFromEtcFile(config_path, config_item, "priority", &(device[device_index].priority));

                            sprintf(config_path, "%s", INETD_CONFIG_FILE);
                            GetIntValueFromEtcFile(config_path, config_item, "scan_time", &(wifi_device->scan_time));
                            if(wifi_device->scan_time == 0)
                                wifi_device->scan_time = DEFAULT_SCAN_TIME;
#ifdef gengyue
                            sprintf(config_path, "%s", INETD_CONFIG_FILE);
                            if(GetValueFromEtcFile(config_path, config_item, "start", config_content, ETC_MAXLINE) == ETC_OK)
                            {
                                if(strncasecmp(config_content, "enabled", 7) == 0)
                                    device[device_index].status = DEVICE_STATUS_UP;
                                else
                                    device[device_index].status = DEVICE_STATUS_DOWN;
                            }
                            else
                                device[device_index].status = DEVICE_STATUS_DOWN;
#endif
                            // TODO: up or down the device
                        }
                        else if(result == DEVICE_TYPE_ETHERNET)
                        {
                        }
                        else if(result == DEVICE_TYPE_MOBILE)
                        {
                        }
                    }
                    else
                        fprintf(stderr, "WIFI DAEMON: can not load library %s.\n", config_content);
                }
                else
                    fprintf(stderr, "WIFI DAEMON: can not get library name for %s.\n", config_item);
            }
            else
                fprintf(stderr, "WIFI DAEMON: can not get device type for %s.\n", config_item);
        }

        // get the next device
        i ++;
        memset(config_item, 0, 64);
        sprintf(config_item, "device%d_name", i);
    }
    
    return library_load_num;
}

int main(void)
{
    int i = 0;

    // for network interface
    network_device device[MAX_DEVICE_NUM];
    int device_num = 0;

    // for hibus
    int fd_hibus_inetd = -1;
    hibus_conn * hibus_context_inetd = NULL;
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

    // step 1: get network device interfaces
    memset(device, 0, sizeof(network_device) * MAX_DEVICE_NUM);
    device_num = get_if_name(device);
    if(device_num == 0)
    {
        fprintf(stderr, "WIFI DAEMON: can not find any network interface, exit.\n");
        exit(1);
    }

    // step 2: get library setting from configure file
    if(init_from_etc_file(device, device_num) == 0)
    {
        fprintf(stderr, "WIFI DAEMON: can not load any library, exit.\n");
        exit(1);
    }

    // step 3: connect to hibus server
    for(i = 0; i < device_num; i++)
    {
        if(((device[i].type == DEVICE_TYPE_WIFI) || (device[i].type == DEVICE_TYPE_ETHERNET) || (device[i].type == DEVICE_TYPE_MOBILE))
                && (fd_hibus_inetd == -1))
        {
            fd_hibus_inetd = hibus_connect_via_unix_socket(SOCKET_PATH, APP_NAME_SETTINGS, RUNNER_NAME_INETD, &hibus_context_inetd);
            if(fd_hibus_inetd <= 0)
            {
                fprintf(stderr, "WIFI DAEMON: inetd runner connects to HIBUS server error, %s.\n", hibus_get_err_message(fd_hibus_inetd));
                exit(1);
            }
            hibus_conn_set_user_data(hibus_context_inetd, &device);
            break;
        }
    }
    if(i >= device_num)
    {
        fprintf(stderr, "WIFI DAEMON: No runner connects to HIBUS server.Exit.\n");
        exit(1);
    }

    // step 4: register remote invocation
    common_register(hibus_context_inetd);
    wifi_register(hibus_context_inetd);
    ethernet_register(hibus_context_inetd);
    mobile_register(hibus_context_inetd);

    // step 5: check device status periodically
    while(1)
    {
        ret_code = hibus_wait_and_dispatch_packet(hibus_context_inetd, 1000);
        if(ret_code)
            fprintf(stderr, "WIFI DAEMON: WiFi error for hibus_wait_and_dispatch_packet, %s.\n", hibus_get_err_message(ret_code));
#ifdef gengyue       
        for(i = 0; i < device_num; i++)
        {
            if(device[i].type == DEVICE_TYPE_WIFI)
            {
                WiFi_device * wifi_device = device[i].device;
                if(wifi_device && wifi_device->context != NULL)
                {
                    char reply[4096];
                    memset(reply, 0, 4096);
                    ret_code = wifi_device->wifi_device_Ops->get_cur_net_info(wifi_device->context, reply, 4096);
                }
            }
        }
#endif
    }

    // step 6: free the resource
    mobile_revoke(hibus_context_inetd);
    ethernet_revoke(hibus_context_inetd);
    wifi_revoke(hibus_context_inetd);
    common_revoke(hibus_context_inetd);

    hibus_disconnect(hibus_context_inetd);

    for(int i = 0; i < device_num; i++)
    {
        if((device[i].status != DEVICE_STATUS_UNCERTAIN) && (device[i].status != DEVICE_STATUS_UNCERTAIN))
        {
            if(device[i].type == DEVICE_TYPE_WIFI)
            {
                WiFi_device * wifi_device = (WiFi_device *)device[i].device;
                wifi_device->wifi_device_Ops->close(wifi_device->context);
            }
        }

        if(device[i].device)
            free(device[i].device);

        if(device[i].lib_handle)
            dlclose(device[i].lib_handle);
    }

	return 0;
}
