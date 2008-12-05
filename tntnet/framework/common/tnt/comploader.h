/* tnt/comploader.h
 * Copyright (C) 2003-2005 Tommi Maekitalo
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


#ifndef TNT_COMPLOADER_H
#define TNT_COMPLOADER_H

#include <cxxtools/smartptr.h>
#include <tnt/urlmapper.h>
#include <tnt/langlib.h>
#include <map>
#include <list>
#include <string>
#include <utility>
#include <dlfcn.h>

namespace tnt
{
  class Comploader;
  class ComponentFactory;
  class Tntconfig;

  /// @internal
  class LibraryNotFound
  {
      std::string libname;
    public:
      explicit LibraryNotFound(const std::string& libname_)
        : libname(libname_)
        { }
      const std::string& getLibname() const  { return libname; }
  };

  template <typename objectType>
  class Dlcloser
  {
    protected:
      void destroy(objectType* ptr)
      { ::dlclose(*ptr); }
  };

  class ComponentLibrary
  {
      friend class Comploader;

      typedef void* HandleType;
      typedef cxxtools::SmartPtr<HandleType, cxxtools::RefLinked, Dlcloser> HandlePointer;
      HandlePointer handlePtr;

      typedef std::map<std::string, ComponentFactory*> factoryMapType;
      factoryMapType factoryMap;

      std::string libname;
      std::string path;
      typedef std::map<std::string, LangLib::PtrType> langlibsType;
      langlibsType langlibs;

      void* dlopen(const std::string& name);
      void init(const std::string& name);

    public:
      ComponentLibrary()
        { }

      ComponentLibrary(const std::string& path_, const std::string& name)
        : libname(name),
          path(path_)
        { init(path_ + '/' + name); }

      ComponentLibrary(const std::string& name)
        : libname(name)
        { init(name); }

      operator const void* () const  { return handlePtr.getPointer(); }

      Component* create(const std::string& component_name, Comploader& cl,
        const Urlmapper& rootmapper);
      LangLib::PtrType getLangLib(const std::string& lang);

      const std::string& getName() const  { return libname; }
      void registerFactory(const std::string& component_name, ComponentFactory* factory)
        { factoryMap.insert(factoryMapType::value_type(component_name, factory)); }
  };

  class Comploader
  {
      typedef std::map<std::string, ComponentLibrary> librarymap_type;
      typedef std::map<Compident, Component*> componentmap_type;
      typedef std::list<std::string> search_path_type;

      // loaded libraries
      static librarymap_type& getLibrarymap();

      // map soname/compname to compinstance
      componentmap_type componentmap;
      static const Tntconfig* config;
      static search_path_type search_path;
      static ComponentLibrary::factoryMapType* currentFactoryMap;

    public:
      Comploader();

      virtual Component& fetchComp(const Compident& compident,
        const Urlmapper& rootmapper);
      virtual Component* createComp(const Compident& compident,
        const Urlmapper& rootmapper);
      const char* getLangData(const Compident& compident, const std::string& lang);

      // lookup library; load if needed
      virtual ComponentLibrary& fetchLib(const std::string& libname);

      static const Tntconfig& getConfig()        { return *config; }
      static void configure(const Tntconfig& config);
      static void registerFactory(const std::string& component_name, ComponentFactory* factory);
      static void addSearchPathEntry(const std::string& path)
        { search_path.push_back(path); }
  };
}

#endif // TNT_COMPLOADER_H

