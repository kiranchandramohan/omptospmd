#ifndef _BARRIER_H
#define _BARRIER_H

#define ADD 0

#define NEED_BARRIER 1
#define MY_MACIG 'G'
#define READ_IOCTL _IOR(MY_MACIG, 0, int)
#define WRITE_IOCTL _IOW(MY_MACIG, 1, int)
#define ATTACH_RDWRITE_IOCTL _IOW(MY_MACIG, 2, int)
#define ATTACH_RDONLY_IOCTL _IOW(MY_MACIG, 3, int)
#define DETACH_IOCTL _IOW(MY_MACIG, 4, int)
#define BARRIER_IOCTL _IOW(MY_MACIG, 5, int*)
#define LOCK_IOCTL _IOW(MY_MACIG, 6, int*)
#define UNLOCK_IOCTL _IOW(MY_MACIG, 7, int*)
#define DEBUG_IOCTL _IO(MY_MACIG, 8)
#define INIT_BARRIER_IOCTL _IOW(MY_MACIG, 9, struct barrier*)
#define FINALIZE_BARRIER_IOCTL _IO(MY_MACIG, 10)
#define INIT_COUNT_BARRIER_IOCTL _IOW(MY_MACIG, 11, struct barrier*)
#define REDUCE_IOCTL _IOW(MY_MACIG, 12, struct my_buffer*)
#define READ_REDUCE_IOCTL _IOWR(MY_MACIG, 13, struct my_buffer*)
#define INIT_REDUCE_IOCTL _IOWR(MY_MACIG, 14, struct my_buffer*)


#define NUM_BARRIER 3

struct barrier
{
	int indx ;
	int count ;
} ;

struct my_buffer
{
	int red_num ;
	int inc_val ;
} ;


void call_barrier(int indx)  ;
void init_barrier(int num_threads_barrier) ;
void init_count_barrier(int num_threads_barrier) ;
void open_barrier(void) ;
void init_reduce(int red_num, int inc_val) ;
int call_read_reduce(int red_num) ;
void call_reduce(int red_num, int type, int inc_val) ;
void close_barrier(void) ;
void finalize_barrier(void) ;
void lock_hwspinlock(int lock_id) ;
void unlock_hwspinlock(int lock_id) ;

#endif
