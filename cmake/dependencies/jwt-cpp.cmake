include(FetchContent)
FetchContent_Declare(
	jwt-cpp
	GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
	GIT_TAG        ce1f9df3a9f861d136d6f0c93a6f811c364d1d3d    
)
FetchContent_MakeAvailable(jwt-cpp)