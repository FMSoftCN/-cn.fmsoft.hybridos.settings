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

// get device index from device array
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

unsigned int is_if_changed(const network_device * pre, const network_device * now)
{
    int changed = 0;

    if((pre == NULL) || (now == NULL))
        return changed;

    if(strcmp(pre->ifname, now->ifname))
        changed |= NETWORK_CHANGED_NAME;
    if(pre->type != now->type)
        changed |= NETWORK_CHANGED_TYPE;
    if(pre->status != now->status)
        changed |= NETWORK_CHANGED_STATUS;
    if(strcmp(pre->mac, now->mac))
        changed |= NETWORK_CHANGED_MAC;
    if(strcmp(pre->ip, now->ip))
        changed |= NETWORK_CHANGED_IP;
    if(strcmp(pre->broadAddr, now->broadAddr))
        changed |= NETWORK_CHANGED_BROADCAST;
    if(strcmp(pre->subnetMask, now->subnetMask))
        changed |= NETWORK_CHANGED_SUBNETMASK;
        
    return changed;
}

// ifconfig ifname up|down
int ifconfig_helper(const char *if_name, const int up)
{
    int fd;
    struct ifreq ifr;

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
        return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, IFNAMSIZ);

    if(ioctl(fd, SIOCGIFFLAGS, &ifr) != 0) 
    {
        close(fd);
        return -2;
    }

    if(up)
        ifr.ifr_flags |= IFF_UP;
    else
        ifr.ifr_flags &= ~IFF_UP;

    if(ioctl(fd, SIOCSIFFLAGS, &ifr) != 0) 
    {
        close(fd);
        return -3;
    }

    close(fd);

    return 0;
}

int get_if_info(network_device * device)
{
    struct ifreq ifr;
    struct iwreq wrq;
    int socket_fd = -1;
    int ret = 0;

    if((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return -1;

    // initialize
    device->type = DEVICE_TYPE_UNKONWN;
    device->status = DEVICE_STATUS_UNCERTAIN;
    memset(device->mac, 0, NETWORK_ADDRESS_LENGTH);
    memset(device->ip, 0, NETWORK_ADDRESS_LENGTH);
    memset(device->broadAddr, 0, NETWORK_ADDRESS_LENGTH);
    memset(device->subnetMask, 0, NETWORK_ADDRESS_LENGTH);
    device->speed = 0;

    // set device interface name
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, device->ifname);

    // get device status: down or up
    if(ioctl(socket_fd, SIOCGIFFLAGS, &ifr) != 0) 
    {
        close(socket_fd);
        return -2;
    }
    if(ifr.ifr_flags & IFF_UP)
        device->status = DEVICE_STATUS_UP;
    else
        device->status = DEVICE_STATUS_DOWN;

    // get device type: lo, ethernet, wifi, mobile
    if(ifr.ifr_flags & IFF_LOOPBACK)
        device->type = DEVICE_TYPE_LO;
    else
    {
        strncpy(wrq.ifr_name, device->ifname, IFNAMSIZ);
        ret = ioctl(socket_fd, SIOCGIWNAME, &wrq);
        if(ret < 0)
            device->type = DEVICE_TYPE_ETHERNET;
        else
        {
            device->type = DEVICE_TYPE_WIFI;

            // get wifi speed
            ret = ioctl(socket_fd, SIOCGIWRATE, &wrq);
            if(ret >= 0)
                device->speed = wrq.u.bitrate.value / 1000000;
        }
    }
    // TODO: how to judge mobile device

    //get the mac of this interface  
    if (!ioctl(socket_fd, SIOCGIFHWADDR, &ifr))
    {
        snprintf(device->mac, sizeof(device->mac), "%02x:%02x:%02x:%02x:%02x:%02x",
                (unsigned char)ifr.ifr_hwaddr.sa_data[0],
                (unsigned char)ifr.ifr_hwaddr.sa_data[1],
                (unsigned char)ifr.ifr_hwaddr.sa_data[2],
                (unsigned char)ifr.ifr_hwaddr.sa_data[3],
                (unsigned char)ifr.ifr_hwaddr.sa_data[4],
                (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
    }

    if(device->status == DEVICE_STATUS_UP)
    {
        //get the IP of this interface  
        if (!ioctl(socket_fd, SIOCGIFADDR, &ifr))
        {
            snprintf(device->ip, sizeof(device->ip), "%s",
                    (char *)inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr));
            device->status = DEVICE_STATUS_LINK;
        }
        else
            device->status = DEVICE_STATUS_UNLINK;

        //get the broad address of this interface  
        if(!ioctl(socket_fd, SIOCGIFBRDADDR, &ifr))
        {
            snprintf(device->broadAddr, sizeof(device->broadAddr), "%s",
                    (char *)inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_broadaddr))->sin_addr));
        }

        //get the subnet mask of this interface  
        if (!ioctl(socket_fd, SIOCGIFNETMASK, &ifr))
        {
            snprintf(device->subnetMask, sizeof(device->subnetMask), "%s",
                    (char *)inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_netmask))->sin_addr));
        }

    }

    close(socket_fd);

    return 0;
}

// get all network interfaces, no matter active or inactive
int get_if_name(network_device * device)
{
    struct if_nameindex *if_ni = NULL;
    struct if_nameindex *i = NULL;
    int number = 0;

    if_ni = if_nameindex();
    if(if_ni == NULL) 
    {
    }
    else
    {
        for(i = if_ni; !(i->if_index == 0 && i->if_name == NULL); i++)
        {
            strcpy(device[number].ifname, i->if_name);
            get_if_info(&device[number]);
printf("================================ %s: %d, %d, %s, %s, %s, %s, %d\n", device[number].ifname, device[number].type, device[number].status, device[number].ip, device[number].mac, device[number].broadAddr, device[number].subnetMask, device[number].speed);
            ++number;
        }
        if_freenameindex(if_ni);
    }

    return number;
}
