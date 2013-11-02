#ifndef MSG_PROTOBUF_H_INCLUDED
#define MSG_PROTOBUF_H_INCLUDED

#include "log.h"
#include "msg.h"
#include "net.h"

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stddef.h>

#define UID_SIZE 8

int create_msg(uint16_t cmd, uint64_t uid, unsigned char **msg, size_t *sz);
int create_msg(uint16_t cmd, unsigned char **msg, size_t *sz);
int message_head(unsigned char *src, size_t src_sz, msg_head *h);

template<typename S>
int create_msg(uint16_t cmd, uint64_t uid, S *s, unsigned char **msg, size_t *sz)
{
    size_t body_sz = s->ByteSize();
    *sz = MSG_HEAD_SIZE + UID_SIZE + body_sz;
    *msg = (unsigned char *)malloc(*sz);
    if (NULL == *msg) {
        merror("msg alloc failed!");
        return -1;
    }

    unsigned short *cur = (unsigned short *)*msg;
    *((unsigned int *)cur) = htons((unsigned int)(UID_SIZE + body_sz));
    cur += 2;
    *cur++ = htons((unsigned short)cmd);
    *cur++ = htons((unsigned short)FLAG_HAS_UID);
    *((uint64_t *)cur) = htons((uint64_t)uid);
    cur += 4;
    
    google::protobuf::io::ArrayOutputStream arr(cur, body_sz);
    google::protobuf::io::CodedOutputStream output(&arr);
    s->SerializeToCodedStream(&output);
    return 0;
}

template<typename S>
int create_msg(uint16_t cmd, S *s, unsigned char **msg, size_t *sz)
{
    size_t body_sz = s->ByteSize();
    *sz = MSG_HEAD_SIZE + body_sz;
    *msg = (unsigned char *)malloc(*sz);
    if (NULL == *msg) {
        merror("msg alloc failed!");
        return -1;
    }

    unsigned short *cur = (unsigned short *)*msg;
    *((unsigned int *)cur) = htons((unsigned short)body_sz);
    cur += 2;
    *cur++ = htons((unsigned short)cmd);
    *cur++ = htons((unsigned short)0);
    
    google::protobuf::io::ArrayOutputStream arr(cur, body_sz);
    google::protobuf::io::CodedOutputStream output(&arr);
    s->SerializeToCodedStream(&output);
    return 0;
}


template<typename S>
int msg_body(unsigned char *src, size_t src_sz, uint64_t *uid, S *s)
{
    if (MSG_HEAD_SIZE > src_sz) {
        merror("msg less than head size!");
        return -1;
    }

    unsigned short *cur = (unsigned short *)src;
    unsigned int len = ntohs(*((unsigned int *)cur)++);
    if (MSG_HEAD_SIZE + len != src_sz) {
        merror("msg length error!");
        return -1;
    }

    cur++;
    unsigned short flags = ntohs(*cur++);
    if (0 == flags & FLAG_HAS_UID) {
        merror(stderr, "msg no uid!");
        return -1;
    }

    google::protobuf::io::CodedInputStream input((const google::protobuf::uint8*)cur, len);
    s->ParseFromCodedStream(&input);
    return 0;
}

template<typename S>
int msg_body(unsigned char *src, size_t src_sz, S *s)
{
    if (MSG_HEAD_SIZE > src_sz) {
        merror("msg less than head size!");
        return -1;
    }
    unsigned short *cur = (unsigned short *)src;
    unsigned int len = ntohs(*((unsigned int *)cur));
    cur += 2;
    if (MSG_HEAD_SIZE + len != src_sz) {
        merror("msg length error!");
        return -1;
    }

    cur += 2;
    google::protobuf::io::CodedInputStream input((const google::protobuf::uint8*)cur, len);
    s->ParseFromCodedStream(&input);
    return 0;
}

template<typename S>
int conn_write(conn *c, unsigned short cmd, S *s)
{
    unsigned char *msg;
    size_t sz;
    int ret = create_msg(cmd, s, &msg, &sz);
    if (ret != 0)
        return ret;
    ret = conn_write(c, msg, sz);
    free(msg);
    return ret;
}

template<typename S>
int conn_write(conn *c, unsigned short cmd, uint64_t uid, S *s)
{
    unsigned char *msg;
    size_t sz;
    int ret = create_msg(cmd, uid, s, &msg, &sz);
    if (ret != 0)
        return ret;
    ret = conn_write(c, msg, sz);
    free(msg);
    return ret;
}

int conn_write(conn *c, unsigned short cmd);
int conn_write(conn *c, unsigned short cmd, uint64_t uid);

template<typename S>
int connector_write(connector *cr, unsigned short cmd, S *s)
{
    unsigned char *msg;
    size_t sz;
    int ret = create_msg<S>(cmd, s, &msg, &sz);
    if (ret != 0)
        return ret;
    ret = connector_write(cr, msg, sz);
    free(msg);
    return ret;
}

template<typename S>
int connector_write(connector *cr, unsigned short cmd, uint64_t uid, S *s)
{
    unsigned char *msg;
    size_t sz;
    int ret = create_msg(cmd, uid, s, &msg, &sz);
    if (ret != 0)
        return ret;
    ret = connector_write(cr, msg, sz);
    free(msg);
    return ret;
}

int connector_write(connector *cr, unsigned short cmd);
int connector_write(connector *cr, unsigned short cmd, uint64_t uid);

#endif /* MSG_PROTOBUF_H_INCLUDED */
