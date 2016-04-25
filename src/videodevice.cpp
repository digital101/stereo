#include "videodevice.h"
#include <iostream>

using namespace std;

VideoDevice::VideoDevice(char* dev_name)
{
 
    strcpy(this->dev_name, dev_name);
    this->fd = -1;
    this->buffers = NULL;
    this->n_buffers = 0;
    this->index = -1;
}
 
int VideoDevice::open_device()
{

    cout << "opening device:  " << this->dev_name << endl ;
    fd = open(this->dev_name, O_RDWR, 0);
 
 
    if(-1 == fd)
    {
        cout << "failed to open device" << endl;
        return -1;
     
    }
    return 0;
 
}
 
int VideoDevice::close_device()
{
 
//    if(-1 == close(fd))
//    {
//
//        emit display_error(tr("close: %1").arg(QString(strerror(errno))));
//        return -1;
//
//    }
    return 0;
 
}
 
int VideoDevice::init_device()
{
 
    v4l2_capability cap;
    v4l2_cropcap cropcap;
    v4l2_crop crop;
    v4l2_format fmt;
    unsigned int min;

    if(-1 == ioctl(fd, VIDIOC_QUERYCAP, &cap))
    {
 
        if(EINVAL == errno)
        {
 
            std::cout << " no V4l2 device %d" << strerror(errno) << endl;
         
        }
        else
        {
 
        	std::cout << "VIDIOC_QUERYCAP: %1" << strerror(errno);
         
        }
        return -1;
     
    }
 
    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
 
        std::cout << "%1 is no video capture device" << dev_name << endl;
        return -1;
     
    }
 
    if(!(cap.capabilities & V4L2_CAP_STREAMING))
    {
 
        cout << "%1 does not support streaming i/o" << dev_name << endl;
        return -1;
     
    }
 
    CLEAR(cropcap);
 
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 
//    if(0 == ioctl(fd, VIDIOC_CROPCAP, &cropcap))
//    {
//
//        CLEAR(crop);
//        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//        crop.c = cropcap.defrect;
//
//        if(-1 == ioctl(fd, VIDIOC_S_CROP, &crop))
//        {
//
//            if(EINVAL == errno)
//            {
 
//neverin!!                emit display_error(tr("VIDIOC_S_CROP not supported"));
             
//            }
//            else
//            {
//
//                emit display_error(tr("VIDIOC_S_CROP: %1").arg(QString(strerror(errno))));
//                return -1;
//
//            }
         
//         }
     
//    }
//    else
//    {
 
//        emit display_error(tr("VIDIOC_CROPCAP: %1").arg(QString(strerror(errno))));
//        return -1;
     
//    }
 
    CLEAR(fmt);
 
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
 
    if(-1 == ioctl(fd, VIDIOC_S_FMT, &fmt))
    {
        cout << "VIDIOC_S_FMT %l" << strerror(errno) << endl;
 
        return -1;
     
    }
 
    if(-1 == init_mmap())
    {
        cout << "failed init mmap %ld" << strerror(errno) << endl;
 
        return -1;
     
    }
    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
            fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
            fmt.fmt.pix.sizeimage = min;

    return 0;
 
}
 
int VideoDevice::init_mmap()
{
 
    v4l2_requestbuffers req;
    CLEAR(req);
 
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

 
    if(-1 == ioctl(fd, VIDIOC_REQBUFS, &req))
    {
 
        if(EINVAL == errno)
        {
 
            return -1;
         
        }
        else
        {
 
            return -1;
         
        }
     
    }
 
    if(req.count < 2)
    {
        cout << "failed req.count < 2 %ld" << strerror(errno) << endl;
 
        return -1;
     
    }
 
    buffers = (buffer*)calloc(req.count, sizeof(*buffers));
 
    if(!buffers)
    {
        cout << "failed no buffers %ld" << strerror(errno) << endl;
 
        return -1;
     
    }
 
    for(n_buffers = 0; n_buffers < req.count; ++n_buffers)
    {
 
        v4l2_buffer buf;
        CLEAR(buf);
 
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;
        buf.flags = V4L2_BUF_FLAG_TIMECODE;
 
        if(-1 == ioctl(fd, VIDIOC_QUERYBUF, &buf))
        {
            cout << "failed to queery buffer %ld" << strerror(errno) << endl;
 
            return -1;
         
        }
 
        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start =
                mmap(NULL, // start anywhere
                     buf.length,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED,
                     fd, buf.m.offset);
 
        if(MAP_FAILED == buffers[n_buffers].start)
        {
            cout << "MAP_FAILED %ld" << strerror(errno) << endl;
 
            return -1;
         
        }
     
    }
    return 0;
 
 
}
 
