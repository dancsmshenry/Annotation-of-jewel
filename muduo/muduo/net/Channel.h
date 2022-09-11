// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include "muduo/base/noncopyable.h"
#include "muduo/base/Timestamp.h"

#include <functional>
#include <memory>

namespace muduo
{
namespace net
{

class EventLoop;

///
/// A selectable I/O channel.
///
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,
/// an eventfd, a timerfd, or a signalfd
class Channel : noncopyable
{
 public:
  typedef std::function<void()> EventCallback;
  typedef std::function<void(Timestamp)> ReadEventCallback;

  // 归属于哪一个loop，监听哪一个fd
  Channel(EventLoop* loop, int fd);
  ~Channel();

  // 处理fd上的事件（会被eventloop调用）
  void handleEvent(Timestamp receiveTime);
  void setReadCallback(ReadEventCallback cb)
  { readCallback_ = std::move(cb); }
  void setWriteCallback(EventCallback cb)
  { writeCallback_ = std::move(cb); }
  void setCloseCallback(EventCallback cb)
  { closeCallback_ = std::move(cb); }
  void setErrorCallback(EventCallback cb)
  { errorCallback_ = std::move(cb); }

  /// Tie this channel to the owner object managed by shared_ptr,
  /// prevent the owner object being destroyed in handleEvent.
  void tie(const std::shared_ptr<void>&);

  int fd() const { return fd_; }
  int events() const { return events_; }
  // 设定发生事件的类型
  void set_revents(int revt) { revents_ = revt; } // used by pollers
  // int revents() const { return revents_; }
  // 表示当前fd对任何事件都不感兴趣
  bool isNoneEvent() const { return events_ == kNoneEvent; }

  // 设置fd感兴趣的事件，并更新

  void enableReading() { events_ |= kReadEvent; update(); }
  void disableReading() { events_ &= ~kReadEvent; update(); }
  void enableWriting() { events_ |= kWriteEvent; update(); }
  void disableWriting() { events_ &= ~kWriteEvent; update(); }
  void disableAll() { events_ = kNoneEvent; update(); }

  // 判断当前的事件是否能读或写

  bool isWriting() const { return events_ & kWriteEvent; }
  bool isReading() const { return events_ & kReadEvent; }

  // for Poller
  int index() { return index_; }
  // 更新channel在poller中的状态
  void set_index(int idx) { index_ = idx; }

  // for debug
  string reventsToString() const;
  string eventsToString() const;

  void doNotLogHup() { logHup_ = false; }

  EventLoop* ownerLoop() { return loop_; }
  // 把当前channel从loop中移除
  void remove();

 private:
  static string eventsToString(int fd, int ev);

  // 把更新后的channel重新放回loop
  void update();
  // 真正处理event的函数（通过位运算判断要调用哪一个事件函数）
  void handleEventWithGuard(Timestamp receiveTime);

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  // channel隶属于哪一个事件循环
  EventLoop* loop_;
  // channel管理的文件描述符
  const int  fd_;
  // fd感兴趣的事件类型的集合
  int        events_;
  // fd上实际发生的事件的集合
  int        revents_; // it's the received event types of epoll or poll
  // 表示当前fd在poller中的状态（-1表示还没有被添入到poll中，1表示已经被加入了，2表示被删除了）
  int        index_;
  // 用于表示是否要写关于pollup的日志
  bool       logHup_;

  std::weak_ptr<void> tie_;
  bool tied_;
  // 标记当前是否在处理fd上发生的事件
  bool eventHandling_;
  // 标记当前的channel是否被加入到eventloop中
  bool addedToLoop_;
  ReadEventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_CHANNEL_H
