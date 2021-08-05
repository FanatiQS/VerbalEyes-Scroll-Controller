bearssl:
	git clone https://www.bearssl.org/git/BearSSL
	mv BearSSL tmp
	mkdir bearssl

	mv tmp/inc/*.h bearssl
	mv tmp/src/hash/sha1.c bearssl
	mv tmp/src/codec/dec32be.c bearssl
	mv tmp/src/codec/enc32be.c bearssl
	mv tmp/src/inner.h bearssl
	mv tmp/src/config.h bearssl

	rm -rf ./tmp


clean:
	rm -rf ./bearssl
