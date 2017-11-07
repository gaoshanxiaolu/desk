/**
 * @file
 */
/******************************************************************************
* Copyright 2013,2015 Qualcomm Connected Experiences, Inc.
*
******************************************************************************/
#define AJ_MODULE NET

#include "aj_target.h"
#include "aj_debug.h"
#include "aj_bufio.h"
#include "aj_net.h"
#include "aj_util.h"
#include "aj_wifi_ctrl.h"
#include "aj_connect.h"

#include <qcom/qcom_time.h>
#include <qcom/qcom_network.h>
#include <qcom/socket_api.h>
#include <qcom/select_api.h>
#include <qcom/qcom_wlan.h>

#ifndef NDEBUG
AJ_EXPORT uint8_t dbgNET = 0;
#endif

extern A_UINT8 qcom_DeviceId;

#define INVALID_SOCKET -1

/*
 * Returns TRUE if the socket (fd) has been closed by the firmware
 */
#define SOCK_IS_CLOSED(fd, set) (FD_ISSET(fd, set) == -1)

/*
 * IANA assigned IPv4 multicast group for AllJoyn.
 */
static const char AJ_IPV4_MULTICAST_GROUP[] = "224.0.0.113";
/*
 * IANA assigned IPv4 multicast group for AllJoyn, in binary form
 */
#define AJ_IPV4_MCAST_GROUP     0xe0000071

/*
 * IANA assigned IPv6 multicast group for AllJoyn.
 */
//static const char AJ_IPV6_MULTICAST_GROUP[] = "ff02::13a";
static const uint8_t AJ_IPV6_MCAST_GROUP[16] = {0xFF, 0x02, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x01, 0x3A};

/*
 * IANA assigned UDP multicast port for AllJoyn
 */
#define AJ_UDP_PORT 9956

/*
 * IANA-assigned IPv4 multicast group for mDNS.
 */
//static const char MDNS_IPV4_MULTICAST_GROUP[] = "224.0.0.251";
#define MDNS_IPV4_MCAST_GROUP     0xe00000fb

/*
 * IANA-assigned IPv6 multicast group for mDNS.
 */
//static const char MDNS_IPV6_MULTICAST_GROUP[] = "ff02::fb";
static const uint8_t MDNS_IPV6_MCAST_GROUP[16] = {0xFF, 0x02, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x00, 0xFB};

/*
 * IANA-assigned UDP multicast port for mDNS
 */
#define MDNS_UDP_PORT 5353

/**
 * Target-specific context for network I/O
 */
typedef struct {
    int tcpSock;
} NetContext;

typedef struct {
    int udpSock;
    int udp6Sock;
    int mDnsSock;
    int mDns6Sock;
    int mDnsRecvSock;
    uint32_t mDnsRecvAddr;
    uint16_t mDnsRecvPort;
    uint8_t softAP;
} MCastContext;

static const MCastContext mCastContextInit = { INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, 0, 0, 0 };
static const NetContext netContextInit = { INVALID_SOCKET };
static MCastContext mCastContext = { INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, 0, 0, 0 };
static NetContext netContext = { INVALID_SOCKET };

/**
 *  control the linger behavior for sockets
 */
typedef struct {
    int l_onoff;
    int l_linger;
} AJ_Linger;

#define AJ_MAX_EXT_FDS  2

typedef struct {
    int32_t fd;
    void (*func)(void*);
    void* ctx;
} AJ_FdLookup;

static uint8_t network_up = FALSE;
static uint8_t numFdEntries = 0;
static AJ_FdLookup fdLookupTable[AJ_MAX_EXT_FDS];

/*
 * Need enough RX buffer space to receive a complete name service packet when
 * used in UDP mode.  NS expects MTU of 1500 subtracts UDP, IP and Ethernet
 * Type II overhead.  1500 - 8 -20 - 18 = 1454.  txData buffer size needs to
 * be big enough to hold a NS WHO-HAS for one name (4 + 2 + 256 = 262) in UDP
 * mode.  TCP buffer size dominates in that case.
 */
static uint8_t rxData[1454];
static uint8_t txData[1452];

/*
 * Sets all external file descriptors to the current fd set. Returns the maximum
 * file descriptor for use with select
 */
static int32_t SetExtFds(q_fd_set* fdSet)
{
    int32_t maxFd = -1;
    int i = 0;
    while (i < AJ_MAX_EXT_FDS) {
        if (fdLookupTable[i].fd != 0) {
            FD_SET(fdLookupTable[i].fd, fdSet);
            /* Find the max FD */
            if (fdLookupTable[i].fd > maxFd) {
                maxFd = fdLookupTable[i].fd;
            }
        }
        i++;
    }
    return maxFd;
}

/*
 * Checks if any external file descriptors were triggered and calls the CB.
 * Returns TRUE if any external FD was set.
 */
static uint8_t CheckExtFds(q_fd_set* fdSet)
{
    uint8_t status = FALSE;
    int i = 0;
    while (i < AJ_MAX_EXT_FDS) {
        if (fdLookupTable[i].fd != 0) {
            /* If the file descriptor was set, call the callback */
            if (FD_ISSET(fdLookupTable[i].fd, fdSet) && fdLookupTable[i].func) {
                AJ_InfoPrintf(("CheckExtFds(): FD:%u has data\n", fdLookupTable[i].fd));
                fdLookupTable[i].func(fdLookupTable[i].ctx);
                FD_CLR(fdLookupTable[i].fd, fdSet);
                status = TRUE;
            }
        }
        i++;
    }
    return status;
}

