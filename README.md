# AWS GameKit C++ SDK

## Prerequisites
Download and Install [CMake v3.21](https://cmake.org/download/)
Some versions of aws-sdk-cpp have a [known issue](https://github.com/aws/aws-sdk-cpp/issues/1820) compiling with CMake 3.22.
Build and Install the dependencies in the following order:

1. AWS SDK C++
2. yaml-cpp
3. GTest
4. Boost
5. Pybind

### AWS SDK C++
Note: `BUILD_TYPE` is either `Debug` or `Release`

1. Clone the AWS SDK C++: `git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp` (make sure you're on tag 1.9.162 or later)
2. Install CMake 3.21: https://cmake.org/download/
3. Use the Windows Command Prompt (or the Legacy command prompt in Windows Terminal) for the following:
4. Create a build directory called AWSSDK in a separate directory than where you cloned the SDK.

```
mkdir AWSSDK
```

5. Change directory into AWSSDK:

```
cd AWSSDK
```

6. Generate the SDK Project files (~5 min):

```
cmake <YOUR_DEVELOPMENT_PATH>\aws-sdk-cpp -G "Visual Studio 16 2019" -DCMAKE_BUILD_TYPE=<BUILD_TYPE> -DFORCE_SHARED_CRT=ON -DBUILD_SHARED_LIBS=ON -DMINIMIZE_SIZE=ON -DCMAKE_INSTALL_PREFIX=<YOUR_DEVELOPMENT_PATH>\AWSSDK\install\x86_64\windows\<BUILD_TYPE>
```

7. Build the SDK (~10 min depends on your workstation specs):

```
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\msbuild.exe" ALL_BUILD.vcxproj /p:Configuration=<BUILD_TYPE> -maxcpucount
```

8. Install the SDK to the <YOUR_DEVELOPMENT_PATH>\AWSSDK\install location

```
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\msbuild.exe" INSTALL.vcxproj /p:Configuration=<BUILD_TYPE> -maxcpucount
```

### yaml-cpp
Note: `BUILD_TYPE` is either `Debug` or `Release`
AWS GameKit uses a fixed version of yaml-cpp: commit `2f899756`

1. `git clone https://github.com/jbeder/yaml-cpp/`
1. `cd yaml-cpp`
1. `git checkout -b gamekit_fixed_version 2f899756`
1. `mkdir build`
1. `cd build`
1. `cmake -DYAML_BUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=<DIRECTORY_IN_STEP_TWO>\install\<BUILD_TYPE> ..`
1. `"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\msbuild.exe" INSTALL.vcxproj /p:Configuration=<BUILD_TYPE> -maxcpucount`

### GTest
1. Create a directory to contain the Google test repo (example: `D:\development`), navigate into it and clone: `git clone https://github.com/google/googletest`. You should end up having a directory tree like `D:\development\googletest`.
2. Navigate into the googletest directory inside newly cloned googletest repo, your cwd should look like `D:\development\googletest\googletest`.
3. Run CMake to generate the configuration files: `cmake -DBUILD_SHARED_LIBS=ON -Dgtest_force_shared_crt=ON -DCMAKE_INSTALL_PREFIX=<path to gtest repo>\build\install -DGTEST_CREATE_SHARED_LIBRARY=1 ..`
4. Build with `"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\msbuild.exe" INSTALL.vcxproj /p:Configuration=Debug -maxcpucount`


### Boost
1. Download and extract https://www.boost.org/users/history/version_1_76_0.html
2. cd into the directory you extracted Boost
3. run `bootstrap.bat`
4. run `.\b2 link=static`

### pybind11
Note: `BUILD_TYPE` is either `Debug` or `Release`
- Make sure `python3` is on your `PATH`

1. Clone https://github.com/pybind/pybind11/
2. cd into the directory where you cloned pybind11
3. `cmake -DBoost_INCLUDE_DIR=<BOOST_PARENT_DIR>\boost_1_76_0 -DCMAKE_INSTALL_PREFIX=<PYBIND_PARENT_DIR>\pybind11\install\<BUILD_TYPE> .`
4. `"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\msbuild.exe" INSTALL.vcxproj /p:Configuration=<BUILD_TYPE> -maxcpucount`

## Create/Update Projects Using CMake
### Windows with Visual Studio 16 2019

Note: `BUILD_TYPE` is either `Debug` or `Release`

Run all script commands from the repository root directory.

**Run:** `scripts/Win64/regenerate_projects.bat`

The script will prompt you to set the following environment variables:

- GAMEKIT_AWSSDK_PATH (ex: D:\development\AWSSDK)
- GAMEKIT_BOOST_PATH (ex: D:\development\boost_1_76_0)
- GAMEKIT_YAMLCPP_PATH (ex: D:\development\yaml-cpp)
- GAMEKIT_GTEST_PATH (ex: D:\development\googletest)
- GAMEKIT_PYBIND_PATH (ex: D:\development\pybind11)
  Once set, it won't prompt you again.

It will also ask you for the `BUILD_TYPE`. Optionally, you can pass it as the first argument to the command.

**Example:** `scripts/Win64/regenerate_projects.bat Debug`

## Build Project

### Windows
- Build ALL_BUILD and/or INSTALL solution in visual studio, or do the following steps from the command line.
- To build: `"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\msbuild.exe" ALL_BUILD.vcxproj -p:Configuration=<BUILD_TYPE> -m`
- To consolidate all libs in install dir: `"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\msbuild.exe" INSTALL.vcxproj -p:Configuration=<BUILD_TYPE> -m`

## Run Unit Tests
1. cd into `tests\<BUILD_TYPE>` directory
2. Run `aws-gamekit-cpp-tests.exe`

## Copy GameKit Headers to Plugin
**Run:** `scripts/Win64/copyheaders.bat <SANDBOX_GAME_PATH>`
**Example:** `scripts/Win64/copyheaders.bat D:\development\SampleUnrealGame`

## Copy Binaries to Plugin Libraries
**Run:** `python scripts/Win64/refresh_plugin.py <PLUGIN_FULL_PATH> <GAME_ENGINE> <BUILD_TYPE>`
**Example:** `python scripts/Win64/refresh_plugin.py D:\development\SampleUnrealGame\Plugins Unreal Debug`
