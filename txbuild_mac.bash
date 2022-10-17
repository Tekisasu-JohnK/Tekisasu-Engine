#!/bin/bash
stage=$1
if [[ "$stage" == "1" ]]; then
	scons platform=osx arch=arm64 --jobs=$(sysctl -n hw.logicalcpu) tools=yes
	scons platform=osx arch=x86_64 --jobs=$(sysctl -n hw.logicalcpu) tools=yes
	lipo -create bin/godot.macos.editor.x86_64 bin/godot.macos.editor.arm64 -output bin/godot.macos.editor.universal
elif [[ "$stage" == "2" ]]; then
	cd bin
	rm -r Tekisasu-Engine.app
	cd ..
	cp -r misc/dist/macos_tools.app bin/Tekisasu-Engine.app
	mkdir -p bin/Tekisasu-Engine.app/Contents/MacOS
	cp bin/godot.macos.editor.universal bin/Tekisasu-Engine.app/Contents/MacOS/Tekisasu-Engine
else
	echo "You need to specifiy what build state to run (1 or 2)!"
fi
