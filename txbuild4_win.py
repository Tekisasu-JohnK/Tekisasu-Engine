import os
import sys
import getopt

# Tekisasu txbuild.py
#   A tool to easily push out builds with specifcs arch and targets, or all
# current windows targets without copy and pasting a bunch of scons commands.

# In python functions, use "/", it is cross platform.
# But where I use (lazily) use os.system(), use F_SLASH
if os.name == "nt":
	F_SLASH = "\\"
else:
	F_SLASH = "/"

# Set Tekisasu encryption key
os.system("set SCRIPT_AES256_ENCRYPTION_KEY=e604b6aa88df1bea5b01ef6383b3535abf345b4b9f76a2be391e569465234cd5")

# Set some common globals
miscerror = "else"
BIN_FOLDER = "bin"
CLEAN_FLAGS = "-c "
EXPORT_FOLDER = BIN_FOLDER + F_SLASH + "export"
BUILD_CLEAN = ""
CREATE_SHIPPING_EXPORTS = 0

# Initiatialize argument vars
action = ""
target = ""
cpuarc = ""
uwp = ""

# Iterate through all arguments
index=0

def print_tekisasu_header():
	print (" \n +-----------------------------------------+\n |      Tekisasu-Engine build script       |")
	print (" +-----------------------------------------+\n ")

# Sanity check arg inputs
for line_arg in sys.argv:
	line_arg = line_arg.lower()
#	print (index, line_arg)
	if index == 1: # Check for {action}
		if line_arg == "build" or line_arg == "clean":
			action = line_arg

	if index == 2: # Check for {target}
		if line_arg == "release" or line_arg == "debug" or line_arg == "editor" or line_arg == "all":
			target = line_arg

	index = index + 1

print_tekisasu_header()

def move_for_export(SOURCE_FILE, TARGET_FILE):
	# Detect old target file and delete
	if os.path.exists(EXPORT_FOLDER + F_SLASH + TARGET_FILE):
		SEND_CMD=("del " + EXPORT_FOLDER + F_SLASH + TARGET_FILE)
		os.system(SEND_CMD)
		SEND_CMD=""
		print("Older target file detected and deleted!")
	
	# Copy bin/SconsCreatedFile.exe to export/PrettyNameForTemplates.exe
	SEND_CMD=("echo F | xcopy " + BIN_FOLDER + F_SLASH + SOURCE_FILE + " " + EXPORT_FOLDER + F_SLASH + TARGET_FILE)
	print("SEND_CMD : " + SEND_CMD)
	os.system(SEND_CMD)

def build_win_editor(BUILD_CLEAN,CREATE_SHIPPING_EXPORTS):
	os.system("scons platform=windows target=editor arch=x86_64 -j8" + BUILD_CLEAN)
	if CREATE_SHIPPING_EXPORTS == 1:
		SOURCE_FILE="godot.windows.editor.x86_64.exe"
		TARGET_FILE="Tekisasu-Engine.exe"
		move_for_export(SOURCE_FILE, TARGET_FILE)
		SOURCE_FILE="godot.windows.editor.x86_64.console.exe"
		TARGET_FILE="Tekisasu-Engine.console.exe"
		move_for_export(SOURCE_FILE, TARGET_FILE)

def build_win_template_release(BUILD_CLEAN,CREATE_SHIPPING_EXPORTS):
	os.system("scons platform=windows target=template_release arch=x86_64 -j8" + BUILD_CLEAN)
	os.system("scons platform=windows target=template_release arch=x86_32 -j8" + BUILD_CLEAN)
	if CREATE_SHIPPING_EXPORTS == 1:
		# Template release 64-bit
		SOURCE_FILE="godot.windows.template_release.x86_64.exe"
		TARGET_FILE="windows_release_x86_64.exe"
		move_for_export(SOURCE_FILE, TARGET_FILE)
		# Template release 32-bit
		SOURCE_FILE="godot.windows.template_release.x86_32.exe"
		TARGET_FILE="windows_release_x86_32.exe"
		move_for_export(SOURCE_FILE, TARGET_FILE)

def build_win_template_debug(BUILD_CLEAN,CREATE_SHIPPING_EXPORTS):
	os.system("scons platform=windows target=template_debug arch=x86_64 -j8" + BUILD_CLEAN)
	os.system("scons platform=windows target=template_debug arch=x86_32 -j8" + BUILD_CLEAN)
	if CREATE_SHIPPING_EXPORTS == 1:
		# Template debug 64-bit
		SOURCE_FILE="godot.windows.template_debug.x86_64.exe"
		TARGET_FILE="windows_debug_x86_64.exe"
		move_for_export(SOURCE_FILE, TARGET_FILE)
		# Template debug 32-bit
		SOURCE_FILE="godot.windows.template_debug.x86_32.exe"
		TARGET_FILE="windows_debug_x86_32.exe"
		move_for_export(SOURCE_FILE, TARGET_FILE)

# Build
if action == "build" or action == "clean":
	# Set up specific actions for build and clean
	if action == "clean":
		BUILD_CLEAN=" -c"
		print("Will clean instead of build.")
	if action == "build":
		CREATE_SHIPPING_EXPORTS = 1
		print("Will create export templates with final naming.")

	# Run through directives
	if target == "editor" or target == "all":
		build_win_editor(BUILD_CLEAN,CREATE_SHIPPING_EXPORTS)
	if target == "release" or target == "all":
		build_win_template_release(BUILD_CLEAN,CREATE_SHIPPING_EXPORTS)
	if target == "debug" or target == "all":
		build_win_template_debug(BUILD_CLEAN,CREATE_SHIPPING_EXPORTS)