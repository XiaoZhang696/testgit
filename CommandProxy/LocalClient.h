#ifndef LOCAL_CLIENT
#define LOCAL_CLIENT
#define UNIX_DOMAIN "/opt/bin/UNIX.domain

int ConnectLocalServer(const char* szLocalFileName);
void DisConnectLocalServer(int connect_fd);
#endif
