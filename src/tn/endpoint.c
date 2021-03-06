#include "tn/endpoint.h"

#include "uv.h"

#include "tn/error.h"

#include "aws/common/byte_buf.h"
#include "aws/common/hash_table.h"

// --------------------------------------------------------------------------------------------------------------
bool tn_endpoint_is_ipv4(const tn_endpoint_t *endpoint)
{
    TN_ASSERT(endpoint);
    return (endpoint->addr4.family == AF_INET);
}

// --------------------------------------------------------------------------------------------------------------
bool tn_endpoint_is_ipv6(const tn_endpoint_t *endpoint)
{
    TN_ASSERT(endpoint);
    return (endpoint->addr6.family == AF_INET6);
}

// --------------------------------------------------------------------------------------------------------------
uint16_t tn_endpoint_af_get(const tn_endpoint_t *endpoint)
{
    TN_ASSERT(endpoint);
    return endpoint->addr4.family;
}

// --------------------------------------------------------------------------------------------------------------
void tn_endpoint_af_set(tn_endpoint_t *endpoint, uint16_t af)
{
    TN_ASSERT(endpoint);
    endpoint->addr4.family = af;
}

// --------------------------------------------------------------------------------------------------------------
void tn_endpoint_port_set(tn_endpoint_t *endpoint, uint16_t port)
{
    TN_ASSERT(endpoint);
    endpoint->addr4.port = aws_hton16(port);
}

// --------------------------------------------------------------------------------------------------------------
uint16_t tn_endpoint_port_get(tn_endpoint_t *endpoint)
{
    TN_ASSERT(endpoint);
    return aws_ntoh16(endpoint->addr4.port);
}

// --------------------------------------------------------------------------------------------------------------
void tn_endpoint_from_short(tn_endpoint_t *endpoint, uint16_t port, uint16_t s0, uint16_t s1, uint16_t s2, uint16_t s3, uint16_t s4, uint16_t s5, uint16_t s6, uint16_t s7)
{
    TN_ASSERT(endpoint);
    tn_endpoint_af_set(endpoint, AF_INET6);
    tn_endpoint_port_set(endpoint, port);
    *(uint16_t *)&endpoint->addr6.addr[0] = aws_hton16(s0);
    *(uint16_t *)&endpoint->addr6.addr[2] = aws_hton16(s1);
    *(uint16_t *)&endpoint->addr6.addr[4] = aws_hton16(s2);
    *(uint16_t *)&endpoint->addr6.addr[6] = aws_hton16(s3);
    *(uint16_t *)&endpoint->addr6.addr[8] = aws_hton16(s4);
    *(uint16_t *)&endpoint->addr6.addr[10] = aws_hton16(s5);
    *(uint16_t *)&endpoint->addr6.addr[12] = aws_hton16(s6);
    *(uint16_t *)&endpoint->addr6.addr[14] = aws_hton16(s7);
}

// --------------------------------------------------------------------------------------------------------------
void tn_endpoint_from_byte(tn_endpoint_t *endpoint, uint16_t port, uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
    TN_ASSERT(endpoint);
    tn_endpoint_af_set(endpoint, AF_INET);
    tn_endpoint_port_set(endpoint, port);
    *((uint8_t *)&endpoint->addr4.addr + 0) = b0;
    *((uint8_t *)&endpoint->addr4.addr + 1) = b1;
    *((uint8_t *)&endpoint->addr4.addr + 2) = b2;
    *((uint8_t *)&endpoint->addr4.addr + 3) = b3;
}

