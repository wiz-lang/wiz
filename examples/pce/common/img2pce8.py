import os
import os.path
import PIL.Image

def write_chr(w, h, data, f):
    for y in range(0, h, 8):
        for x in range(0, w, 8):
            for j in range(8):
                # Write low bits of this row.
                c = 0
                for i in range(8):
                    c = (c * 2) | (data[x + i, y + j] & 1)
                f.write(bytearray([c]))
                
                # Write second bits of this row.
                c = 0
                for i in range(8):
                    c = (c * 2) | ((data[x + i, y + j] >> 1) & 1)
                f.write(bytearray([c]))                
            for j in range(8):            
                # Write third bits of this row.
                c = 0
                for i in range(8):
                    c = (c * 2) | ((data[x + i, y + j] >> 2) & 1)
                f.write(bytearray([c]))

                # Write high bits of this row.
                c = 0
                for i in range(8):
                    c = (c * 2) | ((data[x + i, y + j] >> 3) & 1)
                f.write(bytearray([c]))

if __name__ == '__main__':
    import sys

    if len(sys.argv) > 1:
        WIDTH = 128
        HEIGHT = None
        for arg in range(1, len(sys.argv)):
            filename = sys.argv[arg]
            
            if filename[0] == '-':
                if filename == '-oldfart':
                    HEIGHT = 192
                elif filename == '-newfart':
                    HEIGHT = None
                else:
                    exit('Invalid argument `' + filename + '`.')
                continue
            
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
            if w != WIDTH or h % 8 != 0 or HEIGHT and h != HEIGHT:
                exit('Image ' + filename + ' is not ' + str(WIDTH) + 'x' + str(HEIGHT) + ' pixels in size.')
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
        print('Converts files like foo.png into Game Gear-friendly formats like foo.chr')
