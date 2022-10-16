// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_EVENTLOOPTHREADPOOL_H
#define MUDUO_NET_EVENTLOOPTHREADPOOL_H

#include "muduo/base/noncopyable.h"
#include "muduo/base/Types.h"

#include <functional>
#include <memory>
#include <vector>

namespace muduo
{

namespace net
{

class EventLoop;
class EventLoopThread;

// 出现在tcpserver中
class EventLoopThreadPool : noncopyable
{
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;

  EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg);
  ~EventLoopThreadPool();
  // 设置threadpool中thread的数量
  void setThreadNum(int numThreads) { numThreads_ = numThreads; }
  // pool开始工作（由tcpserver调用）
  void start(const ThreadInitCallback& cb = ThreadInitCallback());

  // valid after calling start()
  /// round-robin
  // tcpserver获取connect后需要放入到一个loop中，该函数的作用就是选取一个loop放入
  EventLoop* getNextLoop();

  /// with the same hash code, it will always return the same EventLoop
  // 给定hash值，返回对应位置的eventloop
  EventLoop* getLoopForHash(size_t hashCode);

  // 获取所有的loops
  std::vector<EventLoop*> getAllLoops();

  bool started() const
  { return started_; }

  const string& name() const
  { return name_; }

 private:

  // 主eventloop
  EventLoop* baseLoop_;
  // pool的名字
  string name_;
  // 标记是否调用了start()
  bool started_;
  // thread的数量
  int numThreads_;
  // 用于从loops_中选取loop（用于负载均衡）
  int next_;
  // 存放eventloopthread*的pool
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  // 存放eventloop*的pool（也可以理解为是一个线程池，主eventloop没放进来的）
  std::vector<EventLoop*> loops_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_EVENTLOOPTHREADPOOL_H
