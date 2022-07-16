Import("env")

SDCC_OPTS = ["--model-large", "--opt-code-speed", "--peep-return",
             "--fomit-frame-pointer", "--allow-unsafe-read"]

env.Append(
    CFLAGS=SDCC_OPTS,
    LINKFLAGS=SDCC_OPTS
)
