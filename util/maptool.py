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
WORLD_SMOOTH_PASSES = 5

class Tiles(Enum):
    NOTHING = (123, 152, 254)
    STONE = (128, 128, 128)
    DIRT = (152, 106, 76)
    GRASS = (38, 127, 0)
    IRON = (140,101,80)
    WOOD = (151, 107, 75)
    PLATFORM = (191, 141, 111)
    VINE = (23, 177, 76)
    TREE = (151, 107, 75)
    SAND = (186, 168, 84)
    WATER = (9, 61, 191)

# Call whenever you want to see the world
def show():
    mpplot.imshow(image)
    mpplot.show()

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

def poisson(mean):
    k = 0
    p = 1
    L = math.pow(math.e, -mean)
    while p > L:
        k += 1
        u = random.random()
        p *= u
    return k - 1

def perlin(amplitude, wavelength, baseY, tile, iterations):
    yPositions = [int(baseY)] * WORLD_WIDTH
    for i in range(iterations):
        a = randFloat()
        b = randFloat()
        for x in range(WORLD_WIDTH):
            if(x % wavelength == 0):
                a = b
                b = randFloat()
                perlinY = a * amplitude
            else:
                perlinY = interpolate(a, b, (x % wavelength) / wavelength) * amplitude
            yPositions[x] += int(perlinY)
        wavelength >>= 1
        amplitude >>= 1
    for x in range(WORLD_WIDTH):
        for y in range(yPositions[x], WORLD_HEIGHT):
            setTile(x, y, tile)

# Very versatile function
# skewHorizontal and skewVertical must be between 0 and 1
# Only one should be above 0
def clump(x, y, num, tile, maskEmpty, skewHorizontal, skewVertical):
    if maskEmpty and getTile(x, y) == Tiles.NOTHING:
        return
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
            if skewHorizontal > 0:
                if delta[1] != 0 and random.random() < skewHorizontal:
                    continue
            if skewVertical > 0:
                if delta[0] != 0 and random.random() < skewVertical:
                    continue
            if getTile(checkX, checkY) == tile or (maskEmpty and getTile(checkX, checkY) == Tiles.NOTHING):
                continue
            coords.append((checkX, checkY))

def box(x, y, width, height, tile, coverage, swap):
    for dX in range(width):
        for dY in range(height):
            if dY == 0 or dY == height - 1 or dX == 0 or dX == width - 1:
                if random.random() < coverage:
                    setTile(x + dX, y + dY, tile)
                else:
                    setTile( x + dX, y + dY, swap)
            else:
                setTile(x + dX, y + dY, Tiles.NOTHING)

def generateTree(x, y, baseHeight):
    top = 0
    height = max(1, baseHeight + random.randrange(0, 3) - 1)

    if x == 0 or x == WORLD_WIDTH - 1 or y < 0 or y >= WORLD_HEIGHT - height: return
    for i in range(height):
        if getTile(x - 1, y - i) == Tiles.TREE \
        or getTile(x + 1, y - i) == Tiles.TREE \
        or getTile(x - 2, y - i) == Tiles.TREE \
        or getTile(x + 2, y - i) == Tiles.TREE:
            return

    while top < height:
        setTile(x, y - top, Tiles.TREE)
        top += 1

    setTile(x, y - top, Tiles.TREE)
    if getTile(x - 1, y + 1) == Tiles.GRASS and random.randrange(0, 3) <= 1:
        setTile(x - 1, y, Tiles.TREE)
    if getTile(x + 1, y + 1) == Tiles.GRASS and random.randrange(0, 3) <= 1: 
        setTile(x + 1, y, Tiles.TREE)

def generate():
    
    print("Generating dirt...")
    perlin(10, 30, WORLD_HEIGHT // 5, Tiles.DIRT, 3)
    
    print("Generating stone...")
    perlin(6, 20, WORLD_HEIGHT // 2.8, Tiles.STONE, 1)

    print("Tunnels...")
    x = 100
    while x < WORLD_WIDTH - 100:
        if random.random() < 0.01:
            yPositions = []
            for dX in range(0, 50, 5):
                y = 0
                while y < WORLD_HEIGHT and getTile(x + dX, y + 8) != Tiles.DIRT: y += 1
                yPositions.append(y)
            for dX, position in enumerate(yPositions):
                clump(x + 5 * dX, position, 20, Tiles.DIRT, False, 0.5, 0)
            x += 100
        else: x += 1

    print("Sand...")
    for i in range(60):
        x = random.randrange(0, WORLD_WIDTH)
        y = random.randrange(WORLD_HEIGHT // 3.5, WORLD_HEIGHT // 2)
        clump(x, y, poisson(50), Tiles.SAND, True, 0, 0)
    for i in range(max(2, poisson(3))):
        width = poisson(75)
        mul = -30 / width
        x = random.randrange(0, WORLD_WIDTH - width)
        for dX in range(width):
            left = min(10, mul * abs(dX - width / 2) + 15)
            y = 0
            while left > 0 :
                if getTile(x + dX, y) != Tiles.NOTHING:
                    setTile(x + dX, y, Tiles.SAND)
                    left -= 1
                y += 1
            
    print("Rocks in dirt...")
    for i in range(1000):
        x = random.randrange(0, WORLD_WIDTH)
        y = random.randrange(0, WORLD_HEIGHT // 2.8)
        if getTile(x, y) == Tiles.DIRT:
            clump(x, y, poisson(10), Tiles.STONE, True, 0, 0)

    print("Dirt in rocks...")
    for i in range(3000):
        x = random.randrange(0, WORLD_WIDTH)
        y = random.randrange(WORLD_HEIGHT // 2.8, WORLD_HEIGHT)
        if getTile(x, y) == Tiles.STONE:
            clump(x, y, poisson(10), Tiles.DIRT, True, 0, 0)

    print("Small holes...")
    for i in range(250):
        x = random.randrange(0, WORLD_WIDTH)
        y = random.randrange(WORLD_HEIGHT // 3.2, WORLD_HEIGHT)
        clump(x, y, poisson(50), Tiles.WATER, False, 0, 0)
    for i in range(750):
        x = random.randrange(0, WORLD_WIDTH)
        y = random.randrange(WORLD_HEIGHT // 3.2, WORLD_HEIGHT)
        clump(x, y, poisson(50), Tiles.NOTHING, True, 0, 0)

    # A 500-length coord array should be enough for this
    print("Caves...")
    for i in range(150):
        x = random.randrange(0, WORLD_WIDTH)
        y = random.randrange(WORLD_HEIGHT // 3.2, WORLD_HEIGHT)
        clump(x, y, poisson(200), Tiles.NOTHING, True, 0, 0)
    
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

    print("Shinies...")
    for i in range(750):
        x = random.randrange(WORLD_WIDTH)
        y = random.randrange(0, WORLD_HEIGHT)
        if getTile(x, y) != Tiles.SAND:
            clump(x, y, poisson(10), Tiles.IRON, True, 0, 0)

    print("Lakes...")
    for i in range(min(2, poisson(3))):
        x = random.randrange(75, WORLD_WIDTH - 75)
        width = poisson(30)
        depth = poisson(10)
        multiplier = -depth / (width / 2) ** 2
        y1 = WORLD_HEIGHT // 5
        while getTile(x, y1) == Tiles.NOTHING:
            y1 += 1
            if getTile(x, y1 + 6) == Tiles.NOTHING:
                y1 += 6
        y2 = WORLD_HEIGHT // 5
        while getTile(x + width, y2) == Tiles.NOTHING:
            y2 += 1
            if getTile(x + width, y2 + 6) == Tiles.NOTHING:
                y2 += 6
        y = max(y1, y2)
        for dX in range(width):
            for dY in range(y):
                if getTile(x + dX, dY) != Tiles.NOTHING:
                    setTile(x + dX, dY, Tiles.NOTHING)
            for dY in range(int(multiplier * (dX - width / 2) ** 2 + depth)):
                setTile(x + dX, y + dY, Tiles.WATER)
            
    print("Gravitating Sand...")
    for x in range(WORLD_WIDTH):
        for y in range(WORLD_HEIGHT, 0, -1):
            if getTile(x, y) == Tiles.SAND and getTile(x, y + 1) == Tiles.NOTHING:
                tempY = y
                while tempY < WORLD_HEIGHT - 1 and getTile(x, tempY + 1) == Tiles.NOTHING:
                    tempY += 1
                setTile(x, tempY, Tiles.SAND)
                setTile(x, y, Tiles.NOTHING)

    print("Smooth World...")
    for i in range(WORLD_SMOOTH_PASSES):
        for passDir in range(2):
            for y in range(WORLD_HEIGHT):
                if getTile((WORLD_WIDTH - 1) if passDir else 0, y) != Tiles.NOTHING:
                    ySave = y
                    break
            for x in range((WORLD_WIDTH - 1) if passDir else 0, 0 if passDir else WORLD_WIDTH, -1 if passDir else 1):
                y = 0
                while getTile(x, y) == Tiles.NOTHING:
                    y += 1
                deltaY = ySave - y
                tile = getTile(x, y)
                if tile == Tiles.WATER and deltaY > 0:
                    for dY in range(deltaY):
                        setTile(x, y + dY, Tiles.NOTHING)
                    ySave = y + deltaY - 1
                elif deltaY > (2 if tile != Tiles.SAND else 1) and getTile(x, y + 6) != Tiles.NOTHING:
                    for dY in range(min(deltaY - 1, 2)):
                        setTile(x, y + dY, Tiles.NOTHING)
                    ySave = y + deltaY - 1
                else:   
                    ySave = y

    print("Buried Chests...")
    for i in range(30):
        num = min(max(1, round(poisson(1.25))), 3)
        x = random.randrange(25, WORLD_WIDTH - 25)
        y = random.randrange(int(WORLD_HEIGHT / 2.8), WORLD_HEIGHT)
        while getTile(x, y) != Tiles.NOTHING:
            x = random.randrange(25, WORLD_WIDTH - 25)
            y = random.randrange(int(WORLD_HEIGHT / 2.8), WORLD_HEIGHT)
        for room in range(num):
            width = random.randrange(10, 20)
            box(x, y, width, 6, Tiles.WOOD, 0.9, Tiles.NOTHING)
            platformX = random.randrange(x + 1, x + width - 5)
            for dX in range(random.randrange(2, 5)):
                setTile(platformX + dX, y, Tiles.PLATFORM)
            y += 7
            x += random.randrange(-(width - 3), width - 3)

    print("Planting Trees...")
    x = 0
    while x < WORLD_WIDTH:
        if random.randrange(0, 40) == 0:
            copseHeight = random.randrange(1, 8)
            while random.randrange(0, 10) != 0 and x < WORLD_WIDTH:
                x += 4
                for y in range(1, WORLD_HEIGHT):
                    tile = getTile(x, y)
                    if tile == Tiles.GRASS:
                        generateTree(x, y - 1, copseHeight)
                        break
                    elif tile != Tiles.NOTHING: break   
        x += 1

    print("Vines...")
    for x in range(WORLD_WIDTH):
        for y in range(int(WORLD_WIDTH // 4.5)):
            if getTile(x, y) == Tiles.GRASS and getTile(x, y + 1) == Tiles.NOTHING and random.randrange(0, 3) > 0:
                for dY in range(1, random.randrange(3, 11)):
                    if getTile(x, y + dY) != Tiles.NOTHING: break;
                    setTile(x, y + dY, Tiles.VINE)

##### END ALGORITHM #####

image = Image.new("RGB", (WORLD_WIDTH, WORLD_HEIGHT), (123, 152, 254))
generate()
show()
if input("Save image? ").lower() in ('y', 'yes'):
    image.resize((2000, 500), Image.NEAREST).save("./map.png")