/*
 * Registers a new FD that will be selected over in AJ_Net_Read
 */
AJ_Status AJ_Net_RegisterExtFd(int32_t fd, void (*func)(void*), void* ctx)
{
    int i = 0;
    if (numFdEntries >= AJ_MAX_EXT_FDS - 1) {
        AJ_ErrPrintf(("AJ_Net_RegisterExtFd(): Can only register %d external file descriptors\n", AJ_MAX_EXT_FDS));
        return AJ_ERR_RESOURCES;
    }
    if (!func || fd == -1) {
        AJ_ErrPrintf(("AJ_Net_RegisterExtFd(): Callback function was null or file descriptor was -1\n"));
        return AJ_ERR_INVALID;
    }
    /*
     * The table has not been initialized
     */
    if (numFdEntries == 0) {
        memset(fdLookupTable, 0, sizeof(AJ_FdLookup) * AJ_MAX_EXT_FDS);
    }
    /*
     * Find an open slot.
     */
    while (i < AJ_MAX_EXT_FDS && fdLookupTable[i].fd != 0) {
        i++;
    }
    AJ_InfoPrintf(("AJ_Net_RegisterExtFd(): Registering external FD: %u, func:0x%p\n", fd, func));
    fdLookupTable[i].ctx = ctx;
    fdLookupTable[i].fd = fd;
    fdLookupTable[i].func = func;
    numFdEntries++;
    return AJ_OK;
}

/*
 * Removes a file descriptor from the table
 */
AJ_Status AJ_Net_UnregisterExtFd(int32_t fd)
{
    int i = 0;
    /*
     * Look in the table until the file descriptor is found or the end is reached
     */
    while (i < AJ_MAX_EXT_FDS && fdLookupTable[i].fd != fd) {
        i++;
    }
    if (i == AJ_MAX_EXT_FDS) {
        AJ_ErrPrintf(("AJ_Net_UnregisterExtFd(): Could not look up fd %d\n", fd));
        return AJ_ERR_INVALID;
    }
    /*
     * Unregister the entry
     */
    fdLookupTable[i].fd = 0;
    fdLookupTable[i].ctx = NULL;
    fdLookupTable[i].func = NULL;
    numFdEntries--;
    return AJ_OK;
}

uint16_t AJ_EphemeralPort(void)
{
    uint8_t bytes[2];
    AJ_RandBytes(bytes, 2);
    /*
     * Return a random port number in the IANA-suggested range
     */
    return 49152 + *((uint16_t*)bytes) % (65535 - 49152);
}

static AJ_Status ConfigMCastSock(AJ_MCastSocket* mcastSock, AJ_RxFunc rxFunc, uint32_t rxLen, AJ_TxFunc txFunc, uint32_t txLen)
{

    if (rxLen) {
        if (rxLen > sizeof(rxData)) {
            AJ_ErrPrintf(("ConfigMCastSock(): Asking for %u bytes but buffer is only %u", rxLen, sizeof(rxData)));
            return AJ_ERR_RESOURCES;
        }
    }
    AJ_IOBufInit(&mcastSock->rx, rxData, rxLen, AJ_IO_BUF_RX, (void*)&mCastContext);
    mcastSock->rx.recv = rxFunc;
    AJ_IOBufInit(&mcastSock->tx, txData, txLen, AJ_IO_BUF_TX, (void*)&mCastContext);
    mcastSock->tx.send = txFunc;
    return AJ_OK;
}

static AJ_Status ConfigNetSock(AJ_BusAttachment* bus, AJ_RxFunc rxFunc, uint32_t rxLen, AJ_TxFunc txFunc, uint32_t txLen)
{

    memset(&netContext, 0, sizeof(NetContext));

    if (rxLen) {
        if (rxLen > sizeof(rxData)) {
            AJ_ErrPrintf(("ConfigNetSock(): Asking for %u bytes but buffer is only %u", rxLen, sizeof(rxData)));
            return AJ_ERR_RESOURCES;
        }
    }
    AJ_IOBufInit(&bus->sock.rx, rxData, rxLen, AJ_IO_BUF_RX, (void*)&netContext);
    bus->sock.rx.recv = rxFunc;
    AJ_IOBufInit(&bus->sock.tx, txData, txLen, AJ_IO_BUF_TX, (void*)&netContext);
    bus->sock.tx.send = txFunc;
    return AJ_OK;
}

static AJ_Status CloseNetSock(AJ_NetSocket* netSock)
{
    NetContext* context = (NetContext*)netSock->rx.context;
    if (context) {
        if (context->tcpSock != INVALID_SOCKET) {
            network_up = FALSE;
            qcom_socket_close(context->tcpSock);
        }

        *context = netContextInit;
        memset(netSock, 0, sizeof(AJ_NetSocket));
    }
    return AJ_OK;
}

