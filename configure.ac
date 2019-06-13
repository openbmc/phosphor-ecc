# Initialization
AC_PREREQ([2.69])
AC_INIT([phosphor-ecc], [1.0], [])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET


# Checks for libraries.
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus], [], [AC_MSG_ERROR(["sdbusplus required and not found."])])
PKG_CHECK_MODULES([SDEVENTPLUS], [sdeventplus], [], [AC_MSG_ERROR(["sdeventplus required and not found."])])
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces], [], [AC_MSG_ERROR(["phosphor-dbus-interfaces required and not found."])])
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging], [], [AC_MSG_ERROR(["phosphor-logging required and not found."])])

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-fpic -Wall -Werror], [CXXFLAGS])

# Checks for header files.
AC_CHECK_HEADER(experimental/filesystem, [], [AC_MSG_ERROR([Could not find experimental/filesystem...libstdc++fs development package required])])

# Checks for library functions.
LT_INIT

# Dbus service name
AC_ARG_VAR(BUSNAME, [The Dbus busname to own])
AS_IF([test "x$BUSNAME" == "x"], [BUSNAME="xyz.openbmc_project.memory.ECC"])
AC_DEFINE_UNQUOTED([BUSNAME], ["$BUSNAME"], [The Dbus busname to own])

# Service dbus root
AC_ARG_VAR(OBJROOT, [The Dbus object root path])
AS_IF([test "x$OBJROOT" == "x"], [OBJROOT="/xyz/openbmc_project/metrics/memory/"])
AC_DEFINE_UNQUOTED([OBJROOT], ["$OBJROOT"], [The Dbus object path])

# Service dbus root
AC_ARG_VAR(OBJPATH, [The Dbus object path])
AS_IF([test "x$OBJPATH" == "x"], [OBJPATH="/xyz/openbmc_project/metrics/memory/BmcECC"])
AC_DEFINE_UNQUOTED([OBJPATH], ["$OBJPATH"], [The Dbus object path])

# sysfs root path
AC_ARG_VAR(sysfsRootPath, [The sysfs root path])
AS_IF([test "x$sysfsRootPath" == "x"], [sysfsRootPath="/sys/devices/system/edac/mc/mc0/"])
AC_DEFINE_UNQUOTED([sysfsRootPath], ["$sysfsRootPath"], [The sysfs root path])

# The sysfs EDAC Report Path
AC_ARG_VAR(sysfsEDACReportPath, [The sysfs EDAC Report Path])
AS_IF([test "x$sysfsEDACReportPath" == "x"], [sysfsEDACReportPath="/sys/module/edac_core/parameters/edac_report"])
AC_DEFINE_UNQUOTED([sysfsEDACReportPath], ["$sysfsEDACReportPath"], [The sysfs EDAC Report Path])


# Create configured output
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
