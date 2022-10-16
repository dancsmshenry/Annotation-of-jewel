// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_TCPCONNECTION_H
#define MUDUO_NET_TCPCONNECTION_H

#include "muduo/base/noncopyable.h"
#include "muduo/base/StringPiece.h"
#include "muduo/base/Types.h"
#include "muduo/net/Callbacks.h"
#include "muduo/net/Buffer.h"
#include "muduo/net/InetAddress.h"

#include <memory>

#include <boost/any.hpp>

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace muduo
{
namespace net
{

class Channel;
class EventLoop;
class Socket;

///
/// TCP connection, for both client and server usage.
///
/// This is an interface class, so don't expose too much details.
class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection>
{
 public:
  /// Constructs a TcpConnection with a connected sockfd
  ///
  /// User should not create this object.
  TcpConnection(EventLoop* loop,
                const string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
  ~TcpConnection();

  EventLoop* getLoop() const { return loop_; }
  const string& name() const { return name_; }
  const InetAddress& localAddress() const { return localAddr_; }
  const InetAddress& peerAddress() const { return peerAddr_; }
  bool connected() const { return state_ == kConnected; }
  bool disconnected() const { return state_ == kDisconnected; }
  // return true if success.
  bool getTcpInfo(struct tcp_info*) const;
  string getTcpInfoString() const;

  // void send(string&& message); // C++11
  void send(const void* message, int len);
  void send(const StringPiece& message);
  // void send(Buffer&& message); // C++11
  void send(Buffer* message);  // this one will swap data
  // 将connection设置为kDisconnecting，并关闭fd的写功能
  void shutdown(); // NOT thread safe, no simultaneous calling
  // void shutdownAndForceCloseAfter(double seconds); // NOT thread safe, no simultaneous calling
  // 强制关闭connetion
  void forceClose();
  // 过seconds后关闭connection
  void forceCloseWithDelay(double seconds);
  // 利用传参on来决定是否要用nagle功能
  void setTcpNoDelay(bool on);
  // reading or not
  // 开启fd的read功能
  void startRead();
  // 关闭fd的read功能
  void stopRead();
  bool isReading() const { return reading_; }; // NOT thread safe, may race with start/stopReadInLoop

  void setContext(const boost::any& context)
  { context_ = context; }

  const boost::any& getContext() const
  { return context_; }

  boost::any* getMutableContext()
  { return &context_; }

  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_ = cb; }

  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_ = cb; }

  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  { writeCompleteCallback_ = cb; }

  void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
  { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

  /// Advanced interface
  Buffer* inputBuffer()
  { return &inputBuffer_; }

  Buffer* outputBuffer()
  { return &outputBuffer_; }

  /// Internal use only.
  void setCloseCallback(const CloseCallback& cb)
  { closeCallback_ = cb; }

  // called when TcpServer accepts a new connection
  // 建立connection（只会被调用一次）
  void connectEstablished();   // should be called only once
  // called when TcpServer has removed me from its map
  // 销毁connection（只会被调用一次）
  void connectDestroyed();  // should be called only once

 private:
  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
  // 处理fd的可读事件
  void handleRead(Timestamp receiveTime);
  // 处理fd的可写事件
  void handleWrite();
  // 处理fd的close事件
  void handleClose();
  // 处理fd上的error事件
  void handleError();
  // void sendInLoop(string&& message);
  void sendInLoop(const StringPiece& message);
  void sendInLoop(const void* message, size_t len);
  // 在loop中注册的callback（关闭fd的写功能）
  void shutdownInLoop();
  // void shutdownAndForceCloseInLoop(double seconds);
  // 关闭connection的主要函数（注册到loop中使用）
  void forceCloseInLoop();
  void setState(StateE s) { state_ = s; }
  // 返回connection的state的char
  const char* stateToString() const;
  void startReadInLoop();
  void stopReadInLoop();

  // connection所在的loop
  EventLoop* loop_;
  // connection的姓名
  const string name_;
  // connection的状态
  StateE state_;  // FIXME: use atomic variable
  // 是否关注read事件
  bool reading_;
  // we don't expose those classes to client.
  // connection的socketfd
  std::unique_ptr<Socket> socket_;
  // connection的channel
  std::unique_ptr<Channel> channel_;
  const InetAddress localAddr_;
  const InetAddress peerAddr_;
  // 连接建立（或关闭）后的callback
  ConnectionCallback connectionCallback_;
  // read complete的callback
  MessageCallback messageCallback_;
  // write complete的callback
  WriteCompleteCallback writeCompleteCallback_;
  HighWaterMarkCallback highWaterMarkCallback_;
  // 连接关闭后的处理函数
  CloseCallback closeCallback_;
  size_t highWaterMark_;
  // TCP连接的输入缓冲区
  Buffer inputBuffer_;
  // TCP连接的输出缓冲区
  Buffer outputBuffer_; // FIXME: use list<Buffer> as output buffer.
  boost::any context_;
  // FIXME: creationTime_, lastReceiveTime_
  //        bytesReceived_, bytesSent_
};

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_TCPCONNECTION_H
