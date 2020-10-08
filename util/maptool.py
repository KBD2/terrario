# This script is used to help me create the world generation algorithm.
# It's included for anyone curious as to how it works, feel free to use any
# parts of the algorithm!
# Uses https://imgur.com/a/EoXWh as a reference, credit to the tModLoader team!

from PIL import Image
import matplotlib.pyplot as mpplot
from enum import Enum
import random
import math

# Map constants
WORLD_WIDTH = 1000
WORLD_HEIGHT = 250

class Tiles(Enum):
    NOTHING = (123, 152, 254)
    STONE = (128, 128, 128)
    DIRT = (152, 106, 76)
    GRASS = (38, 127, 0)

# Gets replaced with the appropriate function in the actual code
def setTile(x, y, tile):
    if x < 0 or x >= WORLD_WIDTH or y < 0 or y >= WORLD_HEIGHT:
        return
    image.putpixel((int(x), int(y)), tile.value)

def getTile(x, y):
    if x < 0 or x >= WORLD_WIDTH or y < 0 or y >= WORLD_HEIGHT:
        return None
    pixel = image.getpixel((x, y))
    for tile in Tiles:
        if pixel == tile.value:
            return tile
    return None

##### MAP ALGORITHM #####
# Everything in here gets translated into C

def interpolate(a, b, x):
    f = (1.0 - math.cos(x * math.pi)) * 0.5;
    return a * (1.0 - f) + b * f;

def randFloat():
    return random.random()

def perlin(amplitude, wavelength, baseY, tile):
    a = randFloat()
    b = randFloat()
    for x in range(WORLD_WIDTH):
        if(x % wavelength == 0):
            a = b
            b = randFloat()
            perlinY = baseY + a * amplitude
        else:
            perlinY = baseY + interpolate(a, b, (x % wavelength) / wavelength) * amplitude
        setTile(x, perlinY, tile)
        for tempY in range(int(perlinY), WORLD_HEIGHT):
            setTile(x, tempY, tile)

# Very versatile function
def clump(x, y, num, tile, maskEmpty=False):
    coords = [(x, y)]
    while num > 0:
        if len(coords) == 0:
            return
        selected = random.randrange(0, len(coords))
        selectedTile = coords[selected]
        del coords[selected]
        setTile(*selectedTile, tile)
        num -= 1
        for delta in ((0, -1), (1, 0), (0, 1), (-1, 0)):
            checkX = selectedTile[0] + delta[0]
            checkY = selectedTile[1] + delta[1]
            if getTile(checkX, checkY) == tile or (maskEmpty and getTile(checkX, checkY) == Tiles.NOTHING):
                continue
            coords.append((checkX, checkY))

def generate():
    
    print("Generating dirt...")
    perlin(10, 20, WORLD_HEIGHT // 5, Tiles.DIRT)
    
    print("Generating stone...")
    perlin(6, 20, WORLD_HEIGHT // 2.8, Tiles.STONE)

    print("Rocks in dirt...")
    for i in range(1000):
        x = random.randrange(0, WORLD_WIDTH)
        y = random.randrange(0, WORLD_HEIGHT // 2.8)
        if getTile(x, y) == Tiles.DIRT:
            clump(x, y, random.randrange(5, 15), Tiles.STONE, True)

    print("Dirt in rocks...")
    for i in range(3000):
        x = random.randrange(0, WORLD_WIDTH)
        y = random.randrange(WORLD_HEIGHT // 2.8, WORLD_HEIGHT)
        if getTile(x, y) == Tiles.STONE:
            clump(x, y, random.randrange(5, 15), Tiles.DIRT, True)

    print("Small holes...")
    for i in range(750):
        x = random.randrange(0, WORLD_WIDTH)
        y = random.randrange(WORLD_HEIGHT // 4, WORLD_HEIGHT)
        clump(x, y, random.randrange(5, 50), Tiles.NOTHING, True)

    # A 500-length coord array should be enough for this
    print("Caves...")
    for i in range(150):
        x = random.randrange(0, WORLD_WIDTH)
        y = random.randrange(WORLD_HEIGHT // 3.5, WORLD_HEIGHT)
        clump(x, y, random.randrange(150, 250), Tiles.NOTHING, True)
    
    print("Generating grass...")
    for x in range(WORLD_WIDTH):
        for y in range(WORLD_HEIGHT):
            if getTile(x, y) == Tiles.DIRT:
                setTile(x, y, Tiles.GRASS)
                if getTile(x - 1, y) == Tiles.NOTHING or getTile(x + 1, y) == Tiles.NOTHING:
                    setTile(x, y + 1, Tiles.GRASS)
                break
            elif getTile(x, y) != Tiles.NOTHING:
                break
    for x in range(WORLD_WIDTH):
        for y in range(int(WORLD_HEIGHT // 2.8)):
            if getTile(x, y) == Tiles.DIRT and (
                getTile(x - 1, y) == Tiles.NOTHING
                or getTile(x + 1, y) == Tiles.NOTHING
                or getTile(x, y - 1) == Tiles.NOTHING
                or getTile(x, y + 1) == Tiles.NOTHING):
                setTile(x, y, Tiles.GRASS)

##### END ALGORITHM #####

image = Image.new("RGB", (WORLD_WIDTH, WORLD_HEIGHT), (123, 152, 254))
generate()
mpplot.imshow(image)
mpplot.show()
if input("Save image? ").lower() in ('y', 'yes'):
    image.save("./map.png")
