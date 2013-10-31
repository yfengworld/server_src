#include "msg_protobuf.h"
#include "msg.h"

int create_msg(uint16_t cmd, uint64_t uid, unsigned char **msg, size_t *sz)
{
    *sz = MSG_HEAD_SIZE + sizeof(uint64_t);
    *msg = (unsigned char *)malloc(*sz);
    if (NULL == *msg)
        return -1;

    unsigned short *cur = (unsigned short *)*msg;
    *cur++ = htons((unsigned short)MAGIC_NUMBER);
    *cur++ = htons((unsigned short)sizeof(uint64_t));
    *cur++ = htons((unsigned short)cmd);
    *cur++ = htons((unsigned short)FLAG_HAS_UID);
    *((uint64_t *)cur) = htons((uint64_t)uid);
    return 0;
}

int create_msg(uint16_t cmd, unsigned char **msg, size_t *sz)
{
    *sz = MSG_HEAD_SIZE;
    *msg = (unsigned char *)malloc(*sz);
    if (NULL == *msg) {
        return -1;
    }

    unsigned short *cur = (unsigned short *)*msg;
    *cur++ = htons((unsigned short)MAGIC_NUMBER);
    *cur++ = htons((unsigned short)0);
    *cur++ = htons((unsigned short)cmd);
    *cur++ = htons((unsigned short)0);
    return 0;
}

int message_head(unsigned char *src, size_t src_sz, msg_head *h)
{
    if (MSG_HEAD_SIZE > src_sz) {
        merror("msg less than head size!");
        return -1;
    }

    unsigned short *cur = (unsigned short *)src;
    h->magic = ntohs(*cur++);
    h->len = ntohs(*cur++);
    h->cmd = ntohs(*cur++);
    h->flags = ntohs(*cur);
    return 0;
}

int conn_write(conn *c, unsigned short cmd)
{
    unsigned char *msg;
    size_t sz;
    int ret = create_msg(cmd, &msg, &sz);
    if (0 != ret)
        return ret;
    ret = conn_write(c, msg, sz);
    free(msg);
    return ret;
}

int conn_write(conn *c, unsigned short cmd, uint64_t uid)
{
    unsigned char *msg;
    size_t sz;
    int ret = create_msg(cmd, uid, &msg, &sz);
    if (0 != ret)
        return ret;
    ret = conn_write(c, msg, sz);
    free(msg);
    return ret;
}

int connector_write(connector *cr, unsigned short cmd)
{
    unsigned char *msg;
    size_t sz;
    int ret = create_msg(cmd, &msg, &sz);
    if (ret != 0)
        return ret;
    ret = connector_write(cr, msg, sz);
    free(msg);
    return ret;
}

int connector_write(connector *cr, unsigned short cmd, uint64_t uid)
{
    unsigned char *msg;
    size_t sz;
    int ret = create_msg(cmd, uid,  &msg, &sz);
    if (ret != 0)
        return ret;
    ret = connector_write(cr, msg, sz);
    free(msg);
    return ret;
}
