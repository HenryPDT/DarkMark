# DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>


FIND_PACKAGE ( GTest			QUIET		)
FIND_PACKAGE ( Threads			REQUIRED	)
FIND_PACKAGE ( OpenCV	CONFIG	REQUIRED	) # sudo apt-get install libopencv-dev
FIND_LIBRARY ( DARKNET			darknet		) # https://github.com/stephanecharette/DarkHelp#building-darknet-linux
FIND_LIBRARY ( DARKHELP			darkhelp	) # https://github.com/stephanecharette/DarkHelp#building-darkhelp-linux
FIND_LIBRARY ( LIBMAGIC			magic		) # sudo apt-get install libmagic-dev
FIND_LIBRARY ( POPPLERCPP		poppler-cpp	) # sudo apt-get install libpoppler-cpp-dev

# ONNX Runtime integration
# No system package available; download and install ONNX Runtime manually as described in the README.
FIND_LIBRARY ( ONNXRUNTIME_LIB onnxruntime
    PATHS /usr/onnxruntime/lib /usr/local/onnxruntime/lib
          /usr/lib /usr/local/lib )
FIND_PATH ( ONNXRUNTIME_INCLUDE onnxruntime_c_api.h
    PATHS /usr/onnxruntime/include /usr/local/onnxruntime/include
          /usr/include /usr/local/include
    PATH_SUFFIXES onnxruntime )

INCLUDE_DIRECTORIES ( ${ONNXRUNTIME_INCLUDE} )

IF ( NOT ONNXRUNTIME_LIB )
    MESSAGE ( FATAL_ERROR "ONNX Runtime library not found! Please install ONNX Runtime to /usr/local/onnxruntime or /usr/onnxruntime as described in the README." )
ENDIF()

set ( DM_LIBRARIES Threads::Threads ${DARKHELP} ${DARKNET} ${OpenCV_LIBS} ${LIBMAGIC} ${POPPLERCPP} ${ONNXRUNTIME_LIB} )

INCLUDE_DIRECTORIES ( ${OpenCV_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES ( ${Darknet_INCLUDE_DIR} )
