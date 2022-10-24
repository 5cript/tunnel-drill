include(FetchContent)
FetchContent_Declare(
	jwt-cpp
	GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
	GIT_TAG        03ad09b996fb85f8fc6645795f89c39940bc4902    
)
FetchContent_MakeAvailable(jwt-cpp)