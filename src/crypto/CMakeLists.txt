#------------------------------------------------------------------------------
# CMake file for Bigbang
#
# Copyright (c) 2019-2020 The Bigbang developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#------------------------------------------------------------------------------

set(pow_hash
    pow_hash/common/int-util.h
    pow_hash/aesb.c          
    pow_hash/hash.c  
    pow_hash/hash-extra-skein.c
    pow_hash/jh.h     
    pow_hash/slow-hash.c
    pow_hash/slow-hash_1.c
    pow_hash/variant4_random_math.h
    pow_hash/blake256.c
    pow_hash/groestl.c
    pow_hash/hash-extra-blake.c
    pow_hash/hash.h
    pow_hash/keccak.c
    pow_hash/oaes_config.h
    pow_hash/skein.c 
    pow_hash/warnings.h
    pow_hash/blake256.h
    pow_hash/groestl.h
    pow_hash/hash-extra-groestl.c
    pow_hash/hash-ops.h
    pow_hash/keccak.h
    pow_hash/oaes_lib.c
    pow_hash/skein.h
    pow_hash/groestl_tables.h  
    pow_hash/hash-extra-jh.c
    pow_hash/jh.c
    pow_hash/oaes_lib.h
    pow_hash/skein_port.h
    pow_hash/variant2_int_sqrt.h
)

set(curve25519
    curve25519/base25519.cpp    curve25519/base25519.h
    curve25519/fp25519.cpp      curve25519/fp25519.h
    curve25519/sc25519.cpp      curve25519/sc25519.h
    curve25519/ed25519.cpp      curve25519/ed25519.h
    curve25519/curve25519.h
)

set(sources
    crc24q.cpp      crc24q.h
    crypto.cpp      crypto.h
    base32.cpp      base32.h
)


set(CMAKE_CXX_FLAGS "-fPIC")

add_library(crypto SHARED ${sources})

include_directories(../xengine ../common ./ ${sodium_INCLUDE_DIR})
