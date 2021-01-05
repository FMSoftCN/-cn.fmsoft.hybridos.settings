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

#include "inetd.h"
#include "wifi.h"
#include "mobile.h"
#include "ethernet.h"

#undef  DAEMON
//#define DAEMON

int get_device_index(const network_device * device, const char * ifname)
{
    int i = 0;
    int find_index = -1;

    for(i = 0; i < MAX_DEVICE_NUM; i++)
    {
        if((ifname != NULL) && device[i].ifname[0] && (strncasecmp(device[i].ifname, ifname, strlen(ifname)) == 0))
        {
            find_index = i;
            break;
        }
    }

    return find_index;
}

static int if_is_wlif(const char * ifname)
{
    int skfd, ret = 0;
    struct iwreq wrq;

    /* Set device name */
    strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(skfd < 0)
        return ret;

    /* Get wireless name */
    ret = ioctl(skfd, SIOCGIWNAME, &wrq);
    close(skfd);

    /* If no wireless name : no wireless extensions */
    if(ret < 0)
        return 0;
    else
        return 1;
}

static int getLocalInfo(network_device * device, int device_num)
{
    int fd = 0;
    int interfaceNum = 0;
    struct ifreq buf[16];
    struct ifconf ifc;
    struct ifreq ifrcopy;
    int device_index = 0;

    if(device_num == 0)
        return 0;

    if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        close(fd);
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;
    if(!ioctl(fd, SIOCGIFCONF, (char *)&ifc))
    {
        interfaceNum = ifc.ifc_len / sizeof(struct ifreq);

        while (interfaceNum-- > 0)
        {
            device_index = get_device_index(device, buf[interfaceNum].ifr_name);
            if(device_index == -1)
                continue;

            //ignore the interface that not up or not runing  
            ifrcopy = buf[interfaceNum];
            if (ioctl(fd, SIOCGIFFLAGS, &ifrcopy))
            {
                close(fd);
                return -1;
            }

            //get the mac of this interface  
            if (!ioctl(fd, SIOCGIFHWADDR, (char *)(&buf[interfaceNum])))
            {
                snprintf(device[device_index].mac, sizeof(device[device_index].mac), "%02x%02x%02x%02x%02x%02x",
                        (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[0],
                        (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[1],
                        (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[2],

                        (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[3],
                        (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[4],
                        (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[5]);
            }
            else
            {
                close(fd);
                return -1;
            }

            //get the IP of this interface  
            if (!ioctl(fd, SIOCGIFADDR, (char *)&buf[interfaceNum]))
            {
                snprintf(device[device_index].ip, sizeof(device[device_index].ip), "%s",
                        (char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_addr))->sin_addr));
                device[device_index].status = DEVICE_STATUS_LINK;
            }
            else
            {
                close(fd);
                return -1;
            }

            //get the broad address of this interface  
            if(!ioctl(fd, SIOCGIFBRDADDR, &buf[interfaceNum]))
            {
                snprintf(device[device_index].broadAddr, sizeof(device[device_index].broadAddr), "%s",
                        (char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_broadaddr))->sin_addr));
            }
            else
            {
                close(fd);
                return -1;
            }

            //get the subnet mask of this interface  
            if (!ioctl(fd, SIOCGIFNETMASK, &buf[interfaceNum]))
            {
                snprintf(device[device_index].subnetMask, sizeof(device[device_index].subnetMask), "%s",
                        (char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_netmask))->sin_addr));
            }
            else
            {
                close(fd);
                return -1;

            }
        }
    }
    else
    {
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

static int get_if_name(network_device * device)
{
    struct if_nameindex *if_ni = NULL;
    struct if_nameindex *i = NULL;
    int num = 0;

    if_ni = if_nameindex();
    if(if_ni == NULL) 
    {
    }
    else
    {
        for(i = if_ni; !(i->if_index == 0 && i->if_name == NULL); i++)
        {
            strcpy(device[num].ifname, i->if_name);
            if(strncasecmp(device[num].ifname, "lo", 2) == 0)
                device[num].type = DEVICE_TYPE_LO;
            else
            {
                if(if_is_wlif(device[num].ifname))
                    device[num].type = DEVICE_TYPE_WIFI;
                else
                    device[num].type = DEVICE_TYPE_ETHERNET;

                // TODO: how to judge mobile ?
            }
            device[num].status = DEVICE_STATUS_UNLINK;

            ++num;
        }
        if_freenameindex(if_ni);
    }

    getLocalInfo(device, num);

    return num;
}

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
            WiFi_device * wifi_device = malloc(sizeof(WiFi_device));
            if(wifi_device)
            {
                memset(wifi_device, 0, sizeof(WiFi_device));
                wifi_device->wifi_device_Ops = wifi_device_Ops;
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

                            sprintf(config_path, "%s", INETD_CONFIG_FILE);
                            GetIntValueFromEtcFile(config_path, config_item, "signal_time", &(wifi_device->signal_time));
                            if(wifi_device->signal_time == 0)
                                wifi_device->signal_time = DEFAULT_SIGNAL_TIME;
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
    int fd_hibus_wifi = -1;
    hibus_conn * hibus_context_wifi = NULL;
    int fd_hibus_ethernet = -1;
    hibus_conn * hibus_context_ethernet = NULL;
    int fd_hibus_mobile = -1;
    hibus_conn * hibus_context_mobile = NULL;
    int ret_code = 0;

    // for select
    fd_set rfds;
    int maxfd = 0;
    struct timeval tm;

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
    for(int i = 0; i < device_num; i++)
    {
        if((device[i].type == DEVICE_TYPE_WIFI) && (fd_hibus_wifi == -1))
        {
            fd_hibus_wifi = hibus_connect_via_unix_socket(SOCKET_PATH, APP_INETD_NAME, RUNNER_WIFI_NAME, &hibus_context_wifi);
            if(fd_hibus_wifi <= 0)
            {
                fprintf(stderr, "WIFI DAEMON: WiFi runner connects to HIBUS server error, %s.\n", hibus_get_err_message(fd_hibus_wifi));
                exit(1);
            }
            hibus_conn_set_user_data(hibus_context_wifi, &device);
            ret_code ++;
        }
        else if((device[i].type == DEVICE_TYPE_ETHERNET) && (fd_hibus_ethernet == -1))
        {
            fd_hibus_ethernet = hibus_connect_via_unix_socket(SOCKET_PATH, APP_INETD_NAME, RUNNER_ETHERNET_NAME, &hibus_context_ethernet);
            if(fd_hibus_ethernet <= 0)
            {
                fprintf(stderr, "WIFI DAEMON: Ethernet runner connects to HIBUS server error, %s.\n", hibus_get_err_message(fd_hibus_ethernet));
                exit(1);
            }
            hibus_conn_set_user_data(hibus_context_ethernet, &device);
            ret_code ++;
        }
        else if((device[i].type == DEVICE_TYPE_MOBILE) && (fd_hibus_mobile == -1))
        {
            fd_hibus_mobile = hibus_connect_via_unix_socket(SOCKET_PATH, APP_INETD_NAME, RUNNER_MOBILE_NAME, &hibus_context_mobile);
            if(fd_hibus_mobile <= 0)
            {
                fprintf(stderr, "WIFI DAEMON: Mobile runner connects to HIBUS server error, %s.\n", hibus_get_err_message(fd_hibus_mobile));
                exit(1);
            }
            hibus_conn_set_user_data(hibus_context_mobile, &device);
            ret_code ++;
        }
    }
    if(ret_code == 0)
    {
        fprintf(stderr, "WIFI DAEMON: No runner connects to HIBUS server.Exit.\n");
        exit(1);
    }

    // step 4: register remote invocation
    if(fd_hibus_wifi != -1)
        wifi_register(hibus_context_wifi);
    if(fd_hibus_ethernet != -1)
        ethernet_register(hibus_context_ethernet);
    if(fd_hibus_mobile != -1)
        mobile_register(hibus_context_mobile);

    // step 5: check device status periodically
    FD_ZERO(&rfds);
    if(fd_hibus_wifi != -1)
    {
        FD_SET(fd_hibus_wifi, &rfds);
        maxfd = (maxfd > fd_hibus_wifi)? maxfd: fd_hibus_wifi;
    }
    if(fd_hibus_ethernet != -1)
    {
        FD_SET(fd_hibus_ethernet, &rfds);
        maxfd = (maxfd > fd_hibus_ethernet)? maxfd: fd_hibus_ethernet;
    }
    if(fd_hibus_mobile != -1)
    {
        FD_SET(fd_hibus_mobile, &rfds);
        maxfd = (maxfd > fd_hibus_mobile)? maxfd: fd_hibus_mobile;
    }
    maxfd ++;

    tm.tv_sec = 1;
    tm.tv_usec = 0;

    while(1)
    {
        ret_code = select(maxfd, &rfds, NULL, NULL, &tm);

        if(ret_code == -1)
        {
            fprintf(stderr, "WIFI DAEMON: Select function error!\n");
        }
        else if(ret_code > 0)
        {
            if(fd_hibus_wifi != -1 && FD_ISSET(fd_hibus_wifi, &rfds))
            {
                ret_code = hibus_wait_and_dispatch_packet(hibus_context_wifi, 1000);
                if(ret_code)
                    fprintf(stderr, "WIFI DAEMON: WiFi error for hibus_wait_and_dispatch_packet, %s.\n", hibus_get_err_message(ret_code));
            }
            else if(fd_hibus_ethernet != -1 && FD_ISSET(fd_hibus_ethernet, &rfds))
            {
                ret_code = hibus_wait_and_dispatch_packet(hibus_context_ethernet, 1000);
                if(ret_code)
                    fprintf(stderr, "WIFI DAEMON: Ethernet error for hibus_wait_and_dispatch_packet, %s.\n", hibus_get_err_message(ret_code));
            }
            else if(fd_hibus_mobile != -1 && FD_ISSET(fd_hibus_mobile, &rfds))
            {
                ret_code = hibus_wait_and_dispatch_packet(hibus_context_mobile, 1000);
                if(ret_code)
                    fprintf(stderr, "WIFI DAEMON: Mobile error for hibus_wait_and_dispatch_packet, %s.\n", hibus_get_err_message(ret_code));
            }
        }
        else
        {
            printf("time out\n");
//                wifi_device_Ops->start_scan(context);
//                wifi_device_Ops->get_signal_strength(context);
        }

        FD_ZERO(&rfds);
        if(fd_hibus_wifi != -1)
        {
            FD_SET(fd_hibus_wifi, &rfds);
            maxfd = (maxfd > fd_hibus_wifi)? maxfd: fd_hibus_wifi;
        }
        if(fd_hibus_ethernet != -1)
        {
            FD_SET(fd_hibus_ethernet, &rfds);
            maxfd = (maxfd > fd_hibus_ethernet)? maxfd: fd_hibus_ethernet;
        }
        if(fd_hibus_mobile != -1)
        {
            FD_SET(fd_hibus_mobile, &rfds);
            maxfd = (maxfd > fd_hibus_mobile)? maxfd: fd_hibus_mobile;
        }
        maxfd ++;

        tm.tv_sec = 1;
        tm.tv_usec = 0;
    }

    // step 6: free the resource
    if(fd_hibus_wifi != -1)
    {
        wifi_revoke(hibus_context_wifi);
        hibus_disconnect(hibus_context_wifi);
    }
    if(fd_hibus_ethernet != -1)
    {
        ethernet_revoke(hibus_context_ethernet);
        hibus_disconnect(hibus_context_ethernet);
    }
    if(fd_hibus_mobile != -1)
    {
        mobile_revoke(hibus_context_mobile);
        hibus_disconnect(hibus_context_mobile);
    }

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
