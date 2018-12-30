set(crypto_SRCS
    aes.c
    base64.c
    des.c
    md5.c
    rsa.c
    sha1.c
    sha256.c
    sha512.c
#    aesni.c
#    arc4.c
#    asn1parse.c
#    asn1write.c
#    bignum.c
#    blowfish.c
#    camellia.c
#    ccm.c
#    cipher.c
#    cipher_wrap.c
#    cmac.c
#    ctr_drbg.c
#    dhm.c
#    ecdh.c
#    ecdsa.c
#    ecjpake.c
#    ecp.c
#    ecp_curves.c
#    entropy.c
#    entropy_poll.c
#    error.c
#    gcm.c
#    havege.c
#    hmac_drbg.c
#    md.c
#    md2.c
#    md4.c
#    md_wrap.c
#    memory_buffer_alloc.c
#    oid.c
#    padlock.c
#    pem.c
#    pk.c
#    pk_wrap.c
#    pkcs12.c
#    pkcs5.c
#    pkparse.c
#    pkwrite.c
#    platform.c
#    ripemd160.c
#    rsa_internal.c
#    threading.c
#    timing.c
#    version.c
#    version_features.c
#    xtea.c
)

if(NOT CMAKE_BUILD_DYNAMIC_LIB)
	add_library(mwnet_mt_crypto ${crypto_SRCS})
else()
	add_library(mwnet_mt_crypto SHARED ${crypto_SRCS})
endif()

target_link_libraries(mwnet_mt_crypto)

install(TARGETS mwnet_mt_crypto DESTINATION lib)

file(GLOB HEADERS "mbedtls/*.h")

#set(HEADERS
#	aes.h
#	base64.h
#	des.h
#	md5.h
#	rsa.h
#	sha256.h
#  )
install(FILES ${HEADERS} DESTINATION include/mwnet_mt/crypto)

#add_executable(util_test tests/util_test.cpp)
#target_link_libraries(util_test mwnet_mt_util)
