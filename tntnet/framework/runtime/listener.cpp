/* listener.cpp
   Copyright (C) 2003 Tommi Maekitalo

This file is part of tntnet.

Tntnet is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Tntnet is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with tntnet; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330,
Boston, MA  02111-1307  USA
*/

#include "tnt/listener.h"
#include "tnt/tntnet.h"
#include <cxxtools/log.h>
#include <errno.h>
#include <unistd.h>

#ifdef USE_SSL
#  include "tnt/ssl.h"
#endif

log_define("tntnet.listener")

static void doListenRetry(cxxtools::net::Server& server,
  const char* ipaddr, unsigned short int port)
{
  for (unsigned n = 1; true; ++n)
  {
    try
    {
      log_debug("listen " << ipaddr << ':' << port);
      server.listen(ipaddr, port, tnt::Listener::getBacklog());
      return;
    }
    catch (const cxxtools::net::Exception& e)
    {
      log_debug("cxxtools::net::Exception catched: errno=" << e.getErrno() << " msg=" << e.what());
      if (e.getErrno() != EADDRINUSE || n > tnt::Listener::getListenRetry())
      {
        log_debug("rethrow exception");
        throw;
      }
      log_warn("address " << ipaddr << ':' << port << " in use - retry; n = " << n);
      ::sleep(1);
    }
  }
}

namespace tnt
{
  void ListenerBase::doStop()
  {
    log_warn("stop listener " << ipaddr << ':' << port);
    try
    {
      cxxtools::net::Stream(ipaddr, port);
    }
    catch (const std::exception& e)
    {
      log_error(e.what());
    }
  }

  int Listener::backlog = 16;
  unsigned Listener::listenRetry = 5;

  Listener::Listener(const std::string& ipaddr, unsigned short int port, Jobqueue& q)
    : ListenerBase(ipaddr, port),
      queue(q)
  {
    log_info("listen ip=" << ipaddr << " port=" << port);
    doListenRetry(server, ipaddr.c_str(), port);
  }

  void Listener::run()
  {
    // accept-loop
    log_debug("enter accept-loop");
    while (!Tntnet::shouldStop())
    {
      try
      {
        Tcpjob* j = new Tcpjob;
        Jobqueue::JobPtr p(j);
        j->accept(server);

        if (Tntnet::shouldStop())
          break;

        queue.put(p);
      }
      catch (const std::exception& e)
      {
        log_error("error in accept-loop: " << e.what());
      }
    }
  }

#ifdef USE_SSL
  Ssllistener::Ssllistener(const char* certificateFile,
      const char* keyFile,
      const std::string& ipaddr, unsigned short int port,
      Jobqueue& q)
    : ListenerBase(ipaddr, port),
      server(certificateFile, keyFile),
      queue(q)
  {
    log_info("listen ip=" << ipaddr << " port=" << port << " (ssl)");
    doListenRetry(server, ipaddr.c_str(), port);
  }

  void Ssllistener::run()
  {
    // accept-loop
    log_debug("enter accept-loop (ssl)");
    while (!Tntnet::shouldStop())
    {
      try
      {
        SslTcpjob* j = new SslTcpjob;
        Jobqueue::JobPtr p(j);
        j->accept(server);

        if (Tntnet::shouldStop())
          break;

        queue.put(p);
      }
      catch (const std::exception& e)
      {
        log_error("error in accept-loop: " << e.what());
      }
    }
  }

#endif // USE_SSL

}
