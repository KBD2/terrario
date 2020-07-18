# Put this script in a directory with the light.vram and dark.vram files.
# Run it and it'll generate a 512x256 screenshot from them and save to out.png.

from PIL import Image

with open('light.vram', 'rb') as file:
    lightRaw = list(i for i in file.read())
    lightFull = []
    for byte in lightRaw:
        for i in range(8):
            lightFull.append(byte >> (7 - i) & 1)

with open('dark.vram', 'rb') as file:
    darkRaw = list(i for i in file.read())
    darkFull = []
    for byte in darkRaw:
        for i in range(8):
            darkFull.append(byte >> (7 - i) & 1)

img = Image.new('RGB', (128, 64))
pixels = img.load()
for y in range(64):
    for x in range(128):
        lightOn = lightFull[y * 128 + x]
        darkOn = darkFull[y * 128 + x]
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
