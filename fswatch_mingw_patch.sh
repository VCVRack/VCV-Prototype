#!/bin/sh
# fix missing S_ISLNK in libfswatch/src/libfswatch/c++/poll_monitor.cpp
patch -ulbf libfswatch/src/libfswatch/c++/poll_monitor.cpp << EOF
@@ -131,2 +131,3 @@

+#ifndef _WIN32
     if (follow_symlinks && S_ISLNK(fd_stat.st_mode))
@@ -139,2 +140,3 @@
     }
+#endif

EOF
# fix missing realpath/lstat in libfswatch/src/libfswatch/c++/path_utils.cpp
patch -ulbf libfswatch/src/libfswatch/c++/path_utils.cpp << EOF
@@ -59,2 +59,5 @@
   {
+#ifdef _WIN32
+    return false;
+#else
     char *real_path = realpath(path.c_str(), nullptr);
@@ -66,2 +69,3 @@
     return ret;
+#endif
   }
@@ -82,2 +86,6 @@
   {
+#ifdef _WIN32
+    fsw_logf_perror(_("Cannot lstat %s (not implemented on Windows)"), path.c_str());
+    return false;
+#else
     if (lstat(path.c_str(), &fd_stat) != 0)
@@ -90,2 +98,3 @@
     return true;
+#endif
   }
EOF
# fix missing sigaction/realpath in fswatch/src/fswatch.cpp
patch -ulbf fswatch/src/fswatch.cpp << EOF
@@ -36,2 +36,5 @@
 #include "libfswatch/c++/libfswatch_exception.hpp"
+#ifdef _WIN32
+#define realpath(N,R) _fullpath((R),(N),_MAX_PATH)
+#endif

@@ -297,2 +300,3 @@
 {
+#ifndef _WIN32
   struct sigaction action;
@@ -328,2 +332,3 @@
   }
+#endif
 }
EOF
# fix libfswatch/src/libfswatch/c++/windows/win_paths.cpp
patch -ulbf libfswatch/src/libfswatch/c++/windows/win_paths.cpp << EOF
@@ -16,3 +16,7 @@
 #include "win_paths.hpp"
+#ifdef  __CYGWIN__
 #include <sys/cygwin.h>
+#else
+#include <windows.h>
+#endif
 #include "../libfswatch_exception.hpp"
@@ -28,2 +32,3 @@
     {
+#ifdef  __CYGWIN__
       void * raw_path = cygwin_create_path(CCP_POSIX_TO_WIN_W, path.c_str());
@@ -36,2 +41,11 @@
       return win_path;
+#else
+      int pathlen = (int)path.length() + 1;
+      int buflen = MultiByteToWideChar(CP_ACP, 0, path.c_str(), pathlen, 0, 0);
+      wchar_t* buf = new wchar_t[buflen];
+      MultiByteToWideChar(CP_ACP, 0, path.c_str(), pathlen, buf, buflen);
+      std::wstring result(buf);
+      delete[] buf;
+      return result;
+#endif
     }
@@ -40,2 +54,3 @@
     {
+#ifdef  __CYGWIN__
       void * raw_path = cygwin_create_path(CCP_WIN_W_TO_POSIX, path.c_str());
@@ -48,2 +63,11 @@
       return posix_path;
+#else
+      int pathlen = (int)path.length() + 1;
+      int buflen = WideCharToMultiByte(CP_ACP, 0, path.c_str(), pathlen, 0, 0, 0, 0);
+      char* buf = new char[buflen];
+      WideCharToMultiByte(CP_ACP, 0, path.c_str(), pathlen, buf, buflen, 0, 0);
+      std::string result(buf);
+      delete[] buf;
+      return result;
+#endif
     }
EOF
# fix missing file
touch README.illumos
# remove detection of realpath/regcomp/select
mv libfswatch/configure.ac libfswatch/configure.ac.bak &&
grep -v "realpath\|regcomp\|select" libfswatch/configure.ac.bak > libfswatch/configure.ac
# remove detection of realpath
mv configure.ac configure.ac.bak2 &&
grep -v "realpath" configure.ac.bak2 > configure.ac
# remove detection of realpath
mv configure.ac configure.ac.bak3 &&
grep -v "The select function cannot be found." configure.ac.bak3 > configure.ac
# fix for building windows_monitor
mv libfswatch/src/libfswatch/Makefile.am libfswatch/src/libfswatch/Makefile.am.bak &&
sed -e "s/USE_CYGWIN/USE_WINDOWS/" libfswatch/src/libfswatch/Makefile.am.bak > libfswatch/src/libfswatch/Makefile.am