static AJ_Status CloseMCastSock(AJ_MCastSocket* mcastSock)
{
    MCastContext* context = (MCastContext*)mcastSock->rx.context;
    if (context) {
        if (context->udpSock != INVALID_SOCKET) {
            qcom_socket_close(context->udpSock);
        }
        if (context->udp6Sock != INVALID_SOCKET) {
            qcom_socket_close(context->udp6Sock);
        }
        if (context->mDnsSock != INVALID_SOCKET) {
            qcom_socket_close(context->mDnsSock);
        }
        if (context->mDns6Sock != INVALID_SOCKET) {
            qcom_socket_close(context->mDns6Sock);
        }
        if (context->mDnsRecvSock != INVALID_SOCKET) {
            qcom_socket_close(context->mDnsRecvSock);
        }
        *context = mCastContextInit;
        memset(mcastSock, 0, sizeof(AJ_MCastSocket));
    }
    network_up = FALSE;
    return AJ_OK;
}

AJ_Status AJ_Net_Send(AJ_IOBuffer* buf)
{
    NetContext* context = (NetContext*)buf->context;
    int ret;
    int tx = AJ_IO_BUF_AVAIL(buf);

    AJ_ASSERT(buf->direction == AJ_IO_BUF_TX);

    while (tx) {
        ret = qcom_send((int) context->tcpSock, (char*) buf->readPtr, tx, 0);
        if (ret < 0) {
            if( ret == -100 ) {
                // there weren't enough buffers, try again
                AJ_Sleep(100);
                continue;
            }

            AJ_ErrPrintf(("AJ_Net_Send: send() failed: %d\n", ret));
            return AJ_ERR_WRITE;
        }
        buf->readPtr += ret;
        tx -= ret;
    }
    if (AJ_IO_BUF_AVAIL(buf) == 0) {
        AJ_IO_BUF_RESET(buf);
    }
    return AJ_OK;
}

AJ_Status AJ_IntToString(int32_t val, char* buf, size_t buflen)
{
    AJ_Status status = AJ_OK;
    int c;

    c = qcom_snprintf(buf, buflen, "%d", val);
    if (c <= 0 || c > buflen) {
        status = AJ_ERR_RESOURCES;
    }
    return status;
}

AJ_Status AJ_InetToString(uint32_t addr, char* buf, size_t buflen)
{
    AJ_Status status = AJ_OK;
    int c;

    c = qcom_snprintf(buf, buflen, "%u.%u.%u.%u",
                    (addr & 0xFF000000) >> 24, (addr & 0x00FF0000) >> 16,
                    (addr & 0x0000FF00) >> 8, (addr & 0x000000FF));
    if (c <= 0 || c > buflen) {
        status = AJ_ERR_RESOURCES;
    }
    return status;
}

static int selectSock = 0;
static uint8_t interrupted = FALSE;

void AJ_Net_Interrupt()
{
    if (selectSock) {
        qcom_unblock_socket(selectSock);
        interrupted = TRUE;
    }
}

void AJ_Net_Lost()
{
    network_up = FALSE;
}

AJ_Status AJ_Net_Recv(AJ_IOBuffer* buf, uint32_t len, uint32_t timeout)
{
    q_fd_set recvFds;
    int maxFd = -1;
    NetContext* context = (NetContext*)buf->context;
    AJ_Status status = AJ_OK;
    size_t rx = AJ_IO_BUF_SPACE(buf);
    int rc = 0;
    int sockFd = (int) context->tcpSock;
    struct timeval tv = { timeout / 1000, 1000 * (timeout % 1000) };

    AJ_ASSERT(buf->direction == AJ_IO_BUF_RX);
    rx = min(rx, len);

    selectSock = context->tcpSock;
    interrupted = FALSE;

    FD_ZERO(&recvFds);
    FD_SET(sockFd, &recvFds);

    /* Find the max FD of the TCP socket and all external FD's */
    maxFd = max(SetExtFds(&recvFds), sockFd);

    if (network_up) {
        rc = qcom_select(maxFd + 1, &recvFds, NULL, NULL, &tv);
    }

    if (rc == 0) {
        if (!network_up) {
            return AJ_ERR_READ;
        } else if (interrupted) {
            return AJ_ERR_INTERRUPTED;
        } else {
            return AJ_ERR_TIMEOUT;
        }
    }
    /*
     * If an external FD was set return interrupted
     */
    if (CheckExtFds(&recvFds)) {
        return AJ_ERR_INTERRUPTED;
    } else {
        if (SOCK_IS_CLOSED(sockFd, &recvFds)) {
            AJ_ErrPrintf(("AJ_Net_Recv(): Socket has been closed, returning AJ_ERR_READ\n"));
            return AJ_ERR_READ;
        }
        rx = min(rx, len);
        if (rx) {
            rc = qcom_recv(sockFd, (char*) buf->writePtr, rx, 0);
            if (rc <= 0) {
                AJ_ErrPrintf(("AJ_Net_Recv: recv() failed: %d\n", rc));
                status = AJ_ERR_READ;
            } else {
                buf->writePtr += rc;
            }
        }
    }

    return status;
}

