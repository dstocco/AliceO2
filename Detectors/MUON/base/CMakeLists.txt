set(MODULE_NAME "MUONBase")

O2_SETUP(NAME ${MODULE_NAME})

link_directories( ${LINK_DIRECTORIES})

set(SRCS
   src/Mapping.cxx
)

set(HEADERS
   include/${MODULE_NAME}/Mapping.h
)
Set(LIBRARY_NAME ${MODULE_NAME})
set(BUCKET_NAME muon_base_bucket)

O2_GENERATE_LIBRARY()
