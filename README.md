# PICS #
Pi Improved Camera Streaming is a super lightweight hardware accelerated MJPEG Streaming Server for the Raspberry Pi.


## This Streaming Server combines the following: ##

* The output is a simple multipart MJPEG Stream that every web client supports.
* Lightweight HTTP server based on [tinyhttpd](https://sourceforge.net/projects/tinyhttpd/) from J. David.
* Camera capture with V4L2 to allow all UVC cameras to be used along with the official Raspberry Pi Camera.
* Using hardware accelerated JPEG compression with OpenMAX.