AJ_Status AJ_Net_Connect(AJ_BusAttachment* bus, const AJ_Service* service)
{
    AJ_Status status = AJ_ERR_CONNECT;
    int ret;
    struct sockaddr_storage addrBuf;
    int tcpSock = INVALID_SOCKET;
    int addrSize;
    AJ_Linger ling;

    memset(&addrBuf, 0, sizeof(addrBuf));

    // TODO: select until the socket is connected; use some kind of timeout
    // if POSIX-compliant, it will come back *writable* when it's connected

    if (service->addrTypes & AJ_ADDR_TCP4) {
        struct sockaddr_in* sa = (struct sockaddr_in*) &addrBuf;
        sa->sin_family = AF_INET;
        /*
         * These might appear to be mismatched, but addr is provided in network-byte order,
         * and port is in host-byte order (assignd in ParseIsAt).
         * The API expects them in network order.
         */
        sa->sin_port = htons(service->ipv4port);
        sa->sin_addr.s_addr = service->ipv4;
        addrSize = sizeof(struct sockaddr_in);
        tcpSock = qcom_socket(AF_INET, SOCK_STREAM, 0);
    } else if (service->addrTypes & AJ_ADDR_TCP6) {
        struct sockaddr_in6* sa = (struct sockaddr_in6*) &addrBuf;
        sa->sin6_family = AF_INET6;
        sa->sin6_port = htons(service->ipv6port);
        memcpy(sa->sin6_addr.addr, service->ipv6, sizeof(sa->sin6_addr.addr));
        addrSize = sizeof(struct sockaddr_in6);
        tcpSock = qcom_socket(AF_INET6, SOCK_STREAM, 0);
    } else {
        AJ_ErrPrintf(("AJ_Net_Connect(): Invalid addrTypes %u, status=AJ_ERR_CONNECT\n", service->addrTypes));
    }

    if (tcpSock == INVALID_SOCKET) {
        return AJ_ERR_CONNECT;
    }

    ret = qcom_connect(tcpSock, (struct sockaddr*) &addrBuf, addrSize);
    if (ret < 0) {
        AJ_ErrPrintf(("qcom_connect() failed: %d\n", ret));
        qcom_socket_close(tcpSock);
        return AJ_ERR_CONNECT;
    } else {
        status = ConfigNetSock(bus, AJ_Net_Recv, sizeof(rxData), AJ_Net_Send, sizeof(txData));
        if (status == AJ_OK) {
            network_up = TRUE;
            netContext.tcpSock = tcpSock;
        } else {
            qcom_socket_close(tcpSock);
            return AJ_ERR_CONNECT;
        }
    }

    /*
     *  linger behavior
     */
    ling.l_onoff = 1;
    ling.l_linger = 0;
    ret = qcom_setsockopt(tcpSock, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));
    if (ret < 0) {
        AJ_WarnPrintf(("AJ_Net_Connect(): qcom_setsockopt(%d) failed %d\n", tcpSock, ret));
    }

    return status;
}

void AJ_Net_Disconnect(AJ_NetSocket* netSock)
{
    CloseNetSock(netSock);
}

