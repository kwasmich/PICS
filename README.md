# PICS #

**P**i **I**mproved **C**amera **S**treaming is a super lightweight hardware accelerated MJPEG Streaming Server for the Raspberry Pi.




## This Streaming Server combines the following: ##

* The output is a simple multipart MJPEG Stream that every web client supports.
* Lightweight HTTP server based on [tinyhttpd](https://sourceforge.net/projects/tinyhttpd/) from J. David.
* Camera capture with V4L2 to allow all UVC cameras to be used along with the official Raspberry Pi Camera.
* Support for multiple cameras and streams simultaneously.
* Using hardware accelerated JPEG compression with OpenMAX.




## Usage ##

Build and run PICS by issuing:

``` bash
sudo apt install libjpeg9-dev
make
./PICS
``` `

On startup it detects all video devices in `/dev/video*` (currently limited from 0 to 9).
PICS also provides a simple file server for static files that can be placed within the `htdocs` folder.
The content of the folder provides the content of the http server root.
To access a camera stream open the URL

    http://localhost:10000/dev/video0

where video0 can be replaced with video1 through video9 if present.




## To Do ##

Currently many things are hardcoded like the TCP port and the frame size. This will change in the future.

* Setting the frame rate.
* Stream does not work on Medion Smart TV.
* Compiling with and without OpenMAX.




## Color Formats ##

There is a mismatch in the color formats between V4L2 and OpenMAX on the Raspberry Pi.

The camera I am using returns [`V4L2_PIX_FMT_YUYV`](https://www.kernel.org/doc/html/v4.10/media/uapi/v4l/pixfmt-yuyv.html). Which is a YUV 4:2:2 format with one chroma value for two pixels.
The memory layout is:

    Y00 U00  Y01 V00  Y02 U02  Y03 V02 …
    Y10 U10  Y11 V10  Y12 U12  Y13 V12 …
    …
    Yn0 Un0  Yn1 Vn0  Yn2 Un2  Yn3 Vn2 …

    (Where the first number denotes the row and the second the column of the sampled chroma.)

The first to pixel share U0 and V0 while they have their own luminance Y. This continues through the end of the image.

The only corresponding color format in OpenMAX is `OMX_COLOR_FormatYCbYCr`. The documentation states:

>    16 bits per pixel YUV interleaved format organized as YUYV (i.e., YCbYCr).

But the implementation on the Raspberry Pi uses a different chroma subsampling. Instead of 4:2:2 it uses 4:2:0.
So chroma ist not only half the resolution horizontally but also vertically.
The odd thing however, is that the memory layout is somewhat special. The chroma is interleaved in the upper half of the
image while the lower half does not carry chroma information. Thus wasting memory. The memory layout looks like this:

    Y00 U00  Y01 V00  Y02 U01  Y03 V01 …
    Y10 U20  Y11 V20  Y12 U21  Y13 V21 …
    Y20 U40  Y21 V40  Y22 U41  Y23 V41 …
    Y30 U60  Y31 V60  Y32 U61  Y33 V61 …
    …
    Yi0 Um0  Yi1 Vm0  Yi2 Um1  Yi3 Vm1 …
    Yj0 ???  Yj1 ???  Yj2 ???  Yj3 ??? …
    Yk0 ???  Yk1 ???  Yk2 ???  Yk3 ??? …
    …
    Ym0 ???  Ym1 ???  Ym2 ???  Ym3 ??? …
    Yn0 ???  Yn1 ???  Yn2 ???  Yn3 ??? …

To be still able to use OpenMAX I had to write a method to "compress" the chroma vertically. This intoduces a higher
than intended CPU usage. Instead of ~11% CPU PICS consumes ~16% CPU while streaming and 0% while noone is watching.
This is still much lower than rougnly 90-100% CPU usage from other MJPEG streaming software.
