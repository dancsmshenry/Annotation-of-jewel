// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/Acceptor.h"

#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/SocketsOps.h"

#include <errno.h>
#include <fcntl.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
  : loop_(loop),
    acceptSocket_(sockets::createNonblockingOrDie(listenAddr.family())),// 创建非阻塞socket
    acceptChannel_(loop, acceptSocket_.fd()),
    listening_(false),
    idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
  // 判断fd是否创建成功
  assert(idleFd_ >= 0);
  acceptSocket_.setReuseAddr(true);
  acceptSocket_.setReusePort(reuseport);
  acceptSocket_.bindAddress(listenAddr);
  // listenfd的可读事件（用来处理新来的fd）
  acceptChannel_.setReadCallback(
      std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
  // 将channel设置为对所有事件都不感兴趣
  acceptChannel_.disableAll();
  // 将当前channel从eventloop中移除
  acceptChannel_.remove();
  // 将持有的空闲fd关闭
  ::close(idleFd_);
}

void Acceptor::listen()
{
  loop_->assertInLoopThread();
  // 将acceptor设置为listen状态
  listening_ = true;
  // 调用socket的listen
  acceptSocket_.listen();
  // 关注socket的可读事件
  acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
  loop_->assertInLoopThread();
  InetAddress peerAddr;
  //FIXME loop until no more
  // 调用socket的accept
  int connfd = acceptSocket_.accept(&peerAddr);
  if (connfd >= 0)
  {// 连接正常
    // string hostport = peerAddr.toIpPort();
    // LOG_TRACE << "Accepts of " << hostport;
    if (newConnectionCallback_)
    {//  如果有设置连接的回调函数（即创建一个tcpconnection对象）
      newConnectionCallback_(connfd, peerAddr);
    }
    else
    {// 如果没有设置连接的回调函数，就直接close
      sockets::close(connfd);
    }
  }
  else
  {// 连接异常
    LOG_SYSERR << "in Acceptor::handleRead";
    // Read the section named "The special problem of
    // accept()ing when you can't" in libev's doc.
    // By Marc Lehmann, author of libev.
    if (errno == EMFILE)
    {// 说明fd数量不够，用空闲fd解决连接数量过多的问题（不用考虑并发，因为整个进程中只有一个acceptor）
      ::close(idleFd_);
      idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
      ::close(idleFd_);
      idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}

