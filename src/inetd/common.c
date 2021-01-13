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
#include "common.h"

const char *op_errors[] = {
    "success",                                  // ERR_NO
    "an error ocures in library operation",     // ERR_LIBRARY_OPERATION
    "can not get devices list.",                // ERR_NONE_DEVICE_LIST
    "wrong procedure name.",                    // ERR_WRONG_PROCEDURE
    "wrong Json format.",                       // ERR_WRONG_JSON
    "can not find device name in param.",       // ERR_NO_DEVICE_NAME_IN_PARAM
    "can not find device in system.",           // ERR_NO_DEVICE_IN_SYSTEM
    "invalid network device type",              // ERR_DEVICE_TYPE
    "some error in load library",               // ERR_LOAD_LIBRARY
    "device is not WiFi device.",               // ERR_NOT_WIFI_DEVICE 
    "device has not openned.",                  // ERR_DEVICE_NOT_OPENNED 
    "an error ocurs in open wifi device.",      // ERR_OPEN_WIFI_DEVICE
    "an error ocurs in close wifi device.",     // ERR_CLOSE_WIFI_DEVICE
    "an error ocurs in open ethernet device.",  // ERR_OPEN_ETHERNET_DEVICE
    "an error ocurs in close ethernet device.", // ERR_CLOSE_ETHERNET_DEVICE
    "an error ocurs in open mobile device.",    // ERR_OPEN_MOBILE_DEVICE
    "an error ocurs in close mobile device.",   // ERR_CLOSE_MOBILE_DEVICE
    "device does not connect."                  // ERR_DEVICE_NOT_CONNECT
};

