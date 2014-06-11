
#ifndef __ESC_IOC_H__
#define __ESC_IOC_H__

long escdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#endif // __ESC_IOC_H__
