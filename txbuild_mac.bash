#!/bin/bash
stage=$1
clean=$2
if [[ "$stage" == "1" ]]; then
	if [[ "$clean" != "c" ]]; then
		scons platform=macos arch=arm64 --jobs=$(sysctl -n hw.logicalcpu) target=editor bits=64
		scons platform=macos arch=x86_64 --jobs=$(sysctl -n hw.logicalcpu) target=editor bits=64
		if [[ -f bin/godot.macos.editor.x86_64 ]] && [[ -f bin/godot.macos.editor.arm64 ]]; then
			lipo -create bin/godot.macos.editor.x86_64 bin/godot.macos.editor.arm64 -output bin/godot.macos.editor.universal
		else
			echo "txbuild: x86_64+arm64 build failed!  Cannot run lipo.  Exiting..."
			exit 1
		fi
	elif [[ "$clean" == "c" ]]; then
		scons platform=macos arch=arm64 --jobs=$(sysctl -n hw.logicalcpu) target=editor -c
		scons platform=macos arch=x86_64 --jobs=$(sysctl -n hw.logicalcpu) target=editor -c
		if [[ -f bin/godot.macos.editor.universal ]]; then 
			rm bin/godot.macos.editor.universal
		fi
	else 
		echo "txbuild: Second argument $clean unrecognized.  Use 'c' to clean or leave empty to build.  Exiting..."
		exit 1
	fi
elif [[ "$stage" == "2" ]]; then
	cd bin
	if [[ -f Tekisasu-Engine.app ]]; then
		rm -r Tekisasu-Engine.app
	fi
	cd ..
	cp -r misc/dist/macos_tools.app bin/Tekisasu-Engine.app
	mkdir -p bin/Tekisasu-Engine.app/Contents/MacOS
	cp bin/godot.macos.editor.universal bin/Tekisasu-Engine.app/Contents/MacOS/Tekisasu-Engine
	echo "txbuild: Tekisasu-Engine editor binary (arm64,x86_64) added to app bundle."
	echo "txbuild: macOS app bundle 'Tekisasu-Engine.app' packaged."
else
	echo "Ytxbuild: ou need to specifiy what build state to run (1 or 2)!"
fi
