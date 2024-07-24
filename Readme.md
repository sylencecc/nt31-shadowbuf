# shadowbuf display driver for NT 3.1

The framebuf display driver shipped with Windows NT 3.1 renders the screen into a linear framebuffer and can be used in tandem with a VBE-supporting miniport driver such as [vbemp](https://bearwindows.zcm.com.au/vbemp.htm) to show the desktop in high resolution and color depth on otherwise unsupported GPUs. However, rendering via the widely supported VESA BIOS Extensions 2.0 or 3.0 implies no hardware support for drawing operations. The result is sloppy performance when running large resolutions, which becomes particularly noticeable  during window scrolling. Scrolling involves lots of BitBlt (*bit block transfer*) operations being executed that copy chunks of memory from one place to another. Since reading from video memory is normally much slower than just writing to it, rendering performance degrades in proportion to the size of the scrolling area.

Shadowbuf is a modified version of the original Windows NT 3.1 framebuf display driver that does all rendering not only on the device's linear framebuffer, but also on another [shadow framebuffer](https://www.xfree86.org/4.1.0/apm5.html) in RAM. While this introduces a slight overhead to each drawing operation and increases memory usage, BitBlt operations can now use the shadow framebuffer as source instead of reading from the slower video memory. The improvement in rendering speed is only noticeable when running NT 3.1 on bare-metal hardware, since virtualized systems typically keep video memory in RAM and thus don't suffer from these differences in memory access performance.

The following screen grab shows the perceived performance improvement on a bare-metal system scrolling through some source code in a window of size 1024x768 on a desktop of size 1600x1200.

https://github.com/user-attachments/assets/98346414-5642-454f-9997-45dd04276303
