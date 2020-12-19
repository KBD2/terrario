# Put this script in a directory with the capt.vram file.
# Run it and it'll generate a 512x256 screenshot from it and save to out.png.

from PIL import Image

with open('capt.vram', 'rb') as file:
    raw = list(i for i in file.read())
    decompressed = []
    for byte in raw:
        for i in range(8):
            decompressed.append(byte >> (7 - i) & 1)

img = Image.new('RGB', (128, 64))
pixels = img.load()
for y in range(64):
    for x in range(128):
        lightOn = decompressed[y * 128 + x]
        darkOn = decompressed[y * 128 + x + 8192]
        if lightOn and darkOn:
            pixels[x, y] = (0, 0, 0)
        elif darkOn:
            pixels[x, y] = (16, 16, 16)
        elif lightOn:
            pixels[x, y] = (150, 150, 150)
        else:
            pixels[x, y] = (255, 255, 255)
            
img = img.resize((512, 256), Image.NEAREST)        
img.save("out.png", "PNG")
