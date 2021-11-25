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
WORLD_WIDTH = 780
WORLD_HEIGHT = 320
WORLD_SMOOTH_PASSES = 0 # 5

class Tiles(Enum):
    NOTHING = (132, 170, 248)
    STONE = (128, 128, 128)
    DIRT = (151, 107, 75)
    GRASS = (28, 216, 94)
    IRON = (181, 164, 149)
    WOOD = (169, 125, 93)
    PLATFORM = (177, 133, 103)
    VINE = (30, 150, 72)
    TREE = (151, 107, 75)
    SAND = (211, 198, 111)
    WATER = (9, 61, 191)
    COBWEB = (191, 191, 191)
    MUD = (92, 68, 73)
    CLAY = (146, 81, 68)
    COPPER = (150, 67, 22)
    TIN = (129, 125, 93)
    CRYSTAL = (182, 18, 57)
    CHEST = (161, 134, 160)
    JUNGLEGRASS = (143, 215, 29)
    ICE = (159, 191, 255)
    SNOW = (223, 223, 223)
    LAVA = (255, 95, 15)
    ASH = (47, 47, 47)
    HELLSTONE = (142, 66, 66)

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

def lerp(x, y, w):
    if (w < 0): return x
    if (w > 1): return y
    return (y - x) * ((w * (w * 6.0 - 15.0) + 10.0) * w * w * w) + x

def dot_grad(tile_x, tile_y, x, y):
    random.seed(tile_x + 1049576 * tile_y)
    angle = randFloat() * 1038576
    
    dx = x - tile_x
    dy = y - tile_y  
    return dx * math.cos(angle) + dy * math.sin(angle)

def perlin_gud(x, y):
    a = lerp(dot_grad(math.floor(x + 0), math.floor(y + 0), x, y), dot_grad(math.floor(x + 1), math.floor(y + 0), x, y), x - math.floor(x))
    b = lerp(dot_grad(math.floor(x + 0), math.floor(y + 1), x, y), dot_grad(math.floor(x + 1), math.floor(y + 1), x, y), x - math.floor(x))
    return lerp(a, b, y - math.floor(y))

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

def parabola(x, y, width, depth, material, noise):
    multiplier = -depth / (width / 2) ** 2
    for dX in range(width):
            if x + dX < 0: continue
            elif x + dX >= WORLD_WIDTH: return
            for dY in range(y):
                if getTile(x + dX, dY) != Tiles.NOTHING:
                    setTile(x + dX, dY, Tiles.NOTHING)
            for dY in range(int(multiplier * (dX - width / 2) ** 2 + depth + random.randrange(-noise / 2, noise / 2))):
                setTile(x + dX, y + dY, material)

def rect(x, y, width, height, material):
    for dY in range(height):
        for dX in range(width):
            setTile(x + dX, y + dY, material)

