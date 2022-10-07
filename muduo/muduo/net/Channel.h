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

  // 对应的loop和fd
  Channel(EventLoop* loop, int fd);
  ~Channel();

  // 处理channel上的事件（对外提供结构给eventloop使用）
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
  void set_revents(int revt) { revents_ = revt; } // used by pollers
  // int revents() const { return revents_; }
  bool isNoneEvent() const { return events_ == kNoneEvent; }

  // 设置channel关注的事件

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
  // 把channel从loop中移除
  void remove();

 private:
  static string eventsToString(int fd, int ev);

  // 把channel放入loop中（或是把修改后的channel放入loop中）
  void update();
  // 真正处理channel上事件的函数（通过位运算switch）
  void handleEventWithGuard(Timestamp receiveTime);

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  // channel对应的loop
  EventLoop* loop_;
  // channel对应的fd
  const int  fd_;
  // 关注的事件（默认是kNoneEvent，kNoneEvent表示不关注，kReadEvent表示关注读事件，kWriteEvent表示关注写事件）
  int        events_;
  // 实际发生的事件
  int        revents_; // it's the received event types of epoll or poll
  // 当前的状态（默认是-1，-1表示还没有被放入到poll中，1表示已经放入，2表示被删除）
  int        index_;
  // 表示是否要写关于pollup的日志（默认是true）
  bool       logHup_;

  std::weak_ptr<void> tie_;
  bool tied_;
  // 是否正在处理fd上发生的事件（默认是false）
  bool eventHandling_;
  // channel是否放入loop中（默认是false）
  bool addedToLoop_;
  // 处理读事件
  ReadEventCallback readCallback_;
  // 处理写事件
  EventCallback writeCallback_;
  // 处理fd的close
  EventCallback closeCallback_;
  // 处理error
  EventCallback errorCallback_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_CHANNEL_H
