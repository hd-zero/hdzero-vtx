Import("env")

import sdcc_map

# hdzero "encryption" as figured out by Peter Peiser https://github.com/pjpei
bitFlipDictionary = {
    0x01: 0x80,
    0x02: 0x04,
    0x04: 0x20,
    0x08: 0x02,
    0x10: 0x08,
    0x20: 0x10,
    0x40: 0x01,
    0x80: 0x40
}


def parse_map(*args, **kwargs):
    sdcc_map.main(env.subst("$BUILD_DIR/${PROGNAME}.map"))

# Multiple actions
env.AddCustomTarget(
    name="sdccmap",
    dependencies=None,
    actions=[
        parse_map
    ],
    title="Map file size reports",
    description="Create reports of space used by symbols, sections and modules"
)


def encrypt(source, target, env):
    byteMap = {}
    reverseByteMap = {}

    for i in range(256):
        newByte = 0
        for bit in bitFlipDictionary:
            if(i & bit == bit):
                newByte = newByte + bitFlipDictionary[bit]
        byteMap[i] = newByte
        reverseByteMap[newByte] = i

    src_name = env.subst("$BUILD_DIR/${PROGNAME}.bin")

    byteArr = []
    with open(src_name, "rb") as file:
        for sourceByte in file.read():
            byteArr.append(byteMap[sourceByte])

    dest_name = env.subst("$BUILD_DIR/HDZERO_TX.bin")
    with open(dest_name, "wb") as file:
        file.write(bytearray(byteArr))


env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.hex",
    [
        parse_map,
        env.VerboseAction(" ".join([
            "$OBJCOPY", "-I", "ihex", "-O", "binary",
            "$BUILD_DIR/${PROGNAME}.hex", "$BUILD_DIR/${PROGNAME}.bin"
        ]), "Building $BUILD_DIR/${PROGNAME}.bin"),
        encrypt
    ]
)
