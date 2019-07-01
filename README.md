# headstripper

headstripper removes EXIF and other info from image files. Multiple images can be supplied as arguments.

Supported Filetypes:

* JPG (JFIF)
* PNG

## How it works

JFIF and PNG containers use chunks to store different types of data (eg. Image data, Colorcorrection, Copyright, EXIF, notes, etc.). Headstripper uses a chunk-whitelist to only copy data needed to display the image. Any unknown chunks are discarded.  

Some image editing software uses custom chunks to store color correction data and more in PNG chunks. This data is also removed (as it is not strictly required) and thus may result in incorrect images.

License: MIT
