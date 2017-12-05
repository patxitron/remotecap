# REMOTECAP

Image capture application for ASI1600MM-C camera and EFW mini filter wheel, both
from ZWO. It is written as a websockets service to be controlled in a browser
window.

## Dependencies

In order to compile this app, it is necessary the ZWO development kit for ASI
cameras and for filter wheels (EFW). Download them from zwo site and unpack them
in their respective folder under zwo.

It is included as dependency (and cloned automatically) [boost/beast](https://github.com/boostorg/beast),
a header-only http/websocket infraestructure based on boost/asio.

Another included dependency is [Json for modern C++](https://github.com/nlohmann/json/),
a header-only json library.

This app depends on several boost libraries (asio, system, filesystem and regex),
libudev, libusb, cfitsio and CCfits.

## Websocket API

Communication through websockets are json objects and binary images.

On connection, the client receive a message with the configuration and status
objects.

When an image is captured, all clients connected receive a newimage object.

At any time any client can request a preview of the image sending a get_preview
object or the complete "raw" image sending a get_raw object.

Both the preview and raw images are sent to the clients in a binary message.

__TO DO:__ Define protocol and write documentation.

## Sample program which captures and saves a ".fits" file

There is included a little independient program (prueba.cpp) that gets some
data from camera, then captures an image and saves it as a fits file. It is
working and serve as proof of concept. You can compile it by typing

```
make prueba
```
