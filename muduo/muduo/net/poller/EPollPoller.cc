// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/poller/EPollPoller.h"

#include "muduo/base/Logging.h"
#include "muduo/net/Channel.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

// On Linux, the constants of poll(2) and epoll(4)
// are expected to be the same.
static_assert(EPOLLIN == POLLIN,        "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI,      "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT,      "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP,  "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR,      "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP,      "epoll uses same flag values as poll");

namespace
{
const int kNew = -1;// 表示当前的channel还没有加入到epollfd中
const int kAdded = 1;// 表示当前的channel已经被加入到epollfd中
const int kDeleted = 2;// 表示当前的channel已经从epollfd中删除（但还是在channels_）
}

EPollPoller::EPollPoller(EventLoop* loop)
  : Poller(loop),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventListSize)
{
  if (epollfd_ < 0)
  {// 创建epollfd失败
    LOG_SYSFATAL << "EPollPoller::EPollPoller";
  }
}

EPollPoller::~EPollPoller()
{
  // 关闭epollfd
  ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
  LOG_TRACE << "fd total count " << channels_.size();
  //  epoll_wait
  int numEvents = ::epoll_wait(epollfd_,
                               &*events_.begin(), //  通过解引用begin得到原生的数组
                               static_cast<int>(events_.size()),
                               timeoutMs);
  int savedErrno = errno;
  // 记录当前的时间，以便后续返回
  Timestamp now(Timestamp::now());
  if (numEvents > 0)
  {// 有事发生
    LOG_TRACE << numEvents << " events happened";
    // 将事件放入activeChannels中
    fillActiveChannels(numEvents, activeChannels);
    if (implicit_cast<size_t>(numEvents) == events_.size())
    {// 如果events_已经装满了（是指达到已有的size了），就对已有的eventlist进行两倍扩容
     // 这里要注重的细节就是，删除fd的时候，是没有在events_上操作的 
      events_.resize(events_.size()*2);
    }
  }
  else if (numEvents == 0)
  {// 无事发生
    LOG_TRACE << "nothing happened";
  }
  else
  {// 发生异常
    // error happens, log uncommon ones
    if (savedErrno != EINTR)
    {
      errno = savedErrno;
      LOG_SYSERR << "EPollPoller::poll()";
    }
  }
  return now;
}

void EPollPoller::fillActiveChannels(int numEvents,
                                     ChannelList* activeChannels) const
{
  assert(implicit_cast<size_t>(numEvents) <= events_.size());
  for (int i = 0; i < numEvents; ++i)
  {
    Channel* channel = static_cast<Channel*>(events_[i].data.ptr); // 原来它这里是把channel放到了epollfd的data中
#ifndef NDEBUG // 用于屏蔽assert
    int fd = channel->fd();
    ChannelMap::const_iterator it = channels_.find(fd);
    assert(it != channels_.end());
    assert(it->second == channel);
#endif
    channel->set_revents(events_[i].events);
    activeChannels->push_back(channel);
  }
}

void EPollPoller::updateChannel(Channel* channel)
{
  Poller::assertInLoopThread();
  const int index = channel->index();
  LOG_TRACE << "fd = " << channel->fd()
    << " events = " << channel->events() << " index = " << index;
  if (index == kNew || index == kDeleted)
  {
    // a new one, add with EPOLL_CTL_ADD
    int fd = channel->fd();
    if (index == kNew)
    {// 当前的channel是一个未曾加入到poller的channel
      assert(channels_.find(fd) == channels_.end());
      channels_[fd] = channel;
    }
    else
    {// 当前channel是从epollfd中删除了（但是是已经放入到channels_中的）
      assert(channels_.find(fd) != channels_.end());
      assert(channels_[fd] == channel);
    }

    channel->set_index(kAdded);
    update(EPOLL_CTL_ADD, channel);
  }
  else
  {// 更新一个已存在的fd
    // update existing one with EPOLL_CTL_MOD/DEL
    int fd = channel->fd();
    (void)fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(index == kAdded);
    if (channel->isNoneEvent())
    {// 如果当前channel对所有的事件都不感兴趣，就把它从epollfd中移除
      update(EPOLL_CTL_DEL, channel);
      channel->set_index(kDeleted);
    }
    else
    {// 否则就更新到epollfd中
      update(EPOLL_CTL_MOD, channel);
    }
  }
}

void EPollPoller::removeChannel(Channel* channel)
{
  Poller::assertInLoopThread();
  int fd = channel->fd();
  LOG_TRACE << "fd = " << fd;
  assert(channels_.find(fd) != channels_.end());
  assert(channels_[fd] == channel);
  // 要保证要移除的channel不对任何事件感兴趣
  assert(channel->isNoneEvent());
  int index = channel->index();
  // 要保证当前要移除的channel是在channels_中的（否则限免erase就会报错）
  assert(index == kAdded || index == kDeleted);
  size_t n = channels_.erase(fd); //  删除channel
  (void)n;
  assert(n == 1);

  if (index == kAdded)
  {
    update(EPOLL_CTL_DEL, channel);
  }
  channel->set_index(kNew);
}

void EPollPoller::update(int operation, Channel* channel)
{
  struct epoll_event event;
  memZero(&event, sizeof event);
  // 组装epoll_event
  event.events = channel->events();
  event.data.ptr = channel;
  int fd = channel->fd();
  LOG_TRACE << "epoll_ctl op = " << operationToString(operation)
    << " fd = " << fd << " event = { " << channel->eventsToString() << " }";
  // epoll_ctl的第一个参数是epollfd的fd，第二个参数是表明要对fd在epoll中的操作
  // EPOLL_CTL_ADD表明添加fd到epoll中
  // EPOLL_CTL_MOD表明修改epoll中的fd
  // EPOLL_CTL_DEL表明从epoll中删除该fd
  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
  {// 返回-1，表示修改channel的状态失败
    if (operation == EPOLL_CTL_DEL)
    {
      LOG_SYSERR << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
    }
    else
    {
      LOG_SYSFATAL << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
    }
  }
}

const char* EPollPoller::operationToString(int op)
{
  switch (op)
  {
    case EPOLL_CTL_ADD:
      return "ADD";
    case EPOLL_CTL_DEL:
      return "DEL";
    case EPOLL_CTL_MOD:
      return "MOD";
    default:
      assert(false && "ERROR op");
      return "Unknown Operation";
  }
}
