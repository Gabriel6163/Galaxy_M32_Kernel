# Kbuild fix for Clang LTO Duplicate Symbols (SysArchitect)

# 1. Base comum (Compilada estritamente uma vez via Kbuild sort)
obj-$(CONFIG_ZSTD_COMPRESS) += zstd_common_core.o
obj-$(CONFIG_ZSTD_DECOMPRESS) += zstd_common_core.o
zstd_common_core-y := entropy_common.o fse_decompress.o zstd_common.o

# 2. Motor de Compressão
obj-$(CONFIG_ZSTD_COMPRESS) += zstd_compress.o
zstd_compress-y := fse_compress.o huf_compress.o compress.o

# 3. Motor de Descompressão
obj-$(CONFIG_ZSTD_DECOMPRESS) += zstd_decompress.o
zstd_decompress-y := huf_decompress.o decompress.o
