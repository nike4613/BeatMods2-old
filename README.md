# BeatMods2

BeatMods 2 API

## Contributing

### Requirements

* PostgreSQL
* CMake
* Boost::program_options and Boost::filesystem (for GraphQLGen)
* A C/C++ toolchain with C++17 support (LLVM Clang 9 or GCC 9 recommended)

### Getting started

1. Clone the repo *with submodules* `git clone --recurse-submodules -j8 https://github.com/nike4613/BeatMods2`
2. Prepare the database `psql -f BeatMods2/BeatMods2.sql`
