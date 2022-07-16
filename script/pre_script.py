Import("env")

SDCC_OPTS = ["--model-large", "--opt-code-speed"]

env.Append(
    CFLAGS=SDCC_OPTS,
    LINKFLAGS=SDCC_OPTS
)
