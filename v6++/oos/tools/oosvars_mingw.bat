@set oos_path=D:\data\study data\v6++\oos
@set mingw_path=D:\data\study data\v6++\MinGW\bin
@set nasm_path=D:\data\study data\v6++\NASM
@set bochs_path=D:\data\study data\v6++\Bochs-2.4.2
@set BXSHARE=%bochs_path%
@set partcopy_path=%oos_path%\tools\partcopy

@set path=%partcopy_path%;%bochs_path%;%nasm_path%;%mingw_path%;%oos_path%\tools\;%path%

@cls
@echo Setting develop and build environment for UnixV6++.