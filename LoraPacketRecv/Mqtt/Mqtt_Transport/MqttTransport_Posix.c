/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      MqttClientLib
  Description:  Implementation of Transport Layer for MqttClientLib

  -------------------------------------------------------------------------

    This implementation is based on 'transport.c', which is part of
    'paho.mqtt.embedded-c', downloaded from:

    https://github.com/eclipse/paho.mqtt.embedded-c

    This original source file includes the following copyright notice:

      -----------------------------------------------------------------

    Copyright (c) 2014 IBM Corp.

    All rights reserved. This program and the accompanying materials
    are made available under the terms of the Eclipse Public License v1.0
    and Eclipse Distribution License v1.0 which accompany this distribution.

    The Eclipse Public License is available at
        http://www.eclipse.org/legal/epl-v10.html
    and the Eclipse Distribution License is available at
        http://www.eclipse.org/org/documents/edl-v10.php.

    Contributors:
        Ian Craggs - initial API and implementation and/or initial documentation
        Sergio R. Caprile - "commonalization" from prior samples and/or documentation extension

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/


#include <sys/types.h>


#if defined(WIN32)

    // default on Windows is 64 - increase to make Linux and Windows the same
    #define FD_SETSIZE 1024

    #include <winsock2.h>
    #include <ws2tcpip.h>

    #define MAXHOSTNAMELEN 256

    #undef  EAGAIN
    #define EAGAIN WSAEWOULDBLOCK

    #undef  EINTR
    #define EINTR WSAEINTR

    #undef  EINVAL
    #define EINVAL WSAEINVAL

    #undef  EINPROGRESS
    #define EINPROGRESS WSAEINPROGRESS

    #undef  EWOULDBLOCK
    #define EWOULDBLOCK WSAEWOULDBLOCK

    #undef  ENOTCONN
    #define ENOTCONN WSAENOTCONN

    #undef  ECONNRESET
    #define ECONNRESET WSAECONNRESET

    #define ioctl ioctlsocket
    #define socklen_t int

    #define SHUT_RD         SD_RECEIVE          // 0 = Shutdown receive operations.
    #define SHUT_WR         SD_SEND             // 1 = Shutdown send operations.
    #define SHUT_RDWR       SD_BOTH             // 2 = Shutdown both send and receive operations.

    #define MSG_DONTWAIT    0

#else

    #define INVALID_SOCKET SOCKET_ERROR
    #include <sys/socket.h>
    #include <sys/param.h>
    #include <sys/time.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <stdio.h>
    #include <unistd.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <string.h>
    #include <stdlib.h>

#endif


#if defined(WIN32)

    #include <Iphlpapi.h>

#else

    #include <sys/ioctl.h>
    #include <net/if.h>

#endif


#if !defined(SOCKET_ERROR)

    #define SOCKET_ERROR -1

#endif






/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          G L O B A L   D E F I N I T I O N S                            */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

//---------------------------------------------------------------------------
//  Local types
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Local variables
//---------------------------------------------------------------------------

// This simple low-level implementation assumes a single connection for a
// single thread. Thus, a static variable is used for that connection.
// On other scenarios, the user must solve this by taking into account that
// the current implementation of MQTTPacket_read() has a function pointer for
// a function call to get the data to a buffer, but no provisions to know the
// caller or other indicator (the socket id): int (*getfn)(unsigned char*, int)

// This low-level transport implementation was originally designed for
// single connection support only. The function 'MqttTransport_GetData()'
// doesn't has a parameter to identify the connection to used (socket id).
// That's why the higher-level implementation has to set the socket id
// by calling 'MqttTransport_SetGetDataSocket()' before calling the function
// 'MqttTransport_GetData()'.
//
// Background: The current implementation of MQTTPacket_read() has a function
// pointer for a function call to get the data to a buffer, but no provisions
// to know the caller or other indicator (the socket id):
//
//      int (*getfn)(unsigned char*, int)
//
// This global variable saves the socket id set 'MqttTransport_SetGetDataSocket()'
// for the next following call of 'MqttTransport_GetData()'. 

static int  iSocket_l = INVALID_SOCKET;



//---------------------------------------------------------------------------
//  Local functions
//---------------------------------------------------------------------------





//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//   Start Transport Layer Library
//---------------------------------------------------------------------------

int  MqttTransport_LibStart (void)
{

    // nothing to do here in POSIX version
    return (0);

}



//---------------------------------------------------------------------------
//   Stop Transport Layer Library
//---------------------------------------------------------------------------

int  MqttTransport_LibStop (void)
{

    // nothing to do here in POSIX version
    return (0);

}



//---------------------------------------------------------------------------
//   Provide Transport Layer Library CPU Time
//---------------------------------------------------------------------------

void MqttTransport_LibProcess (void)
{

    // nothing to do here in POSIX version
    return;

}



//---------------------------------------------------------------------------
//  Open Transport Connection
//---------------------------------------------------------------------------
//  return >=0 for a socket descriptor, <0 for an error code
//---------------------------------------------------------------------------

int  MqttTransport_Open (
    char* pszHostUrl_p,
    int iHostPortNum_p)
{

int                 iSocket;
struct sockaddr_in  IpV4SockAddr;
struct addrinfo*    pHostAddrInfo;
struct addrinfo     LocalAddrInfo = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};
struct addrinfo*    pWalkPtr;
int                 iRes;

#if defined(WIN32)
    WORD            wVersionRequested;
    WSADATA         wsaData;
    short           AddressFamily;
#else
    sa_family_t     AddressFamily;
#endif

#if defined(AF_INET6)
    struct sockaddr_in6  IpV6SockAddr;
#endif


    // init workspace
    iSocket       = -1;
    pHostAddrInfo = NULL;
    iSocket_l     = INVALID_SOCKET;


    // dedicated setup for WinSock
    #if defined(WIN32)
    {
        wVersionRequested = MAKEWORD(2, 2);
        iRes = WSAStartup (wVersionRequested, &wsaData);
        if (iRes != 0)
        {
            // could not find a usable Winsock DLL
            return (-1);
        }
    }
    #endif


    // ignore signal 'SIGPIPE'
    // (If the socket connection is terminated, and send() is called,
    // the program gets SIGPIPE. By default, that signal terminates the
    // program. To prevent termination, the signal is ignored.)
    signal(SIGPIPE, SIG_IGN);


    if (pszHostUrl_p[0] == '[')
    {
        ++pszHostUrl_p;
    }

    iRes = getaddrinfo (pszHostUrl_p, NULL, &LocalAddrInfo, &pHostAddrInfo);
    if (iRes == 0)
    {
        pWalkPtr = pHostAddrInfo;

        // prefer ip4 addresses
        while (pWalkPtr)
        {
            if (pWalkPtr->ai_family == AF_INET)
            {
                pHostAddrInfo = pWalkPtr;
                break;
            }
            pWalkPtr = pWalkPtr->ai_next;
        }


        #if defined(AF_INET6)
        if (pHostAddrInfo->ai_family == AF_INET6)
        {
            AddressFamily = AF_INET6;
            IpV6SockAddr.sin6_port   = htons (iHostPortNum_p);
            IpV6SockAddr.sin6_family = AddressFamily;
            IpV6SockAddr.sin6_addr   = ((struct sockaddr_in6*)(pHostAddrInfo->ai_addr))->sin6_addr;
        }
        else
        #endif

        if (pHostAddrInfo->ai_family == AF_INET)
        {
            AddressFamily = AF_INET;
            IpV4SockAddr.sin_port   = htons (iHostPortNum_p);
            IpV4SockAddr.sin_family = AddressFamily;
            IpV4SockAddr.sin_addr   = ((struct sockaddr_in*)(pHostAddrInfo->ai_addr))->sin_addr;
        }
        else
        {
            iRes = -1;
        }

        freeaddrinfo (pHostAddrInfo);
    }

    if (iRes == 0)
    {
        iSocket = socket (AddressFamily, SOCK_STREAM, 0);
        if (iSocket != -1)
        {
            #if defined(NOSIGPIPE)
            {
                int iSockOpt = 1;

                iRes = setsockopt (iSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&iSockOpt, sizeof(iSockOpt));
                if (iRes != 0)
                {
                    Log(TRACE_MIN, -1, "Could not set SO_NOSIGPIPE for socket %d", iSocket);
                }
            }
            #endif

            if (AddressFamily == AF_INET)
            {
                iRes = connect (iSocket, (struct sockaddr*)&IpV4SockAddr, sizeof(IpV4SockAddr));
            }

            #if defined(AF_INET6)
            else
            {
                iRes = connect (iSocket, (struct sockaddr*)&IpV6SockAddr, sizeof(IpV6SockAddr));
            }
            #endif
        }
    }

    if (iSocket == INVALID_SOCKET)
    {
        goto Exit;
    }


Exit:

    return (iSocket);

}



//---------------------------------------------------------------------------
//  Close Transport Connection
//---------------------------------------------------------------------------

int  MqttTransport_Close (
    int iSocket_p)
{

char  abDummyBuffer[32];
int   iRes;


    iRes = -1;

    // Windows: set socket in non-blocking mode
    #if defined(WIN32)
    {
        int  iMode = 1;     // iMode != 0 -> non-blocking mode is enabled
        iRes = ioctlsocket (iSocket_p, FIONBIO, &iMode);
    }
    #endif


    // remove all pending data
    for (;;)
    {
        iRes = recv (iSocket_p, abDummyBuffer, sizeof(abDummyBuffer), MSG_DONTWAIT);
        if (iRes <= 0)
        {
            break;
        }
    }
    shutdown (iSocket_p, SHUT_RDWR);


    #if defined(WIN32)
    {
        iRes = closesocket (iSocket_p);
        iRes = WSACleanup ();
    }
    #else
    {
        iRes = shutdown (iSocket_p, SHUT_WR);
        iRes = recv (iSocket_p, NULL, (size_t)0, 0);
        iRes = close (iSocket_p);
    }
    #endif


    iSocket_l = INVALID_SOCKET;


    return (iRes);

}



//---------------------------------------------------------------------------
//  Send Data Packet
//---------------------------------------------------------------------------

