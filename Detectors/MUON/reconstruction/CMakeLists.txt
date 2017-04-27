set(MODULE_NAME "MUONReconstruction")

O2_SETUP(NAME ${MODULE_NAME})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/runMIDclustering.json ${CMAKE_BINARY_DIR}/bin/config/runMIDclustering.json)

link_directories( ${LINK_DIRECTORIES})

set(SRCS
   src/MIDclustering.cxx
)

set(HEADERS
   include/${MODULE_NAME}/MIDclustering.h
)

Set(LIBRARY_NAME ${MODULE_NAME})
set(BUCKET_NAME muon_reconstruction_bucket)

O2_GENERATE_LIBRARY()

Set(Exe_Names
  runMIDclustering
)

set(Exe_Source
  src/runMIDclustering.cxx
)

# todo we repeat ourselves because the above macro dares deleting the variables we pass to it.
set(LIBRARY_NAME ${MODULE_NAME})
set(BUCKET_NAME muon_reconstruction_bucket)

list(LENGTH Exe_Names _length)
math(EXPR _length ${_length}-1)

ForEach (_file RANGE 0 ${_length}) # loop over a range because we traverse 2 lists and not 1
list(GET Exe_Names ${_file} _name)
list(GET Exe_Source ${_file} _src)
O2_GENERATE_EXECUTABLE(
EXE_NAME ${_name}
SOURCES ${_src}
MODULE_LIBRARY_NAME ${LIBRARY_NAME}
BUCKET_NAME ${BUCKET_NAME}
)
EndForEach (_file RANGE 0 ${_length})

install(FILES ${CMAKE_BINARY_DIR}/bin/config/runMIDclustering.json
        DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/config)
