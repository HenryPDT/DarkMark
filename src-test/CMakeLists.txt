# DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

# Filename sort tests are standalone: they do not need JUCE, OpenCV, or the rest of DarkMark.
ADD_EXECUTABLE ( darkmark_filename_sort_tests
	TestMain.cpp
	TestFilenameSort.cpp
	${CMAKE_SOURCE_DIR}/src-tools/FilenameSort.cpp
)

TARGET_INCLUDE_DIRECTORIES ( darkmark_filename_sort_tests PRIVATE
	${CMAKE_SOURCE_DIR}/src-tools
)

IF ( TARGET GTest::gtest )
	TARGET_LINK_LIBRARIES ( darkmark_filename_sort_tests GTest::gtest ${CMAKE_THREAD_LIBS_INIT} )
ELSE ()
	TARGET_LINK_LIBRARIES ( darkmark_filename_sort_tests ${GTEST_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} )
ENDIF ()

TARGET_COMPILE_FEATURES ( darkmark_filename_sort_tests PRIVATE cxx_std_17 )

ADD_TEST ( NAME darkmark_filename_sort_tests COMMAND darkmark_filename_sort_tests )


#FILE ( GLOB TEST_SOURCE *.cpp		)
#LIST ( SORT TEST_SOURCE				)
#
#ADD_EXECUTABLE ( darkmark_tests ${TEST_SOURCE}
#			$<TARGET_OBJECTS:dm_tools>
#			$<TARGET_OBJECTS:dm_darknet>
#			$<TARGET_OBJECTS:dm_launcher>
#			$<TARGET_OBJECTS:dm_wnd>
#			)
#TARGET_LINK_LIBRARIES ( darkmark_tests dm_juce ${CMAKE_THREAD_LIBS_INIT} ${GTEST_LIBRARIES} ${DM_LIBRARIES} pthread )
#
#ADD_TEST ( NAME darkmark_tests COMMAND darkmark_tests )
