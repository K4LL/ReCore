IF(NOT EXISTS "C:/Users/gupue/source/repos/ReCore/vcpkg_installed/x64-windows/vcpkg/blds/assimp/x64-windows-rel/install_manifest.txt")
  MESSAGE(FATAL_ERROR "Cannot find install manifest: \"C:/Users/gupue/source/repos/ReCore/vcpkg_installed/x64-windows/vcpkg/blds/assimp/x64-windows-rel/install_manifest.txt\"")
ENDIF(NOT EXISTS "C:/Users/gupue/source/repos/ReCore/vcpkg_installed/x64-windows/vcpkg/blds/assimp/x64-windows-rel/install_manifest.txt")

FILE(READ "C:/Users/gupue/source/repos/ReCore/vcpkg_installed/x64-windows/vcpkg/blds/assimp/x64-windows-rel/install_manifest.txt" files)
STRING(REGEX REPLACE "\n" ";" files "${files}")
FOREACH(file ${files})
  MESSAGE(STATUS "Uninstalling \"$ENV{DESTDIR}${file}\"")
  EXEC_PROGRAM(
    "C:/Users/gupue/AppData/Local/vcpkg/downloads/tools/cmake-3.29.2-windows/cmake-3.29.2-windows-i386/bin/cmake.exe" ARGS "-E remove \"$ENV{DESTDIR}${file}\""
    OUTPUT_VARIABLE rm_out
    RETURN_VALUE rm_retval
    )
  IF(NOT "${rm_retval}" STREQUAL 0)
    MESSAGE(FATAL_ERROR "Problem when removing \"$ENV{DESTDIR}${file}\"")
  ENDIF(NOT "${rm_retval}" STREQUAL 0)
ENDFOREACH(file)
