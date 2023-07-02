include(FetchContent)
FetchContent_Declare(
	spdlog
	GIT_REPOSITORY https://github.com/gabime/spdlog.git
	GIT_TAG        5ece88e5a8517fff5f6552d0e13057f38138d95a    
)
FetchContent_MakeAvailable(spdlog)