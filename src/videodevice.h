#ifndef VIDEODEVICE_H
#define VIDEODEVICE_H
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <asm/types.h>
#include <linux/videodev2.h>
//#include <QString>
//#include <QObject>
 
#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define WIDTH  1280
#define HEIGHT 480
 
class VideoDevice
{
 
public:
    VideoDevice(char* dev_name);
    int open_device();
    int close_device();
    int init_device();
    int start_capturing();
    int stop_capturing();
    int uninit_device();
    int get_frame(void **frame_buf, size_t* len, long * frame_count);
    int unget_frame();
 
private:
    int init_mmap();
 
    struct buffer
    {
 
        void * start;
        size_t length;
     
    };
    char dev_name[64];
    int fd;
    buffer* buffers;
    unsigned int n_buffers;
    int index;

 
 
};
 
#endif // VIDEODEVICE_H
