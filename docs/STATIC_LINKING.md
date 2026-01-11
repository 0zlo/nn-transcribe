# Static linking policy (glibc)

On Ubuntu (glibc), full static linking can be painful.
This repo uses "mostly static":

- your code + fetched deps build static (.a)
- libstdc++ and libgcc are linked static (`-static-libstdc++ -static-libgcc`)
- ONNX Runtime is linked shared and bundled next to the exe (`libonnxruntime.so`)

If you enable `-DNN_FULL_STATIC=ON`, expect breakage unless your environment is prepared for full static (often musl-based).
