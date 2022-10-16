// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_CONNECTOR_H
#define MUDUO_NET_CONNECTOR_H

#include "muduo/base/noncopyable.h"
#include "muduo/net/InetAddress.h"

#include <functional>
#include <memory>

namespace muduo
{
namespace net
{

class Channel;
class EventLoop;

class Connector : noncopyable,
                  public std::enable_shared_from_this<Connector>
{ // client使用，和acceptor是对应的
 public:
  typedef std::function<void (int sockfd)> NewConnectionCallback;

  Connector(EventLoop* loop, const InetAddress& serverAddr);
  ~Connector();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  // 开始连接
  void start();  // can be called in any thread
  // 重新建立连接
  void restart();  // must be called in loop thread
  // 断开连接
  void stop();  // can be called in any thread

  const InetAddress& serverAddress() const { return serverAddr_; }

 private:
  enum States { kDisconnected, kConnecting, kConnected };
  // 默认的最大重连间隔
  static const int kMaxRetryDelayMs = 30*1000;
  static const int kInitRetryDelayMs = 500;

  // 重置state
  void setState(States s) { state_ = s; }
  // 注册连接callback到loop中（调用connect()）
  void startInLoop();
  // 注册断开callback到loop中（调用close()）
  void stopInLoop();
  // 建立连接（非阻塞socket和connect调用）
  void connect();
  // 建立socketfd和channel的关系
  void connecting(int sockfd);
  // 处理可写事件
  void handleWrite();
  // 处理error事件
  void handleError();
  // connect_为false：close fd；为true（重新socket，connect等步骤）
  void retry(int sockfd);
  // 将channel从loop中移除（同时返回socketfd）
  int removeAndResetChannel();
  // 释放持有channel的ptr
  void resetChannel();

  // 对应的eventloop
  EventLoop* loop_;
  // 要连接的服务端地址
  InetAddress serverAddr_;
  // 是否开始连接
  bool connect_; // atomic
  // connector的状态
  States state_;  // FIXME: use atomic variable
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback newConnectionCallback_;
  // 默认重连的时间参数
  int retryDelayMs_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_CONNECTOR_H
