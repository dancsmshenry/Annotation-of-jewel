// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <atomic>
#include <functional>
#include <vector>

#include <boost/any.hpp>

#include "muduo/base/Mutex.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/Timestamp.h"
#include "muduo/net/Callbacks.h"
#include "muduo/net/TimerId.h"

namespace muduo
{
namespace net
{

class Channel;
class Poller;
class TimerQueue;

///
/// Reactor, at most one per thread.
///
/// This is an interface class, so don't expose too much details.
class EventLoop : noncopyable
{
 public:
  typedef std::function<void()> Functor;

  EventLoop();
  ~EventLoop();  // force out-line dtor, for std::unique_ptr members.

  ///
  /// Loops forever.
  ///
  /// Must be called in the same thread as creation of the object.
  ///
  // 开始事件循环
  void loop();

  /// Quits loop.
  ///
  /// This is not 100% thread safe, if you call through a raw pointer,
  /// better to call through shared_ptr<EventLoop> for 100% safety.
  // 退出事件循环
  void quit();

  ///
  /// Time when poll returns, usually means data arrival.
  ///
  // poller返回的时间戳
  Timestamp pollReturnTime() const { return pollReturnTime_; }

  // 返回loop循环的次数
  int64_t iteration() const { return iteration_; }

  /// Runs callback immediately in the loop thread.
  /// It wakes up the loop, and run the cb.
  /// If in the same loop thread, cb is run within the function.
  /// Safe to call from other threads.
  // 让loop执行callback函数（如果当前线程不是eventloop所在的线程，就调用queueinloop）
  void runInLoop(Functor cb);
  /// Queues callback in the loop thread.
  /// Runs after finish pooling.
  /// Safe to call from other threads.
  // 将callback放入queue中
  void queueInLoop(Functor cb);

  // 返回当前消费队列的长度
  size_t queueSize() const;

  // timers

  ///
  /// Runs callback at 'time'.
  /// Safe to call from other threads.
  ///
  TimerId runAt(Timestamp time, TimerCallback cb);
  ///
  /// Runs callback after @c delay seconds.
  /// Safe to call from other threads.
  ///
  TimerId runAfter(double delay, TimerCallback cb);
  ///
  /// Runs callback every @c interval seconds.
  /// Safe to call from other threads.
  ///
  TimerId runEvery(double interval, TimerCallback cb);
  ///
  /// Cancels the timer.
  /// Safe to call from other threads.
  ///
  void cancel(TimerId timerId);

  // 触发可写事件，唤醒loop
  void wakeup();
  // 更新channel
  void updateChannel(Channel* channel);
  // 删除channel
  void removeChannel(Channel* channel);
  // 判断给定的channel是否在当前的eventloop的poller中
  bool hasChannel(Channel* channel);

  // pid_t threadId() const { return threadId_; }
  void assertInLoopThread()
  {
    if (!isInLoopThread())
    {
      abortNotInLoopThread();
    }
  }
  // 判断当前loop是否在当前的这个线程中
  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
  // bool callingPendingFunctors() const { return callingPendingFunctors_; }
  bool eventHandling() const { return eventHandling_; }

  void setContext(const boost::any& context)
  { context_ = context; }

  const boost::any& getContext() const
  { return context_; }

  boost::any* getMutableContext()
  { return &context_; }

  static EventLoop* getEventLoopOfCurrentThread();

 private:
  void abortNotInLoopThread();
  // 当wakeupfd可写时，就用该函数处理
  void handleRead();  // waked up
  // 处理pendingFunctors_中的回调函数
  void doPendingFunctors();

  void printActiveChannels() const; // DEBUG

  typedef std::vector<Channel*> ChannelList;

  // 标记当前eventloop是否在loop中
  bool looping_;
  // 标记当前eventloop是否退出loop
  std::atomic<bool> quit_;
  // 标记此时是否在处理poller得到的活跃事件
  bool eventHandling_;
  // 标记当前的eventloop是否在执行doPendingFunctors
  bool callingPendingFunctors_;
  // 记录loop中while的次数
  int64_t iteration_;
  const pid_t threadId_;
  Timestamp pollReturnTime_;
  // 指向poller的指针
  std::unique_ptr<Poller> poller_;
  // 指向timequeue的指针
  std::unique_ptr<TimerQueue> timerQueue_;
  // fd，用于创造可读事件，从而唤醒poller
  int wakeupFd_;
  // unlike in TimerQueue, which is an internal class,
  // we don't expose Channel to client.
  // 上述可读事件的channel
  std::unique_ptr<Channel> wakeupChannel_;
  boost::any context_;

  // scratch variables
  // 存放活动的事件
  ChannelList activeChannels_;
  // 当前活跃的channel，是指此时loop中正在处理的channel
  Channel* currentActiveChannel_;

  mutable MutexLock mutex_;
  std::vector<Functor> pendingFunctors_ GUARDED_BY(mutex_);
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_EVENTLOOP_H
