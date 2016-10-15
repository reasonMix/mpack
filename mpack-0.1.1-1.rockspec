package = "mpack"
version = "0.1.1-1"

source = {
  url = "git://github.com/reasonMix/mpack.git",
  tag = "v0.1.1"
}
description={
   summary = 'a star algorithm',
   detailed = 'a star algorithm',
   homepage = "https://github.com/reasonMix/mpack",
   license = "The MIT License"
}
dependencies = { "lua >= 5.1" }
build = {
  type = 'cmake',
  platforms = {
     windows = {
        variables = {
           LUA_LIBRARIES = '$(LUA_LIBDIR)/$(LUALIB)'
        }
     }
  },
  variables = {
    BUILD_SHARED_LIBS = 'ON',
    CMAKE_INSTALL_PREFIX = '$(PREFIX)',
    LUA_INCLUDE_DIR = '$(LUA_INCDIR)',
  }
}