// --------------------------------------------------------------------------------------------------------------
int tn_endpoint_string_get(tn_endpoint_t *endpoint, uint16_t *port, char *buf, int buf_len)
{
    TN_ASSERT(endpoint);
    TN_ASSERT(port);
    TN_ASSERT(buf);
    *port = 0;
    memset(buf, 0, buf_len);

    char ipbuf[255];
    memset(ipbuf, 0, 255);

    struct sockaddr_in6 *sa = (struct sockaddr_in6 *)endpoint;
    if (tn_endpoint_is_ipv6(endpoint)) {
        uv_ip6_name(sa, ipbuf, 255);
        *port = ntohs(sa->sin6_port);
    } else if (tn_endpoint_is_ipv4(endpoint)) {
        struct sockaddr_in *sa = (struct sockaddr_in *)endpoint;
        uv_ip4_name(sa, ipbuf, 255);
        *port = ntohs(sa->sin_port);
    } else {
        return TN_ERROR;
    }

    sprintf(buf, "%s", ipbuf);
    return TN_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------
int tn_endpoint_from_string(tn_endpoint_t *endpoint, const char *ip, uint16_t port)
{
    TN_ASSERT(endpoint);
    TN_ASSERT(ip);
    if (tn_endpoint_set_ip6(endpoint, ip, port)) {
        return tn_endpoint_set_ip4(endpoint, ip, port);
    }

    return TN_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------
int tn_endpoint_set_ip4(tn_endpoint_t *endpoint, const char *ip, uint16_t port)
{
    TN_ASSERT(endpoint);
    TN_ASSERT(ip);

    int ret;

    if (!endpoint) return TN_ERROR;
    if (!ip) return TN_ERROR;

    memset(endpoint, 0, sizeof(*endpoint));
    if ((ret = uv_ip4_addr(ip, port, (struct sockaddr_in *)endpoint))) return TN_ERROR;

    return TN_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------
int tn_endpoint_set_ip6(tn_endpoint_t *endpoint, const char *ip, uint16_t port)
{
    TN_ASSERT(endpoint);
    TN_ASSERT(ip);

    int ret;

    if (!endpoint) return TN_ERROR;
    if (!ip) return TN_ERROR;

    memset(endpoint, 0, sizeof(*endpoint));
    if ((ret = uv_ip6_addr(ip, port, (struct sockaddr_in6 *)endpoint))) return TN_ERROR;

    return TN_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------
int tn_endpoint_convert_from(tn_endpoint_t *endpoint, void *sockaddr_void)
{
    if (!endpoint) return TN_ERROR;
    if (!sockaddr_void) return TN_ERROR;

    struct sockaddr_storage *sockaddr = sockaddr_void;

    memset(endpoint, 0, sizeof(*endpoint));
    if (sockaddr->ss_family == AF_INET6) {
        memcpy(endpoint, (struct sockaddr_in6 *)sockaddr, sizeof(struct sockaddr_in6));
    } else if (sockaddr->ss_family == AF_INET) {
        memcpy(endpoint, (struct sockaddr_in *)sockaddr, sizeof(struct sockaddr_in));
    } else {
        return TN_ERROR;
    }

    return TN_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------
bool tn_endpoint_equal_addr(tn_endpoint_t *endpoint, void *sockaddr)
{
    tn_endpoint_t *addr = (tn_endpoint_t *)sockaddr;
    if (tn_endpoint_af_get(addr) != tn_endpoint_af_get(endpoint)) return false;

    if (tn_endpoint_is_ipv4(addr)) {
        struct sockaddr_in6 *ep_addr6 = (struct sockaddr_in6 *)endpoint;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        if (memcmp(endpoint, addr, sizeof(struct sockaddr_in6))) return false;
        return true;
    } else if (tn_endpoint_is_ipv6(addr)) {
        tn_sockaddr4_t *ep_addr4 = (tn_sockaddr4_t *)endpoint;
        tn_sockaddr4_t *addr4 = (tn_sockaddr4_t *)addr;
        if (addr4->addr != ep_addr4->addr) return false;
        if (addr4->port != ep_addr4->port) return false;
        return true;
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------
int tn_endpoint_get_hash(tn_endpoint_t *endpoint, uint64_t *out_hash)
{
    TN_ASSERT(out_hash);

    *out_hash = 0;
    if (endpoint->addr6.family == AF_INET6) {
        struct aws_byte_cursor bc = aws_byte_cursor_from_array(endpoint, sizeof(*endpoint));
        *out_hash = aws_hash_byte_cursor_ptr(&bc);
        return TN_SUCCESS;
    } else if (endpoint->addr4.family == AF_INET) {
        *out_hash = 0x0000000000000000ULL & endpoint->addr4.port;
        *out_hash <<= 32;
        *out_hash |= endpoint->addr4.addr;
        return TN_SUCCESS;
    }

    return TN_ERROR;
}
