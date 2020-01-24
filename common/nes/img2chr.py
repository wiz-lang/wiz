#!/bin/env python
import os
import os.path
import PIL.Image

def write_chr(w, h, data, f):
    for y in range(0, h, 8):
        for x in range(0, w, 8):
            # Copy low bits of each 8x8 chunk into the first 8x8 plane.
            for j in range(8):
                c = 0
                for i in range(8):
                    c = (c * 2) | (data[x + i, y + j] & 1)
                f.write(chr(c))
            # Copy high bits of each chunk into the second 8x8 plane.
            for j in range(8):
                c = 0
                for i in range(8):
                    c = (c * 2) | ((data[x + i, y + j] >> 1) & 1)
                f.write(chr(c))

if __name__ == '__main__':
    import sys
    
    def main():
        if len(sys.argv) > 1:
                for arg in range(1, len(sys.argv)):
                    filename = sys.argv[arg]
                    
                    try:
                        img = PIL.Image.open(filename)
                    except IOError as e:
                        if os.path.isdir(filename):
                            exit(filename + ' is a directory.')
                        if os.path.exists(filename):
                            exit(filename + ' has an unsupported filetype, or you lack permission to open it.')
                        else:
                            exit('File ' + filename + ' does not exist!')
                        
                    w, h = img.size
                    if w != 128 or h != 128:
                        exit('Image ' + filename + ' is not 128x128 pixels in size.')
                    if not img.palette:
                        exit('Image ' + filename + ' has no palette.')
                    data = img.load()
                    
                    save_filename = os.path.splitext(filename)[0] + '.chr'
                    try:
                        f = open(save_filename, 'wb')
                    except Exception as e:
                        exit('Failure attempting to write ' + save_filename)
                    
                    write_chr(w, h, data, f)
                    f.close()
                    print('  ' + filename + ' -> ' + save_filename)
        else:
            print('Usage: ' + sys.argv[0] + ' file [file...]')
            print('Converts files like foo.png into NES-friendly formats like foo.chr')
    main()