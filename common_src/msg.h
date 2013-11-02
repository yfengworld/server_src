#ifndef MSG_H_INCLUDED
#define MSG_H_INCLUDED

typedef struct msg_head msg_head;
struct msg_head
{
    unsigned int len;
    unsigned short cmd;
#define FLAG_HAS_UID 1
    unsigned short flags;
};

#define MSG_HEAD_SIZE 8
#define MSG_MAX_SIZE  1 * 1024 * 1024

#endif /* MSG_H_INCLUDED */
