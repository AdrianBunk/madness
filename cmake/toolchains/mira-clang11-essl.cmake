# Set the system name so CMake uses the appropriate platform settings.
# NOTE: The platforms settings for BlueGeneP are the same for BlueGeneQ 
set(CMAKE_SYSTEM_NAME BlueGeneP-static)

# Set compilers
set(CMAKE_C_COMPILER bgclang)
set(CMAKE_CXX_COMPILER bgclang++11)
set(MPI_C_COMPILER mpicc)

# Set compile flags
set(CMAKE_C_FLAGS_INIT             "-std=c99" CACHE STRING "Inital C compile flags")
set(CMAKE_C_FLAGS_DEBUG            "-g -Wall" CACHE STRING "Inital C debug compile flags")
set(CMAKE_C_FLAGS_MINSIZEREL       "-Os -march=native -DNDEBUG" CACHE STRING "Inital C minimum size release compile flags")
set(CMAKE_C_FLAGS_RELEASE          "-O3 -march=native -DNDEBUG" CACHE STRING "Inital C release compile flags")
set(CMAKE_C_FLAGS_RELWITHDEBINFO   "-O2 -g -Wall" CACHE STRING "Inital C release with debug info compile flags")
set(CMAKE_CXX_FLAGS_INIT           "" CACHE STRING "Inital C++ compile flags")
set(CMAKE_CXX_FLAGS_DEBUG          "-g -Wall" CACHE STRING "Inital C++ debug compile flags")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -march=native -DNDEBUG" CACHE STRING "Inital C++ minimum size release compile flags")
set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -march=native -DNDEBUG" CACHE STRING "Inital C++ release compile flags")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -Wall" CACHE STRING "Inital C++ release with debug info compile flags")

# Set environment paths
set(CLANG_DIR  "/home/projects/llvm")
set(IBM_DIR    "$ENV{IBM_MAIN_DIR}")
set(XLF_DIR    "${IBM_DIR}/xlf/bg/14.1")
set(XLSMP_DIR  "${IBM_DIR}/xlsmp/bg/3.1")
set(ESSL_DIR   "/soft/libraries/essl/current/essl/5.1")
set(LAPACK_DIR "/soft/libraries/alcf/current/xl/LAPACK")

# V1R2M0
#set(MPI_DIR   "/bgsys/drivers/ppcfloor/comm/gcc")
#set(PAMI_DIR  "/bgsys/drivers/ppcfloor/comm/sys")
# V1R2M1
set(GCC_DIR    "/bgsys/drivers/ppcfloor/gnu-linux/powerpc64-bgq-linux")
set(MPI_DIR    "/bgsys/drivers/ppcfloor/comm")
set(PAMI_DIR   "/bgsys/drivers/ppcfloor/comm")
set(SPI_DIR    "/bgsys/drivers/ppcfloor/spi")

# Set library

set(XL_LIBRARIES ${XLSMP_DIR}/bglib64/libxlsmp.a)
set(XLF_LIBRARIES ${XLF_DIR}/bglib64/libxlf90_r.a;${XLF_DIR}/bglib64/libxlfmath.a;${XLF_DIR}/bglib64/libxlopt.a;${XLF_DIR}/bglib64/libxl.a)
set(BLAS_LIBRARY ${ESSL_DIR}/lib64/libesslbg.a)
set(BLAS_LIBRARIES ${BLAS_LIBRARY};${XLF_LIBRARIES};${XL_LIBRARIES})
set(LAPACK_LIBRARY ${LAPACK_DIR}/lib/liblapack.a)
set(LAPACK_LIBRARIES ${LAPACK_LIBRARY};${BLAS_LIBRARIES})
set(FORTRAN_INTEGER_SIZE "4" CACHE STRING "Set Fortran integer size in bytes")
