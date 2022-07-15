# Windows driver for RAPL metrics gathering

## Installation

TODO in debug mode

## Cross compilation MinGW

Dependencies:
- x86_64-w64-mingw32-g++

Compilation:
x86_64-w64-mingw32-g++ DriverLoader.cpp -o DriverLoader.exe

## Compilation CPUID

Code getting CPU manufacturer's informations (through CPUID) uses inlined assembly.

To compile on Linux, we need to tell GCC to use the Intel syntax and not the At&T one. We also need to specify we don't want an axecutable with independant position.

Example : gcc -masm=intel -no-pie cpuid.c -o cpuid

To compile on Windows, we can inline 32 bit assembly but not 64 bit (MSVC compiler doesn't support it).

To fix this, the __cpuid__ function is written in pure assembly matching the Windows ABI (x86 and x86_64). To compile, use those commands :
 - x86 : nasm -f win32 cpuid_x86.nasm -o cpuid_x86.obj
 - x86_64 : nasm -f win64 cpuid_x86_64.nasm -o cpuid_x86_64.obj

To compile the agent, including the __cpuid__ function, do it according to the architecture :
 - x86_64 : x86_64-w64-mingw32-g++ RAPLAgent.cpp -s cpuid_x86_64.obj -o RAPLAgent.exe

## Q & A

## External documentation we used

- https://github.com/intel/powergov
- https://community.intel.com/t5/Software-Tuning-Performance/RAPL-for-energy-measurement/td-p/919723
- www.eecs.berkeley.edu/Pubs/TechRpts/2012/EECS-2012-168.pdf
- https://github.com/kentcz/rapl-tools
- https://01.org/blogs/2014/running-average-power-limit-%E2%80%93-rapl
- https://github.com/energymon/energymon/blob/master/msr/energymon-msr.c
- https://github.com/gz/rust-x86
- https://docs.rs/x86/0.43.0/x86/
- https://github.com/gz/rust-x86/tree/master/x86test
- https://docs.rs/x86/0.43.0/x86/msr/constant.MSR_RAPL_POWER_UNIT.html
- https://github.com/insula-rs/libwhp/tree/master/src
- https://crates.io/crates/register/1.0.2
- https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/rdmsr--read-msr-
- https://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-vol-3b-part-2-manual.html
- https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
- https://software.intel.com/content/www/us/en/develop/download/intel-64-and-ia-32-architectures-software-developers-manual-volume-1-basic-architecture.html
- https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Register/Intel/ArchitecturalMsr.h
- https://github.com/tianocore/edk2/tree/master/MdePkg/Include/Register/Intel/Msr
- https://github.com/lizhuohua/linux-kernel-module-rust
- https://docs.rs/x86test-types/0.0.3/x86test_types/macro.kassert.html
- https://doc.rust-lang.org/unstable-book/library-features/llvm-asm.html
- https://elixir.bootlin.com/linux/latest/source/drivers/powercap
- http://icl.cs.utk.edu/papi/docs/d4/d46/linux-powercap_8c_source.html
- https://www.osr.com/getting-started-writing-windows-drivers/
- http://woshub.com/how-to-sign-an-unsigned-driver-for-windows-7-x64/
- https://www.electronicdesign.com/technologies/windows/article/21802026/how-to-write-windows-drivers
- https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile
- https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-writefile
- https://docs.microsoft.com/en-us/windows/win32/sync/synchronization-and-overlapped-input-and-output
- https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea
- https://docs.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/writing-a-very-small-kmdf--driver
- https://docs.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/writing-your-first-driver
- https://www.intel.com/content/www/us/en/developer/articles/tool/power-gadget.html#inpage-nav-12
- https://github.com/Microsoft/Windows-driver-samples
- https://www.osronline.com/article.cfm%5Earticle=229.htm
- https://fr.adasbcc.org/driver-blocked-from-loading-fix-3085
- https://www.codeproject.com/articles/13090/building-and-deploying-a-basic-wdf-kernel-mode-dri
- https://www.digicert.com/kb/code-signing/signcode-signtool-command-line.htm
- https://www.techpout.com/update-drivers-using-command-prompt-in-windows-10/

Fichiers intéressants dans le noyau linux :
- drivers/powercap/intel_rapl_msr.c
- arch/x86:lib/msr_smp.c