static AJ_Status RecvFrom(AJ_IOBuffer* rxBuf, uint32_t len, uint32_t timeout)
{
    q_fd_set recvfromFds;
    AJ_Status status = AJ_OK;
    MCastContext* context = (MCastContext*)rxBuf->context;

    int rc = 0;
    size_t rx = AJ_IO_BUF_SPACE(rxBuf);
    struct timeval tv = { timeout / 1000, 1000 * (timeout % 1000) };
    int maxFd = INVALID_SOCKET;


    AJ_ASSERT(rxBuf->direction == AJ_IO_BUF_RX);
    AJ_ASSERT(context->mDnsRecvSock != INVALID_SOCKET);

    FD_ZERO(&recvfromFds);

    if (context->udpSock != INVALID_SOCKET) {
        FD_SET(context->udpSock, &recvfromFds);
        maxFd = context->udpSock;
    }

    if (context->udp6Sock != INVALID_SOCKET) {
        FD_SET(context->udp6Sock, &recvfromFds);
        if (context->udp6Sock > maxFd) {
            maxFd = context->udp6Sock;
        }
    }

    if (context->mDnsRecvSock != INVALID_SOCKET) {
        FD_SET(context->mDnsRecvSock, &recvfromFds);
        if (context->mDnsRecvSock > maxFd) {
            maxFd = context->mDnsRecvSock;
        }
    }

    if (maxFd == INVALID_SOCKET) {
        AJ_ErrPrintf(("RecvFrom: no valid sockets\n"));
        return AJ_ERR_READ;
    }

    if (network_up) {
        rc = qcom_select(maxFd + 1, &recvfromFds, NULL, NULL, &tv);
    }
    if (rc == 0) {
        if (!network_up) {
            return AJ_ERR_READ;
        }
        AJ_InfoPrintf(("RecvFrom(): select() timed out. status=AJ_ERR_TIMEOUT\n"));
        return AJ_ERR_TIMEOUT;
    } else if (rc < 0) {
        AJ_ErrPrintf(("RecvFrom: select returned error AJ_ERR_READ\n"));
        return AJ_ERR_READ;
    }

    /* Read mDns first, as it is the preferred result to return */
    rx = AJ_IO_BUF_SPACE(rxBuf);
    if (context->mDnsRecvSock != INVALID_SOCKET && FD_ISSET(context->mDnsRecvSock, &recvfromFds)) {
        if (SOCK_IS_CLOSED(context->mDnsRecvSock, &recvfromFds)) {
            AJ_ErrPrintf(("RecvFrom(): Socket has been closed, returning AJ_ERR_READ\n"));
            return AJ_ERR_READ;
        }
        rx = min(rx, len);
        if (rx) {
            struct sockaddr_in fromAddr;
            int fromAddrSize  = sizeof(fromAddr);
            memset(&fromAddr, 0, sizeof (fromAddr));
            rc = qcom_recvfrom(context->mDnsRecvSock, (char*) rxBuf->writePtr, rx, 0, (struct sockaddr*)&fromAddr, &fromAddrSize);
            if (rc <= 0) {
                AJ_ErrPrintf(("RecvFrom(): qcom_recvfrom() failed, status=AJ_ERR_READ\n"));
                status = AJ_ERR_READ;
            } else {
                AJ_InfoPrintf(("RecvFrom(): *mDNS*\n"));
                rxBuf->flags |= AJ_IO_BUF_MDNS;
                rxBuf->writePtr += rc;
                return AJ_OK;
            }
        }
    }

    rx = AJ_IO_BUF_SPACE(rxBuf);
    if (context->udp6Sock != INVALID_SOCKET && FD_ISSET(context->udp6Sock, &recvfromFds)) {
        if (SOCK_IS_CLOSED(context->udp6Sock, &recvfromFds)) {
            AJ_ErrPrintf(("RecvFrom(): Socket has been closed, returning AJ_ERR_READ\n"));
            return AJ_ERR_READ;
        }
        rx = min(rx, len);
        if (rx) {
            struct sockaddr_in6 fromAddr6;
            int fromAddrSize  = sizeof(fromAddr6);
            memset(&fromAddr6, 0, sizeof(fromAddr6));
            rc = qcom_recvfrom(context->udp6Sock, (char*) rxBuf->writePtr, rx, 0, (struct sockaddr*)&fromAddr6, &fromAddrSize);
            if (rc <= 0) {
                AJ_ErrPrintf(("RecvFrom(): qcom_recvfrom() failed, status=AJ_ERR_READ\n"));
                status = AJ_ERR_READ;
            } else {
                AJ_InfoPrintf(("RecvFrom(): *Legacy-6*\n"));
                rxBuf->flags |= AJ_IO_BUF_AJ;
                rxBuf->writePtr += rc;
                return AJ_OK;
            }
        }
    }

    rx = AJ_IO_BUF_SPACE(rxBuf);
    if (context->udpSock != INVALID_SOCKET && FD_ISSET(context->udpSock, &recvfromFds)) {
        if (SOCK_IS_CLOSED(context->udpSock, &recvfromFds)) {
            AJ_ErrPrintf(("RecvFrom(): Socket has been closed, returning AJ_ERR_READ\n"));
            return AJ_ERR_READ;
        }
        rx = min(rx, len);
        if (rx) {
            struct sockaddr_in fromAddr;
            int fromAddrSize  = sizeof(fromAddr);
            memset(&fromAddr, 0, sizeof(fromAddr));
            rc = qcom_recvfrom(context->udpSock, (char*) rxBuf->writePtr, rx, 0, (struct sockaddr*)&fromAddr, &fromAddrSize);
            if (rc <= 0) {
                AJ_ErrPrintf(("RecvFrom(): qcom_recvfrom() failed, status=AJ_ERR_READ\n"));
                status = AJ_ERR_READ;
            } else {
                AJ_InfoPrintf(("RecvFrom(): *Legacy-4*\n"));
                rxBuf->flags |= AJ_IO_BUF_AJ;
                rxBuf->writePtr += rc;
                return AJ_OK;
            }
        }
    }

    return status;
}

static AJ_Status RewriteSenderInfo(AJ_IOBuffer* buf, uint32_t addr, uint16_t port)
{
    uint16_t sidVal;
    const char send[4] = { 'd', 'n', 'e', 's' };
    const char sid[] = { 's', 'i', 'd', '=' };
    const char ipv4[] = { 'i', 'p', 'v', '4', '=' };
    const char upcv4[] = { 'u', 'p', 'c', 'v', '4', '=' };
    char sidStr[6];
    char ipv4Str[17];
    char upcv4Str[6];
    uint8_t* pkt;
    uint16_t dataLength;
    int match;
    AJ_Status status;

    // first, pluck the search ID from the mDNS header
    sidVal = *(buf->readPtr) << 8;
    sidVal += *(buf->readPtr + 1);

    // convert to strings
    status = AJ_IntToString((int32_t) sidVal, sidStr, sizeof(sidStr));
    if (status != AJ_OK) {
        return AJ_ERR_WRITE;
    }
    status = AJ_IntToString((int32_t) port, upcv4Str, sizeof(upcv4Str));
    if (status != AJ_OK) {
        return AJ_ERR_WRITE;
    }
    status = AJ_InetToString(addr, ipv4Str, sizeof(ipv4Str));
    if (status != AJ_OK) {
        return AJ_ERR_WRITE;
    }

    // ASSUMPTIONS: sender-info resource record is the final resource record in the packet.
    // sid, ipv4, and upcv4 key value pairs are the final three key/value pairs in the record.
    // The length of the other fields in the record are static.
    //
    // search backwards through packet to find the start of "sender-info"
    pkt = buf->writePtr;
    match = 0;
    do {
        if (*(pkt--) == send[match]) {
            match++;
        } else {
            match = 0;
        }
    } while (pkt != buf->readPtr && match != 4);
    if (match != 4) {
        return AJ_ERR_WRITE;
    }

    // move forward to the Data Length field
    pkt += 22;

    // actual data length is the length of the static values already in the buffer plus
    // the three dynamic key-value pairs to re-write
    dataLength = 23 + 1 + sizeof(sid) + strlen(sidStr) + 1 + sizeof(ipv4) + strlen(ipv4Str) + 1 + sizeof(upcv4) + strlen(upcv4Str);
    *pkt++ = (dataLength >> 8) & 0xFF;
    *pkt++ = dataLength & 0xFF;

    // move forward past the static key-value pairs
    pkt += 23;

    // ASSERT: must be at the start of "sid="
    AJ_ASSERT(*(pkt + 1) == 's');

    // re-write new values
    *pkt++ = sizeof(sid) + strlen(sidStr);
    memcpy(pkt, sid, sizeof(sid));
    pkt += sizeof(sid);
    memcpy(pkt, sidStr, strlen(sidStr));
    pkt += strlen(sidStr);

    *pkt++ = sizeof(ipv4) + strlen(ipv4Str);
    memcpy(pkt, ipv4, sizeof(ipv4));
    pkt += sizeof(ipv4);
    memcpy(pkt, ipv4Str, strlen(ipv4Str));
    pkt += strlen(ipv4Str);

    *pkt++ = sizeof(upcv4) + strlen(upcv4Str);
    memcpy(pkt, upcv4, sizeof(upcv4));
    pkt += sizeof(upcv4);
    memcpy(pkt, upcv4Str, strlen(upcv4Str));
    pkt += strlen(upcv4Str);

    buf->writePtr = pkt;

    return AJ_OK;
}

