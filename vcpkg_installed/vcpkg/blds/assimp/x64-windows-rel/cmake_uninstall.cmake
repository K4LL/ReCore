IF(NOT EXISTS "C:/Users/gupue/source/repos/ReCore/vcpkg_installed/vcpkg/blds/assimp/x64-windows-rel/install_manifest.txt")
  MESSAGE(FATAL_ERROR "Cannot find install manifest: \"C:/Users/gupue/source/repos/ReCore/vcpkg_installed/vcpkg/blds/assimp/x64-windows-rel/install_manifest.txt\"")
ENDIF(NOT EXISTS "C:/Users/gupue/source/repos/ReCore/vcpkg_installed/vcpkg/blds/assimp/x64-windows-rel/install_manifest.txt")

FILE(READ "C:/Users/gupue/source/repos/ReCore/vcpkg_installed/vcpkg/blds/assimp/x64-windows-rel/install_manifest.txt" files)
STRING(REGEX REPLACE "\n" ";" files "${files}")
FOREACH(file ${files})
  MESSAGE(STATUS "Uninstalling \"$ENV{DESTDIR}${file}\"")
  EXEC_PROGRAM(
    "D:/VS/VS22IDE/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" ARGS "-E remove \"$ENV{DESTDIR}${file}\""
    OUTPUT_VARIABLE rm_out
    RETURN_VALUE rm_retval
    )
  IF(NOT "${rm_retval}" STREQUAL 0)
    MESSAGE(FATAL_ERROR "Problem when removing \"$ENV{DESTDIR}${file}\"")
  ENDIF(NOT "${rm_retval}" STREQUAL 0)
ENDFOREACH(file)