int  MqttTransport_SendPacketBuffer (
    int iSocket_p,
    unsigned char* pabDataBuff_p,
    int iDataBuffLen_p)
{

int  iFlags;
int  iRes;


    // Notice: The signal 'SIGPIPE' must be ignored here:
    //      signal(SIGPIPE, SIG_IGN);
    // see MqttTransport_Open() for explanation
    iFlags = 0;
    iRes = send (iSocket_p, pabDataBuff_p, iDataBuffLen_p, iFlags);


    return (iRes);

}



//---------------------------------------------------------------------------
//  Set Socket Handle for following call of 'MqttTransport_GetData()'
//---------------------------------------------------------------------------

int  MqttTransport_SetGetDataSocket (
    int iSocket_p)
{

int  iOldSocket;


    // This low-level transport implementation was originally designed for
    // single connection support only. The function 'MqttTransport_GetData()'
    // doesn't has a parameter to identify the connection to used (socket id).
    // That's why the higher-level implementation has to set the socket id
    // by calling this function before using 'MqttTransport_GetData()'.

    iOldSocket = iSocket_l;
    iSocket_l  = iSocket_p;


    return (iOldSocket);

}



//---------------------------------------------------------------------------
//  Get received Data Packet (blocking version)
//---------------------------------------------------------------------------

int  MqttTransport_GetData (
    unsigned char* pabDataBuff_p,
    int iDataBuffLen_p)
{

int  iRes;


    if (iSocket_l == INVALID_SOCKET)
    {
        return (-1);
    }


    // Windows: set socket in blocking mode
    #if defined(WIN32)
    {
        int  iMode = 0;     // Mode = 0 -> blocking is enabled
        iRes = ioctlsocket (iSocket_l, FIONBIO, &iMode);
    }
    #endif


    iRes = recv (iSocket_l, pabDataBuff_p, iDataBuffLen_p, 0);


    return (iRes);

}



//---------------------------------------------------------------------------
//  Get received Data Packet (non-blocking version)
//---------------------------------------------------------------------------

int  MqttTransport_GetDataNonBlock (
    void *pvSocket_p,
    unsigned char* pabDataBuff_p,
    int iDataBuffLen_p)
{

int            iSocket;
unsigned char  bPeekData;
int            iPeekErr;
int            iRes;


    iSocket = *((int *)pvSocket_p);     // pointer to whatever the system may use to identify the transport


    // Windows: set socket in non-blocking mode
    #if defined(WIN32)
    {
        int  iMode = 1;     // iMode != 0 -> non-blocking mode is enabled
        iRes = ioctlsocket (iSocket, FIONBIO, &iMode);
    }
    #endif


    // check if data are available
    iRes = recv (iSocket, &bPeekData, sizeof(bPeekData), MSG_DONTWAIT | MSG_PEEK);
    if (iRes < 0)
    {
        #if defined(WIN32)
        {
            iPeekErr = WSAGetLastError();
        }
        #else
        {
            iPeekErr = errno;
        }
        #endif

        switch (iPeekErr)
        {
            case EINTR:
            case EAGAIN:
            {
                // no data received
                iRes = 0;
                break;
            }

            default:
            {
                // case ENOTCONN:   not connected
                // case ETIMEDOUT:  recv timeout
                // default:         unknown error
                iRes = -1;
                break;
            }
        }
    }


    // read received data from socket
    if (iRes > 0)
    {
        iRes = recv (iSocket, pabDataBuff_p, iDataBuffLen_p, 0);
        if (iRes < 0)
        {
            // return error
            iRes = -1;
        }
    }


    // return:  >0 -> number of received bytes
    //          =0 -> no data received
    //          <0 -> Error
    return (iRes);

/*---------------------------------------------------------------------------

int                    iSocket;
fd_set                 ReadSockFds;
fd_set                 ExceptSockFds;
static struct timeval  tvTimeOut;
int                    iSocketsReady;
int                    iRes;


    iSocket = *((int *)pvSocket_p);     // pointer to whatever the system may use to identify the transport


    // clear the socket fd sets
    FD_ZERO (&ReadSockFds);
    FD_ZERO (&ExceptSockFds);

    // add working socket to fd sets
    FD_SET (iSocket, &ReadSockFds);
    FD_SET (iSocket, &ExceptSockFds);

    tvTimeOut.tv_sec  = 0;
    tvTimeOut.tv_usec = 0;
    iSocketsReady = select (0, &ReadSockFds, NULL, &ExceptSockFds, &tvTimeOut);

    if (iSocketsReady == SOCKET_ERROR)
    {
        return (-1);
    }


    // check for exceptions
    if ( FD_ISSET(iSocket, &ExceptSockFds) )
    {
        // return with error
        return (-1);
    }


    // check for available data
    if ( FD_ISSET(iSocket, &ReadSockFds) )
    {
        iRes = recv (iSocket, pabDataBuff_p, iDataBuffLen_p, 0);
        if (iRes < 0)
        {
            // return error
            return (iRes);
        }

        // return number of received bytes
        return (iRes);
    }


    // no data received
    return (0);

---------------------------------------------------------------------------*/

}





// EOF


