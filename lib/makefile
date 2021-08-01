bearssl:
	git clone https://www.bearssl.org/git/BearSSL
	mv BearSSL tmp
	mv tmp bearssl

	mv bearssl/inc/*.h bearssl
	mv bearssl/src/hash/sha1.c bearssl
	mv bearssl/src/codec/dec32be.c bearssl
	mv bearssl/src/codec/enc32be.c bearssl
	mv bearssl/src/inner.h bearssl
	mv bearssl/src/config.h bearssl

clean:
	rm -rf ./bearssl
