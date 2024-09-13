import os
import sys
import getopt

# Tekisasu txbuild.py
#   A tool to easily push out builds with specifcs arch and targets, or all
# current windows targets without copy and pasting a bunch of scons commands.
#
# TODO:
# 1) replace os.system() calls to CMD fs commands with native python functions
# 2) add support for WSL exec to build linux targets
# 3) add support for Android builds

# In python functions, use "/", it is cross platform.
# But where I use (lazily) use os.system(), use F_SLASH
if os.name == "nt":
	F_SLASH = "\\"
else:
	F_SLASH = "/"

# Set Tekisasu encryption key
os.system("set SCRIPT_AES256_ENCRYPTION_KEY=7352cb11b57dc27b9dc5cef18c7076c39d54cf9302ac7a6d9a2ef8d450a63bb6")

# Set some common globals
fail_exist_tools = 0
fail_exist_win64debug = 0
fail_exist_win32debug = 0
fail_exist_win64release = 0
fail_exist_win32release = 0
fail_exist_uwp_amd64debug = 0
fail_exist_uwp_amd64 = 0
fail_exist_uwp_arm64debug = 0
fail_exist_uwp_arm64 = 0
miscerror = "else"
BIN_FOLDER = "bin"
CLEAN_FLAGS = "-c "
EXPORT_FOLDER = BIN_FOLDER + F_SLASH + "export"

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
		if line_arg == "release" or line_arg == "debug" or line_arg == "tools":
			target = line_arg

	if index == 3: # Check for {arch}
		if line_arg == "32" or line_arg == "64":
			cpuarc = line_arg

	if index == 4: # Check for optional UWP build
		if line_arg == "uwp":
			uwp = line_arg

	index = index + 1

print_tekisasu_header()

def tekisasu_input(action, target, cpuarc, uwp):
#	print ("The choice was: ", action)
	if action == "build":
		# parameters : RELEASE_STATUS.bool, CLEAN_STATUS.bool
		if target == "" and cpuarc == "": # clean all
			build_win64_tools(False,False)
			build_win64_release(True,False)
			build_win32_release(True,False)
			build_win64_debug(False,False)
			build_win32_debug(False,False)
			build_uwp_amd64_release(True,False)
			build_uwp_arm64_release(True,False)
			build_uwp_amd64_debug(True,False)
			build_uwp_arm64_debug(True,False)			
		elif target == "release" and cpuarc == "64":
			build_win64_release(True,False)
		elif target == "release" and cpuarc == "32":
			build_win32_release(True,False)
		elif target == "debug" and cpuarc == "64":
			build_win64_debug(False,False)
		elif target == "debug" and cpuarc == "32":
			build_win32_debug(False,False)
		elif target == "tools" and cpuarc == "": 
			build_win64_tools(False,False)
		elif target == "tools" and cpuarc == "64":
			build_win64_tools(False,False)
		elif target == "release" and cpuarc == "64" and uwp == "uwp":
			build_uwp_amd64_release(True,False)
		elif target == "release" and cpuarc == "64" and uwp == "uwp_arm":
			build_uwp_arm64_release(True,False)
		elif target == "debug" and cpuarc == "64" and uwp == "uwp":
			build_uwp_amd64_debug(True,False)
		elif target == "debug" and cpuarc == "64" and uwp == "uwp_arm":
			build_uwp_arm64_debug(True,False)					
		else:
			inputerror_msg()
	elif action == "clean":
		# parameters : RELEASE_STATUS.bool, CLEAN_STATUS.bool
		if target == "" and cpuarc == "": # clean all
			build_win64_tools(False,True)
			build_win64_release(True,True)
			build_win32_release(True,True)
			build_win64_debug(False,True)
			build_win32_debug(False,True)
			build_uwp_amd64_release(True,True)
			build_uwp_arm64_release(True,True)
			build_uwp_amd64_debug(False,True)
			build_uwp_arm64_debug(False,True)				
		elif target == "release" and cpuarc == "64":
			build_win64_release(True,True)
		elif target == "release" and cpuarc == "32":
			build_win32_release(True,True)
		elif target == "debug" and cpuarc == "64":
			build_win64_debug(False,True)
		elif target == "debug" and cpuarc == "32":
			build_win32_debug(False,True)
		elif target == "tools" and cpuarc == "": 
			build_win64_tools(False,True)
		elif target == "tools" and cpuarc == "64":
			build_win64_tools(False,True)
		elif target == "release" and cpuarc == "64" and uwp == "uwp":
			build_uwp_amd64_release(True,True)
		elif target == "release" and cpuarc == "64" and uwp == "uwp_arm":
			build_uwp_arm64_release(True,True)
		elif target == "debug" and cpuarc == "64" and uwp == "uwp":
			build_uwp_amd64_debug(False,True)
		elif target == "debug" and cpuarc == "64" and uwp == "uwp_arm":
			build_uwp_arm64_debug(False,True)				
		else:
			inputerror_msg()
	else:
		inputerror_msg()
		print ("")

def inputerror_msg():
	print ("\ntxbuild.py - Usage:")
	print ("-------------------")
	print ("To build ALL:\t\t\t$ python txbuild.py build")
	print ("To build RELEASE 64:\t\t$ python txbuild.py build release 64")
	print ("To build RELEASE 32:\t\t$ python txbuild.py build release 32")
	print ("To build DEBUG 64:\t\t$ python txbuild.py build debug 64")
	print ("To build DEBUG 32:\t\t$ python txbuild.py build debug 32")
	print ("To build DEBUG 32:\t\t$ python txbuild.py build tools 64")
	print ("To build RELEASE 64 (UWP):\t\t$ python txbuild.py build release 64 uwp")
	print ("To build DEBUG 64 (UWP):\t\t$ python txbuild.py build debug 64 uwp")
	print ("To build RELEASE 64 (UWP):\t\t$ python txbuild.py build release 64 uwp_arm")
	print ("To build DEBUG 64 (UWP):\t\t$ python txbuild.py build debug 64 uwp_arm")
	print ("To clean ALL:\t\t\t$ python txbuild.py clean")
	print ("To clean RELEASE 64:\t\t$ python txbuild.py clean release 64")
	print ("To clean RELEASE 32:\t\t$ python txbuild.py clean release 32")
	print ("To clean DEBUG 64:\t\t$ python txbuild.py clean debug 64")
	print ("To clean DEBUG 32:\t\t$ python txbuild.py clean debug 32")
	print ("To clean DEBUG 32:\t\t$ python txbuild.py clean tools 64\n")
	print ("To clean RELEASE 64 (UWP):\t\t$ python txbuild.py clean release 64 uwp")
	print ("To clean DEBUG 64 (UWP):\t\t$ python txbuild.py clean debug 64 uwp")
	print ("To clean RELEASE 64 (UWP):\t\t$ python txbuild.py clean release 64 uwp_arm")
	print ("To clean DEBUG 64 (UWP):\t\t$ python txbuild.py clean debug 64 uwp_arm")	

def build_win32_release(RELEASE_STATUS, CLEAN_STATUS):
	SOURCE_FILE = "godot.windows.opt.32.exe"
	TARGET_FILE = "windows_32_release.exe"
	BUILD_FLAGS = "platform=windows -j8 tools=no target=release bits=32 "
	pass_to_scons(SOURCE_FILE, TARGET_FILE, BUILD_FLAGS, RELEASE_STATUS, CLEAN_STATUS)

def build_win64_release(RELEASE_STATUS, CLEAN_STATUS):
	SOURCE_FILE = "godot.windows.opt.64.exe"
	TARGET_FILE = "windows_64_release.exe"
	BUILD_FLAGS = "platform=windows -j8 tools=no target=release bits=64 "
	pass_to_scons(SOURCE_FILE, TARGET_FILE, BUILD_FLAGS, RELEASE_STATUS, CLEAN_STATUS)

def build_uwp_arm64_release(RELEASE_STATUS, CLEAN_STATUS):
	BUILD_FLAGS = "platform=uwp -j8 tools=no target=release bits=64 "
	os.system("C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat amd64 uwp 10.0.16299.0")
	pass_to_scons_uwp(BUILD_FLAGS, RELEASE_STATUS, CLEAN_STATUS)	

def build_win32_debug(RELEASE_STATUS, CLEAN_STATUS):
	SOURCE_FILE = "godot.windows.opt.debug.32.exe"
	TARGET_FILE = "windows_32_debug.exe"
	BUILD_FLAGS = "platform=windows -j8 tools=no target=release_debug bits=32 "
	pass_to_scons(SOURCE_FILE, TARGET_FILE, BUILD_FLAGS, RELEASE_STATUS, CLEAN_STATUS)

def build_win64_debug(RELEASE_STATUS, CLEAN_STATUS):
	SOURCE_FILE = "godot.windows.opt.debug.64.exe"
	TARGET_FILE = "windows_64_debug.exe"
	BUILD_FLAGS = "platform=windows -j8 tools=no target=release_debug bits=64 "
	pass_to_scons(SOURCE_FILE, TARGET_FILE, BUILD_FLAGS, RELEASE_STATUS, CLEAN_STATUS)	

def build_win64_tools(RELEASE_STATUS, CLEAN_STATUS):
	SOURCE_FILE = "godot.windows.opt.tools.64.exe"
	TARGET_FILE = "Tekisasu-Engine.exe"
	BUILD_FLAGS = "platform=windows -j8 tools=yes target=release_debug bits=64 "
	pass_to_scons(SOURCE_FILE, TARGET_FILE, BUILD_FLAGS, RELEASE_STATUS, CLEAN_STATUS)		


def pass_to_scons(SOURCE_FILE, TARGET_FILE, BUILD_FLAGS, RELEASE_STATUS, CLEAN_STATUS):
	# Enable custom release build flags
	if RELEASE_STATUS == True:
		add_release_flags(True)

	# Make sure EXPORT dir exist
	if os.path.exists(EXPORT_FOLDER):
		print("Export folder " + EXPORT_FOLDER + "exists!")
	else:
		os.system("mkdir " + EXPORT_FOLDER)	

	# Remove last pass of specific build
	if os.path.exists(EXPORT_FOLDER + "/" + TARGET_FILE):
		os.system("del " + EXPORT_FOLDER + F_SLASH + TARGET_FILE) # bin/export/windows_32_release.exe
	
	# Do a scons clean?
	if CLEAN_STATUS == True:
		BUILD_FLAGS = BUILD_FLAGS + " " + CLEAN_FLAGS

	# SCONS BUILD TIME, BABY
	os.system("scons " + BUILD_FLAGS)
	print (BUILD_FLAGS)

	# If Scons build successfully, copy SOURCE_FILE to TARGET_FILE in EXPORT_FOLDER dir
	if os.path.exists(BIN_FOLDER + "/" + SOURCE_FILE):
		# Copy bin/SconsCreatedFile.exe to export/PrettyNameForTemplates.exe
		SEND_CMD=("echo F | xcopy " + BIN_FOLDER + F_SLASH + SOURCE_FILE + " " + EXPORT_FOLDER + F_SLASH + TARGET_FILE)
		print("SEND_CMD : " + SEND_CMD)
		os.system(SEND_CMD)

	# Disable custom release build flags
	if RELEASE_STATUS == True:
		add_release_flags(False)

def pass_to_scons_uwp(BUILD_FLAGS, RELEASE_STATUS, CLEAN_STATUS):
	# Enable custom release build flags
	if RELEASE_STATUS == True:
		add_release_flags(True)

	# Make sure EXPORT dir exist
	if os.path.exists(EXPORT_FOLDER):
		print("Export folder " + EXPORT_FOLDER + "exists!")
	else:
		os.system("mkdir " + EXPORT_FOLDER)	

	# Remove last pass of specific build
	if os.path.exists(EXPORT_FOLDER + "/" + TARGET_FILE):
		os.system("del " + EXPORT_FOLDER + F_SLASH + TARGET_FILE) # bin/export/windows_32_release.exe
	
	# Do a scons clean?
	if CLEAN_STATUS == True:
		BUILD_FLAGS = BUILD_FLAGS + " " + CLEAN_FLAGS

	# SCONS BUILD TIME, BABY
	os.system("scons " + BUILD_FLAGS)
	print (BUILD_FLAGS)

	# If Scons build successfully, copy SOURCE_FILE to TARGET_FILE in EXPORT_FOLDER dir
	if os.path.exists(BIN_FOLDER + "/" + SOURCE_FILE):
		# Copy bin/SconsCreatedFile.exe to export/PrettyNameForTemplates.exe
		SEND_CMD=("echo F | xcopy " + BIN_FOLDER + F_SLASH + SOURCE_FILE + " " + EXPORT_FOLDER + F_SLASH + TARGET_FILE)
		print("SEND_CMD : " + SEND_CMD)
		os.system(SEND_CMD)

	# Disable custom release build flags
	if RELEASE_STATUS == True:
		add_release_flags(False)		

def add_release_flags(bidding):
	if bidding == True:
		# clone custom.py into place
		os.system("echo F | xcopy .tekisasu-custom.py custom.py")
	elif bidding == False:
		# remove custom.py clone
		os.system("del custom.py")
	else:
		sys.exit("\nadd_release_flags() did not get a valid input.\n")

tekisasu_input(action, target, cpuarc, uwp)
