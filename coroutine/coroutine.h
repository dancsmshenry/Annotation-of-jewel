#ifndef C_COROUTINE_H
#define C_COROUTINE_H

#define COROUTINE_DEAD 0    //  被销毁后的状态
#define COROUTINE_READY 1   //  准备（还没运行的）
#define COROUTINE_RUNNING 2 //  正在运行
#define COROUTINE_SUSPEND 3 //  挂起，暂停（一般是yield后）

struct schedule;

typedef void (*coroutine_func)(struct schedule *, void *ud);

struct schedule * coroutine_open(void);
void coroutine_close(struct schedule *);

int coroutine_new(struct schedule *, coroutine_func, void *ud);
void coroutine_resume(struct schedule *, int id);
int coroutine_status(struct schedule *, int id);
int coroutine_running(struct schedule *);
void coroutine_yield(struct schedule *);

#endif