static AJ_Status SendTo(AJ_IOBuffer* txBuf)
{
    MCastContext* context = (MCastContext*)txBuf->context;
    int ret;
    int tx = AJ_IO_BUF_AVAIL(txBuf);
    AJ_ASSERT(txBuf->direction == AJ_IO_BUF_TX);
    uint8_t sendSucceeded = FALSE;

    if (tx > 0) {
        if (txBuf->flags & AJ_IO_BUF_AJ) {
            /*
             * Multicast send over IPv4 to AJ Group and Port
             */
            if (context->udpSock != INVALID_SOCKET) {
                struct sockaddr_in sin;
                memset(&sin, 0, sizeof(sin));
                sin.sin_family = AF_INET;
                sin.sin_port = htons(AJ_UDP_PORT);
                sin.sin_addr.s_addr = htonl(AJ_IPV4_MCAST_GROUP);

                ret = qcom_sendto(context->udpSock, (char*) txBuf->bufStart, tx, 0, (struct sockaddr*) &sin, sizeof(sin));
                if (tx == ret) {
                    sendSucceeded = TRUE;
                } else {
                    AJ_ErrPrintf(("SendTo() AJ mcast failed %d\n", ret));
                }


                /*
                 * Broadcast send over IPv4 to AJ Port
                 */
                // the driver does not support subnet bcast so send to 255.255.255.255
                sin.sin_addr.s_addr = 0xFFFFFFFF;
                ret = qcom_sendto(context->udpSock, (char*) txBuf->bufStart, tx, 0, (struct sockaddr*) &sin, sizeof(sin));
                if (tx == ret) {
                    sendSucceeded = TRUE;
                } else {
                    AJ_ErrPrintf(("SendTo() AJ bcast failed: %d\n", ret));
                }
            }

            /*
             * Multicast send over IPv6 to AJ Group and Port
             */

            if (context->udp6Sock != INVALID_SOCKET) {
                struct sockaddr_in6 sin6;
                memset(&sin6, 0, sizeof(sin6));
                sin6.sin6_family = AF_INET6;
                sin6.sin6_port = htons(AJ_UDP_PORT);
                memcpy(&sin6.sin6_addr, AJ_IPV6_MCAST_GROUP, sizeof(AJ_IPV6_MCAST_GROUP));
                ret = qcom_sendto(context->udp6Sock, (char*) txBuf->bufStart, tx, 0, (struct sockaddr*) &sin6, sizeof(sin6));
                if (tx == ret) {
                    sendSucceeded = TRUE;
                } else {
                    AJ_ErrPrintf(("SendTo() AJ mcast6 failed: %d\n", ret));
                }
            }
        }

        if (txBuf->flags & AJ_IO_BUF_MDNS) {
            if (RewriteSenderInfo(txBuf, context->mDnsRecvAddr, context->mDnsRecvPort) != AJ_OK) {
                AJ_ErrPrintf(("SendTo(): RewriteSenderInfo failed.\n"));
                goto rewrite_failed;
            }
            tx = AJ_IO_BUF_AVAIL(txBuf);

            /*
             * Broadcast/Multicast send over IPv4 to mDNS Group and Port
             */
            if (context->mDnsSock != INVALID_SOCKET) {
                struct sockaddr_in sin;
                memset(&sin, 0, sizeof(sin));
                sin.sin_family = AF_INET;
                sin.sin_port = htons(MDNS_UDP_PORT);
                // the driver does not support subnet bcast so send to 255.255.255.255
                sin.sin_addr.s_addr = 0xFFFFFFFF;
                ret = qcom_sendto(context->mDnsSock, txBuf->bufStart, tx, 0, (struct sockaddr*) &sin, sizeof(sin));
                if (tx == ret) {
                    sendSucceeded = TRUE;
                } else {
                    AJ_ErrPrintf(("SendTo() mDNS bcast failed: %d\n", ret));
                }

                /*
                 * inhibit Multicast while in softAP mode
                 */
                if (!context->softAP) {
                    /*
                     * Multicast send over IPv4 to mDNS Port
                     */
                    sin.sin_addr.s_addr = htonl(MDNS_IPV4_MCAST_GROUP);
                    ret = qcom_sendto(context->mDnsSock, (char*) txBuf->bufStart, tx, 0, (struct sockaddr*) &sin, sizeof(sin));
                    if (tx == ret) {
                        sendSucceeded = TRUE;
                    } else {
                        AJ_ErrPrintf(("SendTo() mDNS mcast failed: %d\n", ret));
                    }
                }
            }

            /*
             * Multicast send over IPv6 to mDNS Group and Port
             */
            if (!context->softAP && context->mDns6Sock != INVALID_SOCKET) {
                struct sockaddr_in6 sin6;
                memset(&sin6, 0, sizeof(sin6));
                sin6.sin6_family = AF_INET6;
                sin6.sin6_port = htons(MDNS_UDP_PORT);
                memcpy(&sin6.sin6_addr, MDNS_IPV6_MCAST_GROUP, sizeof(MDNS_IPV6_MCAST_GROUP));
                ret = qcom_sendto(context->mDns6Sock, (char*) txBuf->bufStart, tx, 0, (struct sockaddr*) &sin6, sizeof(sin6));
                if (tx == ret) {
                    sendSucceeded = TRUE;
                } else {
                    AJ_ErrPrintf(("SendTo() mDNS mcast6 failed: %d\n", ret));
                }
            }
        }

rewrite_failed:
        if (!sendSucceeded) {
            AJ_ErrPrintf(("SendTo() failed\n"));
            return AJ_ERR_WRITE;
        }
    }
    AJ_IO_BUF_RESET(txBuf);
    AJ_InfoPrintf(("SendTo(): status=AJ_OK\n"));
    return AJ_OK;
}

