if( NOT TARGET GifEncoder )
	get_filename_component( GIFENCODER_SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../src" ABSOLUTE )
	get_filename_component( GIFENCODER_INCLUDE_PATH "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE )
	get_filename_component( CINDER_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE )

	list( APPEND GIFENCODER_SOURCES
		${GIFENCODER_SOURCE_PATH}/GifDitherTypes.h
		${GIFENCODER_SOURCE_PATH}/GifEncoder.cpp
		${GIFENCODER_SOURCE_PATH}/GifEncoder.h
	)

	add_library( GifEncoder ${GIFENCODER_SOURCES} )
	target_include_directories( GifEncoder PUBLIC "${GIFENCODER_SOURCE_PATH}" )

	list( APPEND GIFENCODER_INCLUDE_DIRS
		${GIFENCODER_INCLUDE_PATH}/lib
		${CINDER_PATH}/include
	)
	target_include_directories( GifEncoder SYSTEM BEFORE PUBLIC "${GIFENCODER_INCLUDE_DIRS}" )

	if( NOT TARGET cinder )
		    include( "${CINDER_PATH}/proj/cmake/configure.cmake" )
		    find_package( cinder REQUIRED PATHS
		        "${CINDER_PATH}/${CINDER_LIB_DIRECTORY}"
		        "$ENV{CINDER_PATH}/${CINDER_LIB_DIRECTORY}" )
	endif()

	add_library( freeimage SHARED IMPORTED )
	set_target_properties( freeimage PROPERTIES IMPORTED_LOCATION /usr/lib/libfreeimage.so )

	target_link_libraries( GifEncoder PRIVATE cinder freeimage)

endif()