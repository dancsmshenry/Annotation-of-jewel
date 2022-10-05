#include "coroutine.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#if __APPLE__ && __MACH__
	#include <sys/ucontext.h>
#else 
	#include <ucontext.h>
#endif 

#define STACK_SIZE (1024*1024)	//	默认的stack的大小
#define DEFAULT_COROUTINE 16	//	默认的coroutine数量

struct coroutine;

struct schedule {
	char stack[STACK_SIZE];	//	运行时的stack（共享栈），是所有协程运行时的stack
	ucontext_t main;	//	主coroutine的上下文（方便后续切回主coroutine）
	int nco;	//	当前存活的coroutine的数量
	int cap;	//	coroutine管理器的最大容量，即当前schedule最多支持多少个coroutine（支持2倍扩容）
	int running;	//	当前正在running的coroutine的id
	struct coroutine **co;	//	存放coroutine指针的一维数组（长度为cap）
};

struct coroutine {
	coroutine_func func;	//	coroutine要执行的函数
	void *ud;	//	coroutine的参数（可以理解为是要传入的函数参数）
	ucontext_t ctx;	//	当前coroutine的上下文
	struct schedule * sch;	//	当前coroutine隶属的调度器
	ptrdiff_t cap;	//	已经分配的内存大小
	ptrdiff_t size;	//	当前协程运行时栈，保存起来后的大小
	int status;	//	当前coroutine的状态
	char *stack;	//	当前协程的保存起来的运行时栈
};

// 新建一个coroutine
struct coroutine * 
_co_new(struct schedule *S , coroutine_func func, void *ud) {
	struct coroutine * co = malloc(sizeof(*co));
	co->func = func;
	co->ud = ud;
	co->sch = S;
	co->cap = 0;
	co->size = 0;
	co->status = COROUTINE_READY;	//	起始状态设为ready
	co->stack = NULL;
	return co;
}

// 删除coroutine
void
_co_delete(struct coroutine *co) {
	free(co->stack);
	free(co);
}

// 创建一个coroutine schedule
struct schedule * 
coroutine_open(void) {
	struct schedule *S = malloc(sizeof(*S));
	S->nco = 0;
	S->cap = DEFAULT_COROUTINE;
	S->running = -1;
	S->co = malloc(sizeof(struct coroutine *) * S->cap);
	memset(S->co, 0, sizeof(struct coroutine *) * S->cap);
	return S;
}

// 删除coroutine schedule，并且把其中所有的coroutine给删除掉
void 
coroutine_close(struct schedule *S) {
	int i;
	for (i=0;i<S->cap;i++) {
		struct coroutine * co = S->co[i];
		if (co) {
			_co_delete(co);
		}
	}
	free(S->co);
	S->co = NULL;
	free(S);
}

// 创建一个新的coroutine，添加到schedule中，并返回其id
int 
coroutine_new(struct schedule *S, coroutine_func func, void *ud) {
	struct coroutine *co = _co_new(S, func , ud);
	if (S->nco >= S->cap) {	//	coroutine数组满了，需要扩容
		int id = S->cap;
		S->co = realloc(S->co, S->cap * 2 * sizeof(struct coroutine *));
		memset(S->co + S->cap , 0 , sizeof(struct coroutine *) * S->cap);
		S->co[S->cap] = co;
		S->cap *= 2;
		++S->nco;
		return id;
	} else {	//	寻找是否有空位，并将当前coroutine放入
		int i;
		for (i=0;i<S->cap;i++) {
			int id = (i+S->nco) % S->cap;	//	细节：从当前存货数的地方开始找，提高效率
			if (S->co[id] == NULL) {
				S->co[id] = co;
				++S->nco;
				return id;
			}
		}
	}
	assert(0);
	return -1;
}

// 设置coroutine对应的func
static void
mainfunc(uint32_t low32, uint32_t hi32) {
	// 组装指针XD
	uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
	struct schedule *S = (struct schedule *)ptr;
	int id = S->running;
	struct coroutine *C = S->co[id];
	C->func(S,C->ud);	//	运行coroutine的函数
	_co_delete(C);	//	执行完了就删除（后续都是对该coroutine的处理）
	S->co[id] = NULL;
	--S->nco;
	S->running = -1;
}

// coroutine的resume（需要根据coroutine的状态执行不同的操作）
void 
coroutine_resume(struct schedule * S, int id) {
	assert(S->running == -1);	//	确保当前有coroutine在运行
	assert(id >=0 && id < S->cap);	//	确保给定的id是正确的
	struct coroutine *C = S->co[id];
	if (C == NULL)	//	如果当前coroutine是null，就退出
		return;
	int status = C->status;
	switch(status) {
		// ready -> running
	case COROUTINE_READY:	//	让coroutine转为running
		getcontext(&C->ctx);	//	将当前的上下文放到C->ctx里面（因为此时子coroutine还没有执行，所以没有上下文）
		C->ctx.uc_stack.ss_sp = S->stack;	//	设置当前coroutine运行时的栈
		C->ctx.uc_stack.ss_size = STACK_SIZE;
		C->ctx.uc_link = &S->main;	//	如果当前coroutine执行完了，就切换回S->main的主协程中运行
		S->running = id;	//	设置当前正在运行的coroutine的id
		C->status = COROUTINE_RUNNING;	//	调整状态
		uintptr_t ptr = (uintptr_t)S;
		makecontext(&C->ctx, (void (*)(void)) mainfunc, 2, (uint32_t)ptr, (uint32_t)(ptr>>32));	//	设置ucontext的执行函数
		swapcontext(&S->main, &C->ctx);	//	将当前coroutine的上下文放到S->main，并将C->ctx的上下文替换到当前上下文
		// PS：这里实际上就是将当前main coroutine的上下文保存好，同时切换到target coroutine中执行
		break;
		// suspend -> running
	case COROUTINE_SUSPEND:	//	此时的coroutine是被yield的，所以需要恢复到running状态
		memcpy(S->stack + STACK_SIZE - C->size, C->stack, C->size);	//	把coroutine的之前保存的栈内容，重新拷贝到当前栈里面
		S->running = id;
		C->status = COROUTINE_RUNNING;
		swapcontext(&S->main, &C->ctx);	//	切换上下文
		break;
	default:
		assert(0);
	}
}

// 保存当前栈数据
static void
_save_stack(struct coroutine *C, char *top) {
	char dummy = 0;
	assert(top - &dummy <= STACK_SIZE);
	if (C->cap < top - &dummy) {	//	如果已分配内存小于当前栈的大小，则释放内存重新分配
		free(C->stack);
		C->cap = top-&dummy;
		C->stack = malloc(C->cap);
	}
	C->size = top - &dummy;
	memcpy(C->stack, &dummy, C->size);
}

// coroutine转让，使当前正在运行的协程切换到主协程中运行
void
coroutine_yield(struct schedule * S) {
	int id = S->running;
	assert(id >= 0);
	struct coroutine * C = S->co[id];
	assert((char *)&C > S->stack);
	_save_stack(C,S->stack + STACK_SIZE);	//	保存当前的栈数据
	C->status = COROUTINE_SUSPEND;	//	状态修改为暂停
	S->running = -1;
	swapcontext(&C->ctx , &S->main);	//	将当前的上下文保存到当前coroutine的ucontext中，同时将主协程的上下文替换为当前上下文
}

// 返回当前所指coroutine的status，如果没有coroutine的话，就返回COROUTINE_DEAD
int 
coroutine_status(struct schedule * S, int id) {
	assert(id>=0 && id < S->cap);
	if (S->co[id] == NULL) {
		return COROUTINE_DEAD;
	}
	return S->co[id]->status;
}

// 返回当前正在运行的coroutine（-1则无coroutine运行）
int 
coroutine_running(struct schedule * S) {
	return S->running;
}

