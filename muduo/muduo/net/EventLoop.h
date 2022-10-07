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
  // 开始loop
  void loop();

  /// Quits loop.
  ///
  /// This is not 100% thread safe, if you call through a raw pointer,
  /// better to call through shared_ptr<EventLoop> for 100% safety.
  // 退出loop
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
  // 执行cb函数（如果当前线程不是eventloop所在的线程，就调用queueinloop）
  void runInLoop(Functor cb);
  /// Queues callback in the loop thread.
  /// Runs after finish pooling.
  /// Safe to call from other threads.
  // 将callback放入queue中
  void queueInLoop(Functor cb);

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
  // 添加（更新）channel
  void updateChannel(Channel* channel);
  // 移除channel
  void removeChannel(Channel* channel);
  // 判断给定channel是否在当前loop中
  bool hasChannel(Channel* channel);

  // pid_t threadId() const { return threadId_; }
  // 判断当前thread和loop是否匹配
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

  // 获取当前thread的eventloop
  static EventLoop* getEventLoopOfCurrentThread();

 private:
  void abortNotInLoopThread();
  // 处理wakefd中的可读事件
  void handleRead();  // waked up
  // 消费vector中的callback
  void doPendingFunctors();

  void printActiveChannels() const; // DEBUG

  typedef std::vector<Channel*> ChannelList;

  // eventloop是否在loop()（默认是false）
  bool looping_;
  // eventloop是否退出loop()（默认是false）
  std::atomic<bool> quit_;
  // eventloop是否在处理fd（默认是false）
  bool eventHandling_;
  // eventloop是否在执行doPendingFunctors()
  bool callingPendingFunctors_;
  // loop()中while的次数
  int64_t iteration_;
  // 当前thread的id
  const pid_t threadId_;
  // 调用poller后返回的time
  Timestamp pollReturnTime_;
  // 持有poller的ptr
  std::unique_ptr<Poller> poller_;
  // 持有timequeue的ptr
  std::unique_ptr<TimerQueue> timerQueue_;
  // fd（用于创造可读事件唤醒loop）
  int wakeupFd_;
  // unlike in TimerQueue, which is an internal class,
  // we don't expose Channel to client.
  // wakeupFd_对应的channel
  std::unique_ptr<Channel> wakeupChannel_;
  boost::any context_;

  // scratch variables
  // 存放活动的事件（vector）
  ChannelList activeChannels_;
  // loop正在处理的channle
  Channel* currentActiveChannel_;

  mutable MutexLock mutex_;
  std::vector<Functor> pendingFunctors_ GUARDED_BY(mutex_);
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_EVENTLOOP_H