def generate():
    print("Terrain")

    for j in range(WORLD_WIDTH):
        print(j)

        sh = 24 + (perlin_gud(j / 60.0, 3.1) + 1) * 30.0 + (perlin_gud(j / 30.0, 4.1) + 1) * 15.0 + (perlin_gud(j / 6.0, 5.1) + 1) * 2.5
        dh = sh + ((perlin_gud(j / 60.0, 6.1) + 1) * 20.0 + (perlin_gud(j / 30.0, 7.1) + 1) * 10.0 + (perlin_gud(j / 6.0, 8.1) + 1) * 2.0)
        
        for i in range(WORLD_HEIGHT):
            value = 0
            tile = Tiles.DIRT
            
            if (sh <= i):
                if (dh <= i):
                    tile = Tiles.STONE

                if (perlin_gud(j / 32.0, i / 32.0) < lerp(0.3, 0.0, i / 100.0)):
                    setTile(j, i, tile)
                    continue
                
                noise = 0.0;
                detail = 30.0
                
                while (detail >= 7.5): # 7.5
                    noise += perlin_gud(j / detail, i / detail) * detail
                    detail /= 2.0
                
                noise /= 30.0
                value = math.floor(((((math.sin((j + 90.0 * noise) / 12.0) + 1) * 0.5) * ((math.sin((j + 90.0 * noise) / 12.0) + 1) * 0.5))) * 256.0)
                
                if (value >= 160 + lerp(96, 0, i / 175.0)):
                    value = 255
                else:
                    value = 0
                
                if (perlin_gud(j / 24.0, i / 8.0) >= 0.1 + lerp(0.1, 0.0, i / 175.0)):
                    value = 255
                
                if (value == 0):
                    setTile(j, i, tile)
    
    print("Tunnels")
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

    print("Sand")
    for i in range(40):
        x = random.randrange(0, WORLD_WIDTH)
        y = random.randrange(WORLD_HEIGHT // 3.5, WORLD_HEIGHT // 2)
        clump(x, y, poisson(40), Tiles.SAND, True, 0, 0)
    for i in range(max(2, poisson(3))):
        width = poisson(60)
        mul = -60 / width
        x = random.randrange(0, WORLD_WIDTH - width)
        for dX in range(width):
            left = min(20, mul * abs(dX - width / 2) + 30)
            y = 0
            while left > 0 :
                if getTile(x + dX, y) != Tiles.NOTHING:
                    setTile(x + dX, y, Tiles.SAND)
                    left -= 1
                y += 1
            
    print("Rocks in dirt")
    for i in range(1000):
        x = random.randrange(0, WORLD_WIDTH)
        y = random.randrange(0, WORLD_HEIGHT // 2.8)
        if getTile(x, y) == Tiles.DIRT:
            clump(x, y, poisson(10), Tiles.STONE, True, 0, 0)

    print("Dirt in rocks")
    for i in range(3000):
        x = random.randrange(0, WORLD_WIDTH)
        y = random.randrange(WORLD_HEIGHT // 3.2, WORLD_HEIGHT)
        if getTile(x, y) == Tiles.STONE:
            clump(x, y, poisson(10), Tiles.DIRT, True, 0, 0)

    print("Clay")
    for i in range(1000):
        x = random.randrange(0, WORLD_WIDTH)
        y = random.randrange(0, WORLD_HEIGHT // 4.2)
        if getTile(x, y) == Tiles.DIRT:
            clump(x, y, poisson(10), Tiles.CLAY, True, 0, 0)

    # print("Small holes")
    # for i in range(250):
    #     x = random.randrange(0, WORLD_WIDTH)
    #     y = random.randrange(WORLD_HEIGHT // 3.2, WORLD_HEIGHT)
    #     clump(x, y, poisson(50), Tiles.WATER, False, 0, 0)
    # for i in range(750):
    #     x = random.randrange(0, WORLD_WIDTH)
    #     y = random.randrange(WORLD_HEIGHT // 3.2, WORLD_HEIGHT)
    #     clump(x, y, poisson(50), Tiles.NOTHING, True, 0, 0)

    # A 500-length coord array should be enough for this
    # print("Caves")
    # for i in range(150):
    #     x = random.randrange(0, WORLD_WIDTH)
    #     y = random.randrange(WORLD_HEIGHT // 3.2, WORLD_HEIGHT)
    #     clump(x, y, poisson(200), Tiles.NOTHING, True, 0, 0)

    # print("Surface Caves...")
    # for i in range(150):
    #     x = random.randrange(0, WORLD_WIDTH)
    #     y = random.randrange(WORLD_HEIGHT // 4, WORLD_HEIGHT // 2.8)
    #     clump(x, y, poisson(125), Tiles.NOTHING, True, 0, 0)
    
    print("Grass")
    for x in range(WORLD_WIDTH):
        for y in range(int(WORLD_HEIGHT // 2.8)):
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

    print("Jungle")
    width = poisson(160)
    x = random.choice([WORLD_WIDTH // 5, WORLD_WIDTH - WORLD_WIDTH // 5 - width])
    for y in range(WORLD_HEIGHT):
        dX = 0
        pX = int(perlin_gud(y / 60.0, 6.17) * 30.0)
        random.seed(y)
        while getTile(x + dX + pX, y) == Tiles.NOTHING:
            dX += 1
        while dX < width:
            tile = getTile(x + dX + pX, y)
            if dX < 20 or width - dX < 20:
                num = (20 - dX) if dX < 20 else (20 - (width - dX))
                if random.randrange(num) > 5:
                    dX += 1
                    continue
            if tile == Tiles.DIRT or tile == Tiles.STONE or tile == Tiles.SAND:
                setTile(x + dX + pX, y, Tiles.MUD)
            elif tile == Tiles.GRASS:
                setTile(x + dX + pX, y, Tiles.JUNGLEGRASS)
            dX += 1
    for dX in range(width):
        for y in range(WORLD_HEIGHT):
            pX = int(perlin_gud(y / 60.0, 6.17) * 30.0)
            if getTile(x + dX + pX, y) == Tiles.MUD and (
                getTile(x + dX + pX - 1, y) == Tiles.NOTHING
                or getTile(x + dX + pX + 1, y) == Tiles.NOTHING
                or getTile(x + dX + pX, y - 1) == Tiles.NOTHING
                or getTile(x + dX + pX, y + 1) == Tiles.NOTHING):
                setTile(x + dX + pX, y, Tiles.JUNGLEGRASS)

    print("Snow Biome")
    width = poisson(110)
    if (x < WORLD_WIDTH // 2):
        x = WORLD_WIDTH - WORLD_WIDTH // 5 - width
    else:
        x = WORLD_WIDTH // 5
    for y in range(int(WORLD_HEIGHT / 1.5)):
            dX = 0
            pX = int(perlin_gud(y / 60.0, 6.17) * 20.0)
            random.seed(y)
            while getTile(x + dX + pX, y) == Tiles.NOTHING:
                dX += 1
            while dX < width:
                tile = getTile(x + dX + pX, y)
                if dX < 20 or width - dX < 20:
                    num = (20 - dX) if dX < 20 else (20 - (width - dX))
                    if random.randrange(num) > 5:
                        dX += 1
                        continue
                if ((int(WORLD_HEIGHT / 1.5) - y) < 20):
                    num = 20 - (int(WORLD_HEIGHT / 1.5) - y)
                    if random.randrange(num) > 5:
                        dX += 1
                        continue
                if tile == Tiles.STONE:
                    setTile(x + dX + pX, y, Tiles.ICE)
                elif tile == Tiles.DIRT or tile == Tiles.GRASS or tile == Tiles.SAND:
                    setTile(x + dX + pX, y, Tiles.SNOW)
                dX += 1
    for dX in range(width):
        for y in range(int(WORLD_HEIGHT / 1.5)):
            pX = int(perlin_gud(y / 60.0, 6.17) * 20.0)
            if getTile(x + dX + pX, y) == Tiles.ICE and (
                getTile(x + dX + pX - 1, y) == Tiles.NOTHING
                or getTile(x + dX + pX + 1, y) == Tiles.NOTHING
                or getTile(x + dX + pX, y - 1) == Tiles.NOTHING
                or getTile(x + dX + pX, y + 1) == Tiles.NOTHING) and False:
                setTile(x + dX + pX, y, Tiles.SNOW)
    
    print("Shinies")
    for i in range(750):
        x = random.randrange(WORLD_WIDTH)
        y = random.randrange(WORLD_HEIGHT)
        if getTile(x, y) != Tiles.SAND and getTile(x, y) != Tiles.SNOW and getTile(x, y) != Tiles.ICE:
            clump(x, y, poisson(6), Tiles.IRON, True, 0, 0)
    
    if random.randrange(0, 2) == 0:
        for i in range(750):
            x = random.randrange(WORLD_WIDTH)
            y = random.randrange(WORLD_HEIGHT)
            if getTile(x, y) != Tiles.SAND and getTile(x, y) != Tiles.SNOW and getTile(x, y) != Tiles.ICE:
                clump(x, y, poisson(8), Tiles.COPPER, True, 0, 0)
    else:
        for i in range(750):
            x = random.randrange(WORLD_WIDTH)
            y = random.randrange(WORLD_HEIGHT)
            if getTile(x, y) != Tiles.SAND and getTile(x, y) != Tiles.SNOW and getTile(x, y) != Tiles.ICE:
                clump(x, y, poisson(10), Tiles.TIN, True, 0, 0)

    print("Lakes")
    for i in range(max(2, poisson(4))):
        x = random.randrange(75, WORLD_WIDTH - 75)
        width = max(15, poisson(15))
        depth = max(5, poisson(10))
        leftY = WORLD_HEIGHT // 5
        while getTile(x, leftY) == Tiles.NOTHING:
            leftY += 1
            if getTile(x, leftY + 6) == Tiles.NOTHING: leftY += 6
        rightY = WORLD_HEIGHT // 5
        while getTile(x + width, rightY) == Tiles.NOTHING:
            rightY += 1
            if getTile(x + width, rightY + 6) == Tiles.NOTHING: rightY += 6
        y = max(leftY, rightY)
        parabola(x, y, width, depth, Tiles.WATER, 2)
        
    
    print("Beaches")
    leftY = 0
    while getTile(60, leftY) == Tiles.NOTHING:
        leftY += 1
    rightY = 0
    while getTile(WORLD_WIDTH - 60, rightY) == Tiles.NOTHING:
        rightY += 1
    # Left beach
    parabola(-62, leftY, 124, 32, Tiles.DIRT, 2)
    parabola(-60, leftY, 120, 30, Tiles.SAND, 2)
    parabola(-50, leftY + 2, 100, 20, Tiles.WATER, 2)
    # Right beach
    parabola(WORLD_WIDTH - 62, rightY, 124, 32, Tiles.DIRT, 2)
    parabola(WORLD_WIDTH - 60, rightY, 120, 30, Tiles.SAND, 2)
    parabola(WORLD_WIDTH - 50, rightY + 2, 100, 20, Tiles.WATER, 2)
            
    print("Gravitating Sand")
    for x in range(WORLD_WIDTH):
        for y in range(WORLD_HEIGHT, 0, -1):
            if getTile(x, y) == Tiles.SAND and getTile(x, y + 1) == Tiles.NOTHING:
                tempY = y
                while tempY < WORLD_HEIGHT - 1 and getTile(x, tempY + 1) == Tiles.NOTHING:
                    tempY += 1
                setTile(x, tempY, Tiles.SAND)
                setTile(x, y, Tiles.NOTHING)

    print("Wet Jungle")
    for x in range(1, WORLD_WIDTH):
        for y in range(WORLD_HEIGHT // 6, WORLD_HEIGHT // 2):
            if getTile(x, y) == Tiles.MUD:
                if getTile(x + 1, y) == Tiles.NOTHING and random.randrange(0, 3) == 0:
                    setTile(x + 1, y, Tiles.WATER)
                if getTile(x - 1, y) == Tiles.NOTHING and random.randrange(0, 3) == 0:
                    setTile(x - 1, y, Tiles.WATER)
                if getTile(x, y - 1) == Tiles.NOTHING and random.randrange(0, 3) == 0:
                    setTile(x , y - 1, Tiles.WATER)

    ySave = 0
    
    print("Smooth World")
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
                if deltaY > (2 if tile != Tiles.SAND else 1) and getTile(x, y + 6) != Tiles.NOTHING:
                    for dY in range(min(deltaY - 1, 2)):
                        setTile(x, y + dY, Tiles.NOTHING)
                    ySave = y + deltaY - 1
                else:   
                    ySave = y

    print("Cobwebs")
    for i in range(250):
        for tries in range(1000):
            x = random.randrange(0, WORLD_WIDTH)
            y = random.randrange(WORLD_HEIGHT // 2.4, WORLD_HEIGHT - (WORLD_HEIGHT // 6.8))

            if getTile(x, y) != Tiles.NOTHING and getTile(x, y) != Tiles.VINE and getTile(x, y) != Tiles.COBWEB:
                continue

            canPlace = 0
            new_y = y

            for j in range(y - 1, -1, -1):
                if(getTile(x, j) != Tiles.NOTHING):
                    canPlace = 1
                    break
                else:
                    new_y = j

            if(canPlace):
                clump(x, new_y, poisson(9), Tiles.COBWEB, False, 0, 0)
                break

    print("Life Crystals")
    for i in range(20):
        for place in range(50):
            x = random.randrange(25, WORLD_WIDTH - 25)
            y = random.randrange(WORLD_HEIGHT // 2.8, WORLD_HEIGHT - 10)
            if getTile(x, y) == Tiles.NOTHING:
                rect(x, y, 2, 2, Tiles.CRYSTAL)
                rect(x, y + 2, 2, 1, Tiles.STONE)
                break

    print("Buried Chests")
    for i in range(30):
        num = min(max(1, round(poisson(1.25))), 3)
        x = random.randrange(25, WORLD_WIDTH - 25)
        y = random.randrange(int(WORLD_HEIGHT / 2.8), WORLD_HEIGHT)
        while getTile(x, y) != Tiles.NOTHING:
            x = random.randrange(25, WORLD_WIDTH - 25)
            y = random.randrange(int(WORLD_HEIGHT / 2.8), WORLD_HEIGHT)
        placed = False
        for room in range(num):
            width = random.randrange(10, 20)
            box(x, y, width, 6, Tiles.WOOD, 0.9, Tiles.NOTHING)
            platformX = random.randrange(x + 1, x + width - 5)
            for dX in range(random.randrange(2, 5)):
                setTile(platformX + dX, y, Tiles.PLATFORM)
            y += 7
            x += random.randrange(-(width - 3), width - 3)
            if not placed:
                if random.randrange(0, num) or room == num - 1:
                    tempX = random.randrange(x, x + width - 2)
                    rect(tempX, y + 3, 2, 2, Tiles.CHEST)
                    placed = True

    print("Surface Chests")
    for x in range(WORLD_WIDTH):
        stage = 0
        for y in range(WORLD_HEIGHT // 4):
            if getTile(x, y) != Tiles.NOTHING: stage = 1
            elif stage == 1:
                if random.randrange(0, 35) == 0:
                    while getTile(x, y + 2) == Tiles.NOTHING: y += 1
                    rect(x, y - 1, 2, 2, Tiles.CHEST)
                break

    print("Spreading grass")
    for x in range(WORLD_WIDTH):
        for y in range(int(WORLD_HEIGHT // 2.8)):
            if getTile(x, y) == Tiles.DIRT:
                setTile(x, y, Tiles.GRASS)
                if getTile(x - 1, y) == Tiles.NOTHING or getTile(x + 1, y) == Tiles.NOTHING:
                    setTile(x, y + 1, Tiles.GRASS)
                break
            if getTile(x, y) == Tiles.MUD:
                setTile(x, y, Tiles.JUNGLEGRASS)
                if getTile(x - 1, y) == Tiles.NOTHING or getTile(x + 1, y) == Tiles.NOTHING:
                    setTile(x, y + 1, Tiles.JUNGLEGRASS)
                break
            elif getTile(x, y) != Tiles.NOTHING:
                break

    print("Planting Trees")
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
                    elif tile == Tiles.JUNGLEGRASS:
                        generateTree(x, y - 1, copseHeight + 4)
                        break
                    elif tile != Tiles.NOTHING: break   
        x += 1

    print("Vines")
    for x in range(WORLD_WIDTH):
        for y in range(int(WORLD_WIDTH)):
            tile = getTile(x, y)
            if tile == Tiles.GRASS and y < WORLD_HEIGHT // 3.2 and getTile(x, y + 1) == Tiles.NOTHING and random.randrange(0, 3) > 0:
                for dY in range(1, random.randrange(3, 11)):
                    if getTile(x, y + dY) != Tiles.NOTHING: break;
                    setTile(x, y + dY, Tiles.VINE)
            elif tile == Tiles.JUNGLEGRASS and getTile(x, y + 1) == Tiles.NOTHING and random.randrange(0, 3) > 0:
                for dY in range(1, random.randrange(3, 11)):
                    if getTile(x, y + dY) != Tiles.NOTHING: break;
                    setTile(x, y + dY, Tiles.VINE)

    print("Underworld")
    hell_height = int(WORLD_HEIGHT // 6.8)
    for x in range(WORLD_WIDTH):
        for y in range(hell_height):
            if y > (((perlin_gud(x / 20.0, 12.5) * 1.5 + perlin_gud(x / 8.0, 14.7) * 0.5) + 1) * (hell_height // 2)) + 10:
                setTile(x, y + (WORLD_HEIGHT - hell_height), Tiles.ASH)
            elif y > (hell_height // 1.8):
                setTile(x, y + (WORLD_HEIGHT - hell_height), Tiles.LAVA)
            else:
                setTile(x, y + (WORLD_HEIGHT - hell_height), Tiles.NOTHING)
    for x in range(int(WORLD_WIDTH // 4)):
        clump(int(4 * (x + 0.5)), WORLD_HEIGHT - hell_height, 60, Tiles.ASH, False, 0, 0)
    
    print("Hellstone")
    for i in range(200):
        x = random.randrange(WORLD_WIDTH)
        y = WORLD_HEIGHT - random.randrange(hell_height - 15)
        while getTile(x, y) != Tiles.ASH:
            x = random.randrange(WORLD_WIDTH)
            y = WORLD_HEIGHT - random.randrange(hell_height - 15)
        clump(x, y, poisson(7), Tiles.HELLSTONE, True, 0, 0)

##### END ALGORITHM #####

image = Image.new("RGB", (WORLD_WIDTH, WORLD_HEIGHT), Tiles.NOTHING.value)
generate()
show()
if input("Save image? ").lower() in ('y', 'yes'):
    image.resize((WORLD_WIDTH * 2, WORLD_HEIGHT * 2), Image.NEAREST).save("./map.png")
