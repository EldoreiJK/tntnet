/* cookie.cpp
   Copyright (C) 2003-2005 Tommi Maekitalo

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

#include <tnt/cookie.h>
#include <tnt/http.h>
#include <cxxtools/log.h>
#include <iostream>
#include <stdexcept>

namespace tnt
{
  log_define("tntnet.cookie");

  const cookie cookies::empty_cookie;

  const std::string cookie::Comment = "Comment";
  const std::string cookie::Domain  = "Domain";
  const std::string cookie::MaxAge  = "Max-Age";
  const std::string cookie::Path    = "Path";
  const std::string cookie::Secure  = "Secure";
  const std::string cookie::Version = "Version";
  const std::string cookie::Expires = "Expires";

  void cookies::clearCookie(const std::string& name)
  {
    cookies_type::iterator it = data.find(name);
    if (it != data.end())
      it->second.setMaxAge("0");
    else
    {
      cookie c;
      c.setMaxAge("0");
      setCookie(name, c);
    }
  }

  void cookies::clearCookie(const std::string& name, const cookie& c)
  {
    cookie cc(c);
    cc.setMaxAge("0");
    setCookie(name, cc);
  }

  class cookie_parser
  {
      // Cookie: $Version="1"; Customer="WILE_E_COYOTE"; $Path="/acme"
      cookie::attrs_type common_attrs;
      cookie::attrs_type* current_attrs;
      cookie current_cookie;
      bool attr;
      std::string current_cookie_name;

      std::string name;
      std::string value;

      cookies& mycookies;

      void store_cookie();
      void process_nv();

    public:
      cookie_parser(cookies& c)
        : current_attrs(&common_attrs),
          mycookies(c)
        { }

      void parse(const std::string& header);
  };

  void cookies::set(const std::string& header)
  {
    log_debug("parse cookies: " << header);
    cookie_parser parser(*this);
    parser.parse(header);
  }

  void cookie_parser::store_cookie()
  {
    if (!mycookies.hasCookie(current_cookie_name))
      mycookies.setCookie(current_cookie_name, current_cookie);
    current_cookie.value.clear();
  }

  void cookie_parser::process_nv()
  {
    if (attr)
    {
      log_debug("attribute: " << name << '=' << value);
      current_attrs->insert(
        cookie::attrs_type::value_type(name, value));
    }
    else
    {
      if (!current_cookie_name.empty())
        store_cookie();

      log_debug("cookie: " << name << '=' << value);

      current_cookie_name = name;
      current_cookie.value = value;
      name.clear();
      current_attrs = &current_cookie.attrs;
      current_cookie.attrs = common_attrs;
    }
  }

  void cookie_parser::parse(const std::string& header)
  {
    // Cookie: $Version="1"; Customer="WILE_E_COYOTE"; $Path="/acme"

    enum state_type
    {
      state_0,
      state_name,
      state_eq,
      state_value0,
      state_value,
      state_valuee,
      state_qvalue,
      state_qvaluee
    };

    state_type state = state_0;

    for (std::string::const_iterator it = header.begin();
         it != header.end(); ++it)
    {
      char ch = *it;
      switch(state)
      {
        case state_0:
          if (ch == '$')
          {
            attr = true;
            name.clear();
            state = state_name;
          }
          else if (!std::isspace(ch))
          {
            attr = false;
            name = ch;
            state = state_name;
          }
          break;

        case state_name:
          if (std::isspace(ch))
            state = state_eq;
          else if (ch == '=')
            state = state_value0;
          else
            name += ch;
          break;

        case state_eq:
          if (ch == '=')
            state = state_value0;
          else if (!std::isspace(ch))
            throw httpError("400 invalid cookie: " + header);
          break;

        case state_value0:
          if (ch == '"')
          {
            value.clear();
            state = state_qvalue;
          }
          else if (!std::isspace(ch))
          {
            value = ch;
            state = state_value;
          }
          break;

        case state_value:
          if (ch == ';' || ch == ',')
          {
            process_nv();
            state = state_0;
          }
          else if (std::isspace(ch))
            state = state_valuee;
          else
            value += ch;
          break;

        case state_valuee:
          if (ch == ';' || ch == ',')
          {
            process_nv();
            state = state_0;
          }
          else if (std::isspace(ch))
            state = state_valuee;
          else
            throw httpError("400 invalid cookie: " + header);
          break;

        case state_qvalue:
          if (ch == '"')
            state = state_qvaluee;
          else
            value += ch;
          break;

        case state_qvaluee:
          if (ch == ';' || ch == ',')
          {
            process_nv();
            state = state_0;
          }
          else if (!std::isspace(ch))
            throw httpError("400 invalid cookie: " + header);
          break;
      }
    }

    if (state == state_qvaluee || state == state_value)
      process_nv();
    else if (state != state_0)
      throw httpError("400 invalid cookie: " + header);

    if (!current_cookie.value.empty())
      store_cookie();
  }

  std::ostream& operator<< (std::ostream& out, const cookies& c)
  {
    // Set-Cookie: Customer="WILE_E_COYOTE"; Version="1"; Path="/acme"

    bool first = true;
    for (cookies::cookies_type::const_iterator it = c.data.begin();
         it != c.data.end(); ++it)
    {
      if (first)
        first = false;
      else
        out << ", ";

      const cookie& cookie = it->second;

      // print name (Customer="WILE_E_COYOTE")
      out << it->first << "=\"" << cookie.getValue() << '"';

      // print attributes
      for (cookie::attrs_type::const_iterator a = cookie.attrs.begin();
           a != cookie.attrs.end(); ++a)
        out << "; " << a->first << "=\"" << a->second << '"';
    }

    return out;
  }
}