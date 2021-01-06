#ifndef __INETD_TOOLS__
#define __INETD_TOOLS__

int get_device_index(const network_device * device, const char * ifname);
int if_is_wlif(const char * ifname);
int getLocalInfo(network_device * device, int device_num);
int get_if_name(network_device * device);

#endif  // __INETD_TOOLS__
