#!/bin/bash

# This is just for development.  Production has its own key.
export SCRIPT_AES256_ENCRYPTION_KEY=$(cat godot.gdkey)

action=$1
command=$2
value=$2




if [[ "$action" == "build" ]]; then
	if [[ "$command" != "clean" ]]; then
		scons platform=macos arch=arm64 --jobs=$(sysctl -n hw.logicalcpu) target=editor bits=64
		scons platform=macos arch=x86_64 --jobs=$(sysctl -n hw.logicalcpu) target=editor bits=64
		if [[ -f bin/godot.macos.editor.x86_64 ]] && [[ -f bin/godot.macos.editor.arm64 ]]; then
			lipo -create bin/godot.macos.editor.x86_64 bin/godot.macos.editor.arm64 -output bin/godot.macos.editor.universal
		else
			echo "txbuild: x86_64+arm64 build failed!  Cannot run lipo.  Exiting..."
			exit 1
		fi
	elif [[ "$command" == "clean" ]]; then
		scons platform=macos arch=arm64 --jobs=$(sysctl -n hw.logicalcpu) target=editor -c
		scons platform=macos arch=x86_64 --jobs=$(sysctl -n hw.logicalcpu) target=editor -c
		if [[ -f bin/godot.macos.editor.universal ]]; then 
			rm bin/godot.macos.editor.universal
		fi
	else 
		echo "txbuild: Second argument $clean unrecognized.  Use 'c' to clean or leave empty to build.  Exiting..."
		exit 1
	fi
elif [[ "$action" == "package" ]]; then
	if [[ "$command" == "editor" ]]; then 
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
	elif [[ "$command" == "templates" ]]; then
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
		echo "ERROR: valid arguments are 'package editor' and 'package templates'"
		exit 1
	fi
elif [[ "$action" == "exports" ]]; then
	if [[ "$command" != "clean" ]]; then
		scons platform=macos target=template_release arch=arm64 --jobs=$(sysctl -n hw.logicalcpu)
		scons platform=macos target=template_release arch=x86_64 --jobs=$(sysctl -n hw.logicalcpu)
		if [[ -f bin/godot.macos.template_release.x86_64 ]] && [[ -f bin/godot.macos.template_release.arm64 ]]; then
			lipo -create bin/godot.macos.template_release.x86_64 bin/godot.macos.template_release.arm64 -output bin/godot.macos.template_release.universal
		else
			echo "txbuild: release template x86_64+arm64 build failed!  Cannot run lipo.  Exiting..."
			exit 1
		fi
	elif [[ "$command" == "clean" ]]; then
		scons platform=macos target=template_release arch=arm64 --jobs=$(sysctl -n hw.logicalcpu) -c
		scons platform=macos target=template_release arch=x86_64 --jobs=$(sysctl -n hw.logicalcpu) -c
		if [[ -f bin/godot.macos.template_release.universal ]]; then 
			rm bin/godot.macos.template_release.universal
		fi
	else 
		echo "txbuild: Second argument command unrecognized.  Use 'clean' to clean or leave empty to build.  Exiting..."
		exit 1
	fi
else
	echo "txbuild: you need to specifiy what build state to run."
	echo "	Examples for Tekisasu-Engine editor"
	echo "		# txbuild_mac.bash build"
	echo "		# txbuild_mac.bash build clean"
	echo "  Examples for Tekisasu-Engine exports"
	echo "		# txbuild_mac.bash exports"
	echo "		# txbuild_mac.bash exports clean"
	echo "	Examples for Tekisasu-Engine packaging"
	echo "		# txbuild_mac.bash package"
fi
