WT_BUILD = deps/wiredtiger-2.1.3

all: build_wt
	node-gyp build -d

clean:
	$(MAKE) -C $(WT_BUILD) clean
	node-gyp clean

config:
	mkdir -p $(WT_BUILD)
	(TOP=`pwd` && cd $(WT_BUILD) && env CFLAGS="-fPIC" ./configure --enable-snappy --enable-bzip2 --enable-zlib --prefix=$$TOP)
	node-gyp configure -d

build_wt:
	$(MAKE) -C $(WT_BUILD) install



