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
#include "tools.h"
#include "wifi.h"
#include "mobile.h"
#include "ethernet.h"
#include "common.h"

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

int if_is_wlif(const char * ifname)
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

int getLocalInfo(network_device * device, int device_num)
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

int get_if_name(network_device * device)
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
