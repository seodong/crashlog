all: update
	ndk-build V=1
	ant  -Dapp_manifest=AndroidManifest.xml release

clean:
	ndk-build clean
	ant clean
	rm -rf bin

update:
	android update project --path ./ --name crashlog --target android-15 

