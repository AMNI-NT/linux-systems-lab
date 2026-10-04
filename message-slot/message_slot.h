#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>

#define MAJOR_NUM 235
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)
#define MSG_SLOT_SET_CEN _IOW(MAJOR_NUM, 1, unsigned int)

#define DEVICE_RANGE_NAME "message_slot"
#define DEVICE_FILE_NAME "message_slot_file"
#define SUCCESS 0
#define FAILURE -1
#define BUF_LEN 128

typedef struct channel{
    char message[BUF_LEN];
    unsigned long channel_id;
    int message_length;
    struct channel* next;
} channel;

typedef struct device_msg{
    unsigned int minor_number;
    struct channel* channels;
    struct device_msg* next;
} device_msg;


typedef struct private_data_{
    int censorship_mode;
    unsigned int minor_number;
    unsigned long channel_id;
} pd;

#endif