WT_BUILD = deps/wiredtiger-2.1.2

all: build_wt
	node-gyp build

clean:
	$(MAKE) -C $(WT_BUILD) clean
	node-gyp clean

config:
	mkdir -p $(WT_BUILD)
	(TOP=`pwd` && cd $(WT_BUILD) && env CFLAGS="-fPIC" ./configure --prefix=$$TOP --disable-shared)
	node-gyp configure

build_wt:
	$(MAKE) -C $(WT_BUILD) install



