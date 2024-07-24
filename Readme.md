# shadowbuf display driver for NT 3.1

The framebuf display driver shipped with Windows NT 3.1 renders the screen onto a linear framebuffer and can be used in tandem with a VBE-supporting miniport driver such as [vbemp](https://bearwindows.zcm.com.au/vbemp.htm) to show the desktop in high resolution and color depth on otherwise unsupported GPUs. However, rendering via the widely supported VESA BIOS Extensions 2.0 or 3.0 implies no hardware support for drawing operations. The result is sloppy performance when running large resolutions, which becomes particularly noticeable  during window scrolling. Scrolling involves lots of BitBlt (*bit block transfer*) operations being executed that copy chunks of memory from one place to another. Since reading from video memory is normally much slower than just writing to it, rendering performance degrades in proportion to the size of the scrolling area.

Shadowbuf is a modified version of the original Windows NT 3.1 framebuf display driver that does all rendering not only on the device's linear framebuffer, but also on another [shadow framebuffer](https://www.xfree86.org/4.1.0/apm5.html) in RAM. While this introduces a slight overhead to each drawing operation and increases memory usage, BitBlt operations can now use the shadow framebuffer as source instead of reading from the slower video memory. The improvement in rendering speed is only noticeable when running NT 3.1 on bare-metal hardware, since virtualized systems typically keep video memory in RAM and thus don't suffer from these differences in memory access performance.

The following screen grab shows the perceived performance improvement on a bare-metal system scrolling through some source code in a window of size 1024x768px on a desktop of size 1600x1200px.

https://github.com/user-attachments/assets/98346414-5642-454f-9997-45dd04276303

Further insignificant changes to the stock framebuf driver as shipped with the NT 3.1 DDK are
* MIPS support removed, to streamline the code and due to a lack of testing
* Experimental support for DFB/DDB (Device Dependent Bitmaps) removed, which is disabled in the release version anyway
* Minimal performance improvement in case there is no hardware pointer support

## Usage
Shadowbuf is a drop-in replacement for the stock framebuf driver shipped with NT 3.1. This way, all miniport drivers that depend on framebuf can also be used with shadowbuf. It is recommended to create a backup of the stock driver at `%windir%\system32\framebuf.dll`.

To install the shadowbuf driver, either download prebuilt binaries from the [Releases](https://github.com/sylencecc/nt31-shadowbuf/releases) section or build it yourself by following the instructions below. Move the resulting `framebuf.dll` to `%windir%\system32\framebuf.dll`, then switch to a display driver that supports framebuf in *Main* -> *Windows NT Setup* and reboot.

If you're already running a display driver that depends on the stock `framebuf.dll`, overwriting the existing driver is impossible due to it being currently in use. In that case,  Possible workarounds are to
* switch to another display driver such as *Standard VGA*, reboot NT, overwrite `framebuf.dll`, then switch back to the intended driver.
* boot another OS such as DOS or any Linux and overwrite `framebuf.dll` from there.

## Build
With the Windows NT 3.1 DDK installed, launch either the *Free* or *Checked Build Environment* (for debug builds) from *Program Manager*. NT 3.1 has issues with recognizing CPUs newer than the i586. In case `Error: PROCESSOR_ARCHITECTURE environment variable not recognized.` is shown, either set that env var to `x86` globally or use a batch file such as
```
set PROCESSOR_ARCHITECTURE=x86
set NTSDK=1
c:\ddk\bin\setenv.bat c:\ddk free
```
Setting `NTSDK` fixes additional build issues in case the NT 3.1 SDK is installed simultaneously.

Navigate to the shadowbuf sources and execute `build`. The resulting binary will be written to the DDK directory, e.g. to `c:\ddk\lib\i386\free\framebuf.dll`.
