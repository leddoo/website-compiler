@echo off

if not exist "build" (mkdir "build")
pushd "build"

set allow_unnamed_struct=/wd4201
set allow_deleted_assignment=/wd4626
set allow_shadow=/wd4456
set allow_relative_path_up=/wd4464
set no_padding_info=/wd4820
set no_align_padding_info=/wd4324
set no_spectre_info=/wd5045
set no_inline_info=/wd4710 /wd4711 /wd4514
set non_constant_initializers=/wd4204
set other_warnings=/wd4582 /wd4577 /wd4668

set warnings=/Wall %allow_unnamed_struct% %allow_deleted_assignment%^
    %allow_shadow% %allow_relative_path_up% %no_padding_info%^
    %no_align_padding_info% %no_spectre_info% %no_inline_info%^
    %non_constant_initializers%^
    %other_warnings%

set compile_flags_common=/nologo /MP /GL %warnings%

set compile_flags_debug=/Zi /Od %compile_flags_common%
set compile_flags_release=/O2 %compile_flags_common%
set compile_flags_release_info=/Zi /O2 %compile_flags_common%
set link_flags=/SUBSYSTEM:CONSOLE /LTCG

set LIBCPP_ROOT=..\code

set LIBCPP_SOURCES=^
    %LIBCPP_ROOT%\libcpp\memory\allocator.cpp^
    %LIBCPP_ROOT%\libcpp\memory\arena.cpp^
    %LIBCPP_ROOT%\libcpp\memory\hash.cpp^
    %LIBCPP_ROOT%\libcpp\memory\heap.cpp^
    %LIBCPP_ROOT%\libcpp\util\assert.cpp^
    %LIBCPP_ROOT%\libcpp\util\assert_win32.cpp

set libcpp=/I%LIBCPP_ROOT%

cl ..\examples\examples.cpp %LIBCPP_SOURCES% User32.lib /I..\examples %libcpp% %compile_flags_debug% /link %link_flags% /out:main.exe

popd
