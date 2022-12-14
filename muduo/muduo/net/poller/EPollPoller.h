// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_POLLER_EPOLLPOLLER_H
#define MUDUO_NET_POLLER_EPOLLPOLLER_H

#include "muduo/net/Poller.h"

#include <vector>

struct epoll_event;

namespace muduo
{
namespace net
{

///
/// IO Multiplexing with epoll(4).
///
class EPollPoller : public Poller
{
 public:
  EPollPoller(EventLoop* loop);
  ~EPollPoller() override;

  // 等价于epoll_wait（timeouts为poller阻塞的时间，activechannels是一个存放channel指针的vector）
  Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
  // 添加（更新）channel
  void updateChannel(Channel* channel) override;
  // 移除channel
  void removeChannel(Channel* channel) override;

 private:
  // events_数组的大小
  static const int kInitEventListSize = 16;

  // 根据状态转换为对应的字符串
  static const char* operationToString(int op);

  // 将epoll_wait返回的事件放到activechannels中
  void fillActiveChannels(int numEvents,
                          ChannelList* activeChannels) const;
  // 等价于epoll_ctl
  void update(int operation, Channel* channel);

  typedef std::vector<struct epoll_event> EventList;

  // epollfd
  int epollfd_;
  // 数组，存放epoll_wait返回的事件
  EventList events_;
};

}  // namespace net
}  // namespace muduo
#endif  // MUDUO_NET_POLLER_EPOLLPOLLER_H
