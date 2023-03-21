cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)

project(dvrapp VERSION 0.1.0 LANGUAGES CXX C)

set(TARGET_FILE "dvrapp")

add_definitions(-DTI_816X_BUILD)
add_definitions(-DTI816X_DVR)

INCLUDE_DIRECTORIES(
	"include"
	"include/mcfw"
	"include/mcfw/interfaces"
	"include/mcfw/interfaces/common_def"
	"include/mcfw/interfaces/link_api"
	"include/mcfw/src_linux/links/system"
	"include/mcfw/src_linux/mcfw_api"
	"include/mcfw/src_linux/mcfw_api/usecases"
	"include/ti"
	"libs/libosa/include"
)

FILE(GLOB SRC_FILES "src/*.c")

set(LIB_OSA "osa")
ADD_SUBDIRECTORY("libs/libosa")

set(LIB_TIV "tiv")
ADD_SUBDIRECTORY("libs/libtiv")


ADD_EXECUTABLE(${TARGET_FILE} ${SRC_FILES})

TARGET_LINK_LIBRARIES(${TARGET_FILE} 
	pthread
	m
	asound
	${LIB_OSA}
	${LIB_TIV}

)