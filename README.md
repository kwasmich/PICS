# PICS #

**P**i **I**mproved **C**amera **S**treaming is a super lightweight hardware accelerated MJPEG Streaming Server for the Raspberry Pi.

On a Raspberry Pi 1 Model B running Raspberry Pi OS 11 Bullseye PICS consumes ~16% CPU while streaming and 0% while noone is watching.
This is much lower than rougnly 90-100% CPU usage from other MJPEG streaming software.



## This Streaming Server combines the following: ##

* The output is a simple multipart MJPEG Stream that every web client supports.
* Lightweight HTTP server based on [tinyhttpd](https://sourceforge.net/projects/tinyhttpd/) from J. David.
* Camera capture with V4L2 to allow all UVC cameras to be used along with the official Raspberry Pi Camera.
* Support for multiple cameras and streams simultaneously.
* Using hardware accelerated JPEG compression.




## Usage ##

Build and run PICS by issuing:

``` bash
sudo apt install libjpeg9-dev
make
./PICS
```

On startup it detects all video devices in `/dev/video*` (currently limited from 0 to 19).
PICS also provides a simple file server for static files that can be placed within the `htdocs` folder.
The content of the folder provides the content of the http server root.
To access a camera stream open the URL

    http://localhost:8080/dev/video0
    
or if you are interested in one single image

    http://localhost:8080/dev/still0

where video0 can be replaced with video1 through video9 if present.



### Using the official Raspberry Pi Camera ###


As of Raspberry Pi OS 11 (Bullseye) you need to enable **Legacy Camera** in the Interface Options in  `raspi-config`.
This sets the GPU memory to 128 MB but at least for the Raspberry Pi Camera 1, 64 MB are sufficient.

Support for the new native libcamera API may be added in the future.


If you are using Raspberri Pi OS 10 (Buster) or older, you may want to use the using the _OMX_ branch.
This is because of the breaking changes that have been introduced with Bullseye such as removal of OpenMAX.




## To Do ##

Currently many things are hardcoded like the TCP port and the frame size. This will change in the future.

* Setting the image size and frame rate.
* Stream does not work on Medion Smart TV.
* Make it more flexible again to support cameras other than Raspberry Pi Camera 1.
* Adopt the new libcamera API
