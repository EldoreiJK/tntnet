/* tnt/component.h
 * Copyright (C) 2003-2006 Tommi Maekitalo
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */



#ifndef TNT_COMPONENT_H
#define TNT_COMPONENT_H

#include <tnt/compident.h>
#include <time.h>

namespace cxxtools
{
  class QueryParams;
}

namespace tnt
{

class HttpRequest;
class HttpReply;

class Component
{
    time_t atime;
    bool top;

  public:
    Component() : atime(0), top(false)    { }
    virtual ~Component() { }

    virtual unsigned operator() (HttpRequest& request,
      HttpReply& reply, cxxtools::QueryParams& qparam);
    virtual unsigned endTag (HttpRequest& request,
      HttpReply& reply, cxxtools::QueryParams& qparam);

    virtual std::string getAttribute(const std::string& name,
      const std::string& def = std::string()) const;

    bool isTop() const           { return top; }
    void setTop(bool sw = true)  { top = sw; }

    /// explicitly call operator() - sometimes more readable
    unsigned call(HttpRequest& request, HttpReply& reply, cxxtools::QueryParams& qparam)
      { return operator() (request, reply, qparam); }
    /// call component without parameters
    unsigned call(HttpRequest& request, HttpReply& reply);

    /// return output as a string rather than outputting to stream
    std::string scall(HttpRequest& request, cxxtools::QueryParams& qparam);
    /// return output as a string rather than outputting to stream without query-parameters
    std::string scall(HttpRequest& request);
};

}

#endif // TNT_COMPONENT_H