char * openDevice(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
{
    hibus_json *jo = NULL;
    hibus_json *jo_tmp = NULL;
    const char * device_name = NULL;
    int index = -1;
    int ret_code = ERR_NO;
    char * ret_string = malloc(256);
    WiFi_device * wifi_device = NULL;

    // get device array
    network_device * device = hibus_conn_get_user_data(conn);
    if(device == NULL)
    {
        ret_code = ERR_NONE_DEVICE_LIST;
        goto failed;
    }

    // get procedure name
    if(strncasecmp(to_method, METHOD_WIFI_OPEN_DEVICE, strlen(METHOD_WIFI_OPEN_DEVICE)))
    {
        ret_code = ERR_WRONG_PROCEDURE;
        goto failed;
    }

    // analyze json
    jo = hibus_json_object_from_string(method_param, strlen(method_param), 2);
    if(jo == NULL)
    {
        ret_code = ERR_WRONG_JSON;
        goto failed;
    }

    // get device name
    if(json_object_object_get_ex(jo, "device", &jo_tmp) == 0)
    {
        ret_code = ERR_WRONG_JSON;
        goto failed;
    }

    device_name = json_object_get_string(jo_tmp);
    if(device_name && strlen(device_name) == 0)
    {
        ret_code = ERR_NO_DEVICE_NAME_IN_PARAM;
        goto failed;
    }

    // device does exist?
    index = get_device_index(device, device_name);
    if(index == -1)
    {
        ret_code = ERR_NO_DEVICE_IN_SYSTEM;
        goto failed;
    }

    if(device[index].type == DEVICE_TYPE_WIFI)
    {
        // whether library has been loaded
        wifi_device = (WiFi_device *)device[index].device;
        if(wifi_device == NULL)
        {
            ret_code = ERR_LOAD_LIBRARY;
            goto failed;
        }
        else
        {
            // if the interface is down, up it now
            if((device[index].status == DEVICE_STATUS_DOWN) || (device[index].status == DEVICE_STATUS_UNCERTAIN))
            {
                ret_code = ERR_OPEN_WIFI_DEVICE;
                if(ifconfig_helper(device_name, 1))
                    goto failed;
            }

            // if the device is not openned
            ret_code = ERR_NO;
            if(wifi_device->context == NULL)
                ret_code = wifi_device->wifi_device_Ops->open(device_name, &(wifi_device->context));
            if(ret_code != 0)
                goto failed;

            get_if_info(&device[index]);
            wifi_device->wifi_device_Ops->set_scan_interval(wifi_device->context, wifi_device->scan_time);
        }
    }
    else if(device[index].type == DEVICE_TYPE_ETHERNET)
    {
        ret_code = ERR_OPEN_ETHERNET_DEVICE;
        if(ifconfig_helper(device_name, 1))
            goto failed;
        ret_code = ERR_NO;
    }
    else if(device[index].type == DEVICE_TYPE_MOBILE)
    {
        ret_code = ERR_OPEN_MOBILE_DEVICE;
        if(ifconfig_helper(device_name, 1))
            goto failed;
        ret_code = ERR_NO;
    }
    else
        ret_code = ERR_DEVICE_TYPE; 

failed:
    if(jo)
        json_object_put (jo);

    memset(ret_string, 0, 256);
    sprintf(ret_string, "{\"errCode\":%d, \"errMsg\":\"%s\"}", ret_code, op_errors[-1 * ret_code]);
    return ret_string;

}

char * closeDevice(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
{
    hibus_json *jo = NULL;
    hibus_json *jo_tmp = NULL;
    const char * device_name = NULL;
    int index = -1;
    int ret_code = 0;
    char * ret_string = malloc(256);
    WiFi_device * wifi_device = NULL;

    network_device * device = hibus_conn_get_user_data(conn);
    if(device == NULL)
    {
        ret_code = ERR_NONE_DEVICE_LIST;
        goto failed;
    }

    if(strncasecmp(to_method, METHOD_WIFI_CLOSE_DEVICE, strlen(METHOD_WIFI_CLOSE_DEVICE)))
    {
        ret_code = ERR_WRONG_PROCEDURE;
        goto failed;
    }

    jo = hibus_json_object_from_string(method_param, strlen(method_param), 2);
    if(jo == NULL)
    {
        ret_code = ERR_WRONG_JSON;
        goto failed;
    }

    if(json_object_object_get_ex(jo, "device", &jo_tmp) == 0)
    {
        ret_code = ERR_WRONG_JSON;
        goto failed;
    }

    device_name = json_object_get_string(jo_tmp);
    if(device_name && strlen(device_name) == 0)
    {
        ret_code = ERR_NO_DEVICE_NAME_IN_PARAM;
        goto failed;
    }

    index = get_device_index(device, device_name);
    if(index == -1)
    {
        ret_code = ERR_NO_DEVICE_IN_SYSTEM;
        goto failed;
    }

    if(device[index].type == DEVICE_TYPE_WIFI)
    {
        // whether library has been loaded
        wifi_device = (WiFi_device *)device[index].device;
        if(wifi_device == NULL)
        {
            ret_code = ERR_LOAD_LIBRARY;
            goto failed;
        }
        else
        {
            // if the interface is active
            if((device[index].status == DEVICE_STATUS_LINK) || (device[index].status == DEVICE_STATUS_UNLINK))
            {
                ret_code = ERR_CLOSE_WIFI_DEVICE;
                if(ifconfig_helper(device_name, 0))
                    goto failed;
            }

            // if the device is openned
            ret_code = ERR_NO;
            if(wifi_device->context)
            {
                ret_code = wifi_device->wifi_device_Ops->close(wifi_device->context);
                wifi_device->context = NULL;
            }
        }
    }
    else if(device[index].type == DEVICE_TYPE_ETHERNET)
    {
        ret_code = ERR_CLOSE_ETHERNET_DEVICE;
        if(ifconfig_helper(device_name, 0))
            goto failed;
        ret_code = ERR_NO;
    }
    else if(device[index].type == DEVICE_TYPE_MOBILE)
    {
        ret_code = ERR_CLOSE_MOBILE_DEVICE;
        if(ifconfig_helper(device_name, 0))
            goto failed;
        ret_code = ERR_NO;
    }
    else
        ret_code = ERR_DEVICE_TYPE; 

failed:
    if(jo)
        json_object_put (jo);

    memset(ret_string, 0, 256);
    sprintf(ret_string, "{\"errCode\":%d, \"errMsg\":\"%s\"}", ret_code, op_errors[-1 * ret_code]);
    return ret_string;
}

char * getNetworkDevicesStatus(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
{
    int i = 0;
    int ret_code = 0;
    char * ret_string = malloc(4096);
    char * type = NULL;
    char * status = NULL;
    int first = true;

    network_device * device = hibus_conn_get_user_data(conn);
    if(device == NULL)
    {
        ret_code = ERR_NONE_DEVICE_LIST;
        goto failed;
    }

    if(strncasecmp(to_method, METHOD_WIFI_GET_DEVICES_STATUS, strlen(METHOD_WIFI_GET_DEVICES_STATUS)))
    {
        ret_code = ERR_WRONG_PROCEDURE;
        goto failed;
    }

    memset(ret_string, 0, 4096);
    sprintf(ret_string, "{\"data\":[");
    for(i = 0; i < MAX_DEVICE_NUM; i++)
    {
        if(device[i].ifname[0])
        {
            if(device[i].type == DEVICE_TYPE_WIFI)
                type = "wifi";
            else if(device[i].type == DEVICE_TYPE_ETHERNET)
                type = "ethernet";
            else if(device[i].type == DEVICE_TYPE_MOBILE)
                type = "mobile";
            else if(device[i].type == DEVICE_TYPE_LO)
                type = "lo";
            else
                continue;

            if((device[i].status == DEVICE_STATUS_UNCERTAIN) || (device[i].status == DEVICE_STATUS_DOWN))
                status = "down";
            else if(device[i].status == DEVICE_STATUS_UP)
                status = "up";
            else if(device[i].status == DEVICE_STATUS_LINK)
                status = "link";
            else
                status = "unlink";

            if(!first)
            {
                sprintf(ret_string + strlen(ret_string), ",");
            }
            first = false;

            sprintf(ret_string + strlen(ret_string), 
                    "{"
                        "\"device\":\"%s\","
                        "\"type\":\"%s\","
                        "\"status\":\"%s\","
                        "\"mac\":\"%s\","
                        "\"ip\":\"%s\","
                        "\"broadcast\":\"%s\","
                        "\"subnetmask\":\"%s\""
                    "}",
                    device[i].ifname, type, status, device[i].mac,
                    device[i].ip, device[i].broadAddr, device[i].subnetMask);
        }
        else
            break;

    }
failed:
    sprintf(ret_string + strlen(ret_string), "],\"errCode\":%d, \"errMsg\":\"%s\"}", ret_code, op_errors[-1 * ret_code]);
    return ret_string;
}

void common_register(hibus_conn * hibus_context_inetd)
{
    int ret_code = 0;

    ret_code = hibus_register_procedure(hibus_context_inetd, METHOD_WIFI_OPEN_DEVICE, NULL, NULL, openDevice);
    if(ret_code)
    {
        fprintf(stderr, "WIFI DAEMON: Error for register procedure %s, %s.\n", METHOD_WIFI_OPEN_DEVICE, hibus_get_err_message(ret_code));
        return;
    }

    ret_code = hibus_register_procedure(hibus_context_inetd, METHOD_WIFI_CLOSE_DEVICE, NULL, NULL, closeDevice);
    if(ret_code)
    {
        fprintf(stderr, "WIFI DAEMON: Error for register procedure %s, %s.\n", METHOD_WIFI_CLOSE_DEVICE, hibus_get_err_message(ret_code));
        return;
    }

    ret_code = hibus_register_procedure(hibus_context_inetd, METHOD_WIFI_GET_DEVICES_STATUS, NULL, NULL, getNetworkDevicesStatus);
    if(ret_code)
    {
        fprintf(stderr, "WIFI DAEMON: Error for register procedure %s, %s.\n", METHOD_WIFI_GET_DEVICES_STATUS, hibus_get_err_message(ret_code));
        return;
    }

    ret_code = hibus_register_event(hibus_context_inetd, NETWORKDEVICECHANGED, NULL, NULL);
    if(ret_code)
    {
        fprintf(stderr, "WIFI DAEMON: Error for register event %s, %s.\n", NETWORKDEVICECHANGED, hibus_get_err_message(ret_code));
        return;
    }
}

void common_revoke(hibus_conn * hibus_context_inetd)
{
    hibus_revoke_event(hibus_context_inetd, NETWORKDEVICECHANGED);

    hibus_revoke_procedure(hibus_context_inetd, METHOD_WIFI_OPEN_DEVICE);
    hibus_revoke_procedure(hibus_context_inetd, METHOD_WIFI_CLOSE_DEVICE);
    hibus_revoke_procedure(hibus_context_inetd, METHOD_WIFI_GET_DEVICES_STATUS);
}