int VideoDevice::start_capturing()
{
 
    unsigned int i;
    for(i = 0; i < n_buffers; ++i)
    {
 
        v4l2_buffer buf;
        CLEAR(buf);
 
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory =V4L2_MEMORY_MMAP;
        buf.index = i;
        buf.flags = V4L2_BUF_FLAG_TIMECODE;
//        fprintf(stderr, "n_buffers: %d\n", i);
 
        if(-1 == ioctl(fd, VIDIOC_QBUF, &buf))
        {
            cout << "ioctl failed %ld" << strerror(errno) << endl;
 
            return -1;
         
        }
     
    }
 
    v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 
    if(-1 == ioctl(fd, VIDIOC_STREAMON, &type))
    {
        cout << "ioctl failed VIDIOSCSTREAMON %ld" << strerror(errno) << endl;
 
        return -1;
     
    }
    return 0;
 
}
 
int VideoDevice::stop_capturing()
{
 
    v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 
    if(-1 == ioctl(fd, VIDIOC_STREAMOFF, &type))
    {
        cout << "ioctl failed stop capture %ld" << strerror(errno) << endl;
 
//        emit display_error(tr("VIDIOC_STREAMOFF: %1").arg(QString(strerror(errno))));
        return -1;
     
    }
    return 0;
 
}
 
int VideoDevice::uninit_device()
{
 
    unsigned int i;
    for(i = 0; i < n_buffers; ++i)
    {
 
        if(-1 == munmap(buffers[i].start, buffers[i].length))
        {
            cout << "ioctl failed unitit device %ld" << strerror(errno) << endl;
 
//            emit display_error(tr("munmap: %1").arg(QString(strerror(errno))));
            return -1;
         
        }
 
     
    }
    free(buffers);
    return 0;
 
}
 
int VideoDevice::get_frame(void **frame_buf, size_t* len, long * frame_count)
{
 
    v4l2_buffer queue_buf;
    CLEAR(queue_buf);

    fd_set fds;
    struct timeval tv;
    int r;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    /* Timeout. */
    tv.tv_sec = 4;
    tv.tv_usec = 0;

    r = select(fd + 1, &fds, NULL, NULL, &tv);

    if(r == -1)
    {
        switch(errno)
        {

        case EAGAIN:
            return -1;
        case EIO:
            return -1 ;
        default:
            cout << "failed select %ld" << strerror(errno) << endl;
//            emit display_error(tr("Select failed!: %1").arg(QString(strerror(errno))));
            return -1;

        }
    }

    queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    queue_buf.memory = V4L2_MEMORY_MMAP;

    if(-1 == ioctl(fd, VIDIOC_DQBUF, &queue_buf))
    {
 
        switch(errno)
        {
 
        case EAGAIN:
            return -1;
        case EIO:
            return -1 ;
        default:
            cout << "failed VIDIOC_DQBUF %ld" << strerror(errno) << endl;
//            emit display_error(tr("VIDIOC_DQBUF: %1").arg(QString(strerror(errno))));
            return -1;
         
        }
     
    }

    *frame_buf = buffers[queue_buf.index].start;
    *len = buffers[queue_buf.index].length;
    //   *frame_count =  queue_buf.timecode.frames;
    *frame_count =  0;

    index = queue_buf.index;

    return 0;
 
 
}
 
int VideoDevice::unget_frame()
{
 
    if(index != -1)
    {
 
        v4l2_buffer queue_buf;
        CLEAR(queue_buf);
 
        queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        queue_buf.memory = V4L2_MEMORY_MMAP;
        queue_buf.index = index;
 
        if(-1 == ioctl(fd, VIDIOC_QBUF, &queue_buf))
        {
            cout << "failed VIDIOC_QBUF %ld" << strerror(errno) << endl;
 
//            emit display_error(tr("VIDIOC_QBUF: %1").arg(QString(strerror(errno))));
            return -1;
         
        }
        return 0;
     
    }
    return -1;
 
}