static int MCastUp4(uint32_t group, uint16_t port)
{
    int mcastSock;

    mcastSock = qcom_socket(PF_INET, SOCK_DGRAM, 0);
    if (mcastSock == INVALID_SOCKET) {
        AJ_ErrPrintf(("MCastUp4: problem creating socket\n"));
        return INVALID_SOCKET;
    }

    /*
     * Only the Well-Known AJ UDP port is for both Multicast Rx and Tx
     */
    if (port == AJ_UDP_PORT) {
        int ret;
        struct sockaddr_in sin;
        struct ip_mreq {
            uint32_t imr_multiaddr;   /* IP multicast address of group */
            uint32_t imr_interface;   /* local IP address of interface */
        } mreq;

        /*
         * Bind multicast port
         */
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);
        sin.sin_addr.s_addr = INADDR_ANY;
        ret = qcom_bind(mcastSock, (struct sockaddr*) &sin, sizeof(sin));
        if (ret < 0) {
            qcom_socket_close(mcastSock);
            return INVALID_SOCKET;
        }


        /*
         * Join the AllJoyn IPv4 multicast group
         */
        mreq.imr_multiaddr = htonl(group);
        mreq.imr_interface = INADDR_ANY;
        ret = qcom_setsockopt(mcastSock, SOL_SOCKET, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
        if (ret < 0) {
            AJ_WarnPrintf(("MCastUp4(): qcom_setsockopt(%d) failed\n", ret));
            qcom_socket_close(mcastSock);
            return INVALID_SOCKET;
        }
    }

    return mcastSock;
}


static int MCastUp6(const uint8_t group[], uint16_t port)
{
    int sock;

    /*
     * Create the IPv6 socket
     */
    sock = qcom_socket(PF_INET6, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        AJ_ErrPrintf(("MCastUp6(): qcom_socket() failed\n"));
        return INVALID_SOCKET;
    }

    /*
     * Only the Well-Known AJ UDP port is for both Multicast Rx and Tx
     */
    if (port == AJ_UDP_PORT) {
        int ret;
        struct ip_mreq {
            uint8_t ipv6mr_multiaddr[16];   /* IP multicast address of group */
            uint8_t ipv6mr_interface[16];   /* local IP address of interface */
        } group6;
        struct sockaddr_in6 sin6;
        uint8_t gblAddr[16];
        uint8_t locAddr[16];
        uint8_t gwAddr[16];
        uint8_t gblExtAddr[16];
        uint32_t linkPrefix;
        uint32_t glbPrefix;
        uint32_t gwPrefix;
        uint32_t glbExtPrefix;

        /*
         * Get the local IPv6 address.
         */
        ret = qcom_ip6_address_get(qcom_DeviceId, gblAddr, locAddr, gwAddr, gblExtAddr, &linkPrefix, &glbPrefix, &gwPrefix, &glbExtPrefix);
        if (ret != A_OK) {
            AJ_ErrPrintf(("MCastUp6(): qcom_ip6_address_get() failed: %d\n", ret));
            qcom_socket_close(sock);
            return INVALID_SOCKET;
        }

        /*
         * Bind to the supplied port
         */
        memset(&sin6, 0, sizeof(sin6));
        sin6.sin6_family = AF_INET6;
        sin6.sin6_port = htons(port);
        ret = qcom_bind(sock,  (struct sockaddr*) &sin6, sizeof(sin6));
        if (ret < 0 ) {
            AJ_ErrPrintf(("MCastUp6(): qcom_bind(%d) failed %d\n", sock, ret));
            qcom_socket_close(sock);
            return INVALID_SOCKET;
        }

        /*
         * Join the AllJoyn IPv6 multicast group
         */
        memset(&group6, 0, sizeof(group6));
        memcpy(&group6.ipv6mr_multiaddr, group, sizeof(AJ_IPV6_MCAST_GROUP));
        memcpy(&group6.ipv6mr_interface, locAddr, sizeof(locAddr));

        ret = qcom_setsockopt(sock, SOL_SOCKET, IPV6_JOIN_GROUP, (char*)&group6, sizeof(group6));
        if (ret < 0) {
            AJ_ErrPrintf(("MCastUp6(): qcom_setsockopt(%d) failed %d\n", sock, ret));
            qcom_socket_close(sock);
            return INVALID_SOCKET;
        }
    }
    return sock;
}

