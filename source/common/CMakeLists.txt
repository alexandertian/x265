# vim: syntax=cmake
set(LIBCOMMON_SRC
    ../Lib/TLibCommon/TComBitStream.cpp
    ../Lib/TLibCommon/TComDataCU.cpp
    ../Lib/TLibCommon/TComLoopFilter.cpp
    ../Lib/TLibCommon/TComMotionInfo.cpp
    ../Lib/TLibCommon/TComPattern.cpp
    ../Lib/TLibCommon/TComPic.cpp
    ../Lib/TLibCommon/TComPicSym.cpp
    ../Lib/TLibCommon/TComPicYuv.cpp
    ../Lib/TLibCommon/TComPicYuvMD5.cpp
    ../Lib/TLibCommon/TComPrediction.cpp
    ../Lib/TLibCommon/TComRom.cpp
    ../Lib/TLibCommon/TComSampleAdaptiveOffset.cpp
    ../Lib/TLibCommon/TComSlice.cpp
    ../Lib/TLibCommon/TComTrQuant.cpp
    ../Lib/TLibCommon/TComWeightPrediction.cpp
    ../Lib/TLibCommon/TComYuv.cpp)
set(LIBCOMMON_HDR
    ../Lib/TLibCommon/SEI.h
    ../Lib/TLibCommon/CommonDef.h
    ../Lib/TLibCommon/TypeDef.h
    ../Lib/TLibCommon/ContextTables.h
    ../Lib/TLibCommon/TComBitStream.h
    ../Lib/TLibCommon/TComDataCU.h
    ../Lib/TLibCommon/TComLoopFilter.h
    ../Lib/TLibCommon/TComMotionInfo.h
    ../Lib/TLibCommon/TComPattern.h
    ../Lib/TLibCommon/TComPic.h
    ../Lib/TLibCommon/TComPicSym.h
    ../Lib/TLibCommon/TComPicYuv.h
    ../Lib/TLibCommon/TComPrediction.h
    ../Lib/TLibCommon/TComRom.h
    ../Lib/TLibCommon/TComSampleAdaptiveOffset.h
    ../Lib/TLibCommon/TComSlice.h
    ../Lib/TLibCommon/TComTrQuant.h
    ../Lib/TLibCommon/TComWeightPrediction.h
    ../Lib/TLibCommon/TComYuv.h)

source_group(TLibCommon FILES ${LIBCOMMON_SRC} ${LIBCOMMON_HDR})
if(GCC)
    set_source_files_properties(${LIBCOMMON_SRC} PROPERTIES COMPILE_FLAGS 
        "-Wno-sign-compare")
endif(GCC)
if(MSVC)
    # ignore these warnings from HM source
    # /wd4244 type conversion, possible loss of data
    # /wd4389 signed/unsigned mismatch
    # /wd4018 '<' signed/unsigned mismatch
    set_source_files_properties(${LIBCOMMON_SRC} PROPERTIES COMPILE_FLAGS 
        "/wd4244 /wd4389 /wd4018")
    if(INTEL_CXX)
        add_definitions(/Qwd2557) # signed/unsigned mismatch
    endif()
endif(MSVC)

set(SSE3  vec/dct-sse3.cpp  vec/blockcopy-sse3.cpp)
set(SSSE3 vec/dct-ssse3.cpp)
set(SSE41 vec/dct-sse41.cpp)

if(MSVC AND X86)
    set(PRIMITIVES ${SSE3} ${SSSE3} ${SSE41})
    set(WARNDISABLE "/wd4100") # unreferenced formal parameter
    if(INTEL_CXX)
        add_definitions(/Qwd111) # statement is unreachable
        add_definitions(/Qwd128) # loop is unreachable
        add_definitions(/Qwd177) # declared function is unused
        add_definitions(/Qwd185) # dynamic initialization in unreachable code
        add_definitions(/Qwd280) # conditional expression is constant
    endif()
    if(X64)
        set_source_files_properties(${SSE3} ${SSSE3} ${SSE41} PROPERTIES COMPILE_FLAGS "${WARNDISABLE}")
    else()
        # x64 implies SSE4, so only add /arch:SSE2 if building for Win32
        set_source_files_properties(${SSE3} ${SSSE3} ${SSE41} PROPERTIES COMPILE_FLAGS "${WARNDISABLE} /arch:SSE2")
    endif()
endif()
if(GCC AND X86)
    if(CLANG)
        # llvm intrinsic headers cause shadow warnings
        set(WARNDISABLE "-Wno-shadow -Wno-unused-parameter")
    else()
        set(WARNDISABLE "-Wno-unused-parameter")
    endif()
    if(INTEL_CXX OR CLANG OR (NOT CC_VERSION VERSION_LESS 4.3))
        set(PRIMITIVES ${SSE3} ${SSSE3} ${SSE41})
        set_source_files_properties(${SSE3}  PROPERTIES COMPILE_FLAGS "${WARNDISABLE} -msse3")
        set_source_files_properties(${SSSE3} PROPERTIES COMPILE_FLAGS "${WARNDISABLE} -mssse3")
        set_source_files_properties(${SSE41} PROPERTIES COMPILE_FLAGS "${WARNDISABLE} -msse4.1")
    endif()
endif()
set(VEC_PRIMITIVES vec/vec-primitives.cpp ${PRIMITIVES})
source_group(Intrinsics FILES ${VEC_PRIMITIVES})

if(ENABLE_ASSEMBLY)
    set_source_files_properties(primitives.cpp PROPERTIES COMPILE_FLAGS -DENABLE_ASSEMBLY=1)
    set(C_SRCS asm-primitives.cpp pixel.h mc.h ipfilter8.h blockcopy8.h dct8.h loopfilter.h)
    set(A_SRCS pixel-a.asm const-a.asm cpu-a.asm ssd-a.asm mc-a.asm
               mc-a2.asm pixel-util8.asm blockcopy8.asm
               pixeladd8.asm dct8.asm)
    if(HIGH_BIT_DEPTH)
        set(A_SRCS ${A_SRCS} sad16-a.asm intrapred16.asm ipfilter16.asm)
    else()
        set(A_SRCS ${A_SRCS} sad-a.asm intrapred8.asm ipfilter8.asm loopfilter.asm)
    endif()

    if(NOT X64)
        set(A_SRCS ${A_SRCS} pixel-32.asm)
    endif()

    if(MSVC_IDE OR XCODE)
        # MSVC requires custom build rules in the main cmake script for yasm
        set(MSVC_ASMS "${A_SRCS}" CACHE INTERNAL "yasm sources")
        set(A_SRCS)
    endif()

    enable_language(ASM_YASM)

    foreach(SRC ${A_SRCS} ${C_SRCS})
        set(ASM_PRIMITIVES ${ASM_PRIMITIVES} x86/${SRC})
    endforeach()
    source_group(Assembly FILES ${ASM_PRIMITIVES})
endif(ENABLE_ASSEMBLY)

check_symbol_exists(strtok_r "string.h" HAVE_STRTOK_R)
if(HAVE_STRTOK_R)
    set_source_files_properties(param.cpp PROPERTIES COMPILE_FLAGS -DHAVE_STRTOK_R=1)
endif()

if(GCC AND CC_HAS_NO_NARROWING)
    set_source_files_properties(cpu.cpp PROPERTIES COMPILE_FLAGS -Wno-narrowing)
endif()
if(WIN32)
    set(WINXP winxp.h winxp.cpp)
endif(WIN32)

set_source_files_properties(version.cpp PROPERTIES COMPILE_FLAGS -DX265_VERSION=${X265_VERSION})

add_library(common OBJECT
    ${ASM_PRIMITIVES} ${VEC_PRIMITIVES}
    ${LIBCOMMON_SRC} ${LIBCOMMON_HDR} ${WINXP}
    primitives.cpp primitives.h
    pixel.cpp dct.cpp ipfilter.cpp intrapred.cpp
    cpu.cpp cpu.h version.cpp
    threading.cpp threading.h
    threadpool.cpp threadpool.h
    wavefront.h wavefront.cpp
    md5.cpp md5.h
    bitstream.h mv.h
    shortyuv.cpp shortyuv.h
    common.cpp common.h
    param.cpp param.h
    lowres.cpp lowres.h
    piclist.cpp piclist.h
    loopfilter.cpp)
