# Steganography
Hiding text inside PNG images

## Usage
### Dependencies
This project uses the libpng library 
http://libpng.org/pub/png/libpng.html

Compile using `make`.
### Hide text
`./esteganografo h source.png out.png "lorem ipsum"`
### Expose text
`./esteganografo u out.png`

Notice the `u` and the `h` in the first argument.