static int MDnsRecvUp(uint16_t* port)
{
    uint16_t p;
    int ret;
    int mDnsRecvSock;
    struct sockaddr_in sin;

    /*
     * Open socket
     */
    mDnsRecvSock = qcom_socket(PF_INET, SOCK_DGRAM, 0);
    if (mDnsRecvSock == INVALID_SOCKET) {
        AJ_ErrPrintf(("MDnsRecvUp(): t_socket() failed\n"));
        return INVALID_SOCKET;
    }

    /*
     * bind the socket to the address with ephemeral port
     */
    p = AJ_EphemeralPort();
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(p);
    sin.sin_addr.s_addr = INADDR_ANY;
    ret = qcom_bind(mDnsRecvSock, (struct sockaddr*) &sin, sizeof(sin));
    if (ret < 0) {
        AJ_ErrPrintf(("MDnsRecvUp(): t_bind(%d) failed\n", mDnsRecvSock));
        qcom_socket_close(mDnsRecvSock);
        return INVALID_SOCKET;
    }

    *port = p;
    network_up = TRUE;
    return mDnsRecvSock;
}

AJ_Status AJ_Net_MCastUp(AJ_MCastSocket* mcastSock)
{
    AJ_Status status = AJ_ERR_READ;
    uint32_t ip;
    uint32_t mask;
    uint32_t gateway;
    uint16_t port;

    memset(mcastSock, 0, sizeof(AJ_MCastSocket));
    mCastContext = mCastContextInit;

    status = AJ_GetIPAddress(&ip, &mask, &gateway);
    if (status != AJ_OK) {
        AJ_ErrPrintf(("AJ_Net_MCastUp(): no IP address\n"));
        return status;
    }

    mCastContext.mDnsRecvAddr = ip;
    if (mCastContext.mDnsRecvAddr == 0) {
        AJ_ErrPrintf(("AJ_Net_MCastUp(): no mDNS recv address\n"));
        return AJ_ERR_READ;
    } else if (!gateway) {
        /* We are a SoftAP. Only Broadcasts Queries to IPv4  */
        AJ_InfoPrintf(("SoftAP Detected\n"));
        mCastContext.softAP = 1;
    }

    mCastContext.mDnsRecvSock = MDnsRecvUp(&port);
    if (mCastContext.mDnsRecvSock == INVALID_SOCKET) {
        AJ_ErrPrintf(("AJ_Net_MCastUp(): MDnsRecvUp for mDnsRecvPort failed"));
        return AJ_ERR_READ;
    }
    mCastContext.mDnsRecvPort = port;
    status = AJ_ERR_READ;

    AJ_InfoPrintf(("AJ_Net_MCastUp(): mDNS recv on %d.%d.%d.%d:%d\n",
                            ((ip >> 24) & 0xFF), ((ip >> 16) & 0xFF),
                            ((ip >> 8) & 0xFF), (ip & 0xFF), port));

    mCastContext.mDnsSock = MCastUp4(MDNS_IPV4_MCAST_GROUP, MDNS_UDP_PORT);
    mCastContext.mDns6Sock = MCastUp6(MDNS_IPV6_MCAST_GROUP, MDNS_UDP_PORT);

    if (AJ_GetMinProtoVersion() < 10) {
        mCastContext.udpSock = MCastUp4(AJ_IPV4_MCAST_GROUP, AJ_UDP_PORT);
        mCastContext.udp6Sock = MCastUp6(AJ_IPV6_MCAST_GROUP, AJ_UDP_PORT);
    }

    if (mCastContext.udpSock != INVALID_SOCKET || mCastContext.udp6Sock != INVALID_SOCKET ||
        mCastContext.mDnsSock != INVALID_SOCKET || mCastContext.mDns6Sock != INVALID_SOCKET) {

        status = ConfigMCastSock(mcastSock, RecvFrom, 1454, SendTo, 471);
        if (status == AJ_OK) {
            return AJ_OK;
        }
    }

    AJ_ErrPrintf(("AJ_Net_MCastUp(): Socket setup(s) failed"));
    CloseMCastSock(mcastSock);
    return status;
}

void AJ_Net_MCastDown(AJ_MCastSocket* mcastSock)
{
    CloseMCastSock(mcastSock);
}
