O2_SETUP(NAME MIDSerializationTest)
set(BUCKET_NAME mid_serialization_test_bucket)

set(LINKDEF src/MIDSimulationLinkDef.h)

if (benchmark_FOUND)
    O2_GENERATE_EXECUTABLE(
            EXE_NAME benchSerialization
            SOURCES benchSerialization.cxx
            BUCKET_NAME ${BUCKET_NAME})
endif ()
