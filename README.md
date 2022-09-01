# AWS GameKit C++ SDK

## Prerequisites

Download and Install [CMake v3.21](https://github.com/Kitware/CMake/releases/tag/v3.21.6)
Some versions of aws-sdk-cpp have a [known issue](https://github.com/aws/aws-sdk-cpp/issues/1820) compiling with CMake 3.22 or later.

### Enable Windows long path support
For the build steps of the dependencies and the SDK to work properly on Windows,
you will have to enable long path support.

Run the following command in an Administrator level Powershell terminal:
```powershell
Set-ItemProperty `
  -Path HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem `
  -Name LongPathsEnabled -Value 1
```

### Dependencies

Install the dependencies in the following order:

1. AWS SDK C++
2. yaml-cpp
3. GTest
4. Boost
5. Pybind

### Third party Tools for iOS Builds

**iOS Toolchain file:** Get the toolchain file from https://github.com/leetal/ios-cmake.

**cURL, OpenSSL, nghttp:** Get the script that will build cURL, OpenSSL, and nghttp for iOS from: https://github.com/jasonacox/Build-OpenSSL-cURL

- Needs Xcode 13.1 or above
  - If multiple Xcode versions are installed,
    run: `sudo xcode-select -s <YOUR_XCODE13_APP>/Contents/Developer`
    example: `sudo xcode-select -s /Applications/Xcode13.app/Contents/Developer`
- Run: `./build.sh`
  - If needed the '-e' flag can be used to compile with OpenSSL engine support although this may cause some 'undefined symbols for architecture' depending on your OS version.

**Boost:** Get the shell script that will download, bootstrap, and build Boost for iOS from: https://github.com/faithfracture/Apple-Boost-BuildScript/blob/master/boost.sh.

- The variables in boost.sh should be replaced with the following to avoid building unecessary libraries 
  - BOOST_LIBS=("regex" "filesystem" "iostreams")
  - ALL_BOOST_LIBS_1_68=("regex" "filesystem" "iostreams")
  - ALL_BOOST_LIBS_1_69=("regex" "filesystem" "iostreams")

### Third party Tools for Android Builds (On Windows)

**Ninja**: Available as part of Visual Studio 2019

**Developer Command Prompt for VS 2019**: Available as part of Visual Studio 2019. Note: all Android build commands should be executed from this console. Usually located at `"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat"`

- For Ninja build tool, you may need to use the dev command prompt from VS Build Tools instead of VS Community edition, if you do not have VS Professional installed. This is especially important when using an EC2 instance instead of your local machine.
- You can launch the dev cmd prompt using the `LaunchDevCmd.bat` in the above path.

**Android NDK r21d for Unreal**: Use Unreal's NDK, follow these steps to install: https://docs.unrealengine.com/4.27/en-US/SharingAndReleasing/Mobile/Android/Setup/AndroidStudio/

**Android NDK with Unity**: (API Level 24, NDK version 21), you can use the NDK supplied by Unity. To do this, set the following (replace <Unity Version> with the Unity version you are using): 
```bat
set ndkroot=C:\Program Files\Unity\Hub\Editor\<UNITY VERSION>\Editor\Data\PlaybackEngines\AndroidPlayer\NDK
```

**Android Studio Version 4**: Follow these steps to install: https://docs.unrealengine.com/4.27/en-US/SharingAndReleasing/Mobile/Android/Setup/AndroidStudio/

After following the steps to install Android Studio 4.0 and downloaded the NDK using Unreal´s script, open Android Studio again and make the following modifications:

- Install Android SDK Build Tools 30.0.3 in Configure > Appeareance and Behavior > System Settings > Android SDK > SDK Tools. You might need to uncheck "Hide obsolete packages"
  - This may also be accessed from the startup screen: Configure → SDK Manager → SDK Tools
  - You may need to check `Show Package Details` to see all the versions of the `Android SDK Build Tools`
- Uninstall newer versions of the SDK Build Tools

### AWS SDK C++

#### Build and Install AWS SDK C++

Note: `BUILD_TYPE` is either `Debug` or `Release`
Note: Make sure your development path is absolute, not relative, the install might silently fail otherwise.

##### Windows

1. Clone the AWS SDK C++: `git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp` (make sure you're on tag 1.9.162 or later)
2. Install CMake 3.21: https://github.com/Kitware/CMake/releases/tag/v3.21.6
3. Use the Windows Command Prompt (or the Legacy command prompt in Windows Terminal) for the following:
4. Create a build directory called AWSSDK in a separate directory than where you cloned the SDK.

    ```bat
    mkdir AWSSDK
    ```

5. Change directory into AWSSDK:

    ```bat
    cd AWSSDK
    ```

6. Generate the SDK Project files (~5 min):

    ```bat
    cmake <YOUR_DEVELOPMENT_PATH>\aws-sdk-cpp -G "Visual Studio 16 2019" -DCMAKE_BUILD_TYPE=<BUILD_TYPE> -DFORCE_SHARED_CRT=ON -DBUILD_SHARED_LIBS=ON -DMINIMIZE_SIZE=ON -DCMAKE_INSTALL_PREFIX=<YOUR_DEVELOPMENT_PATH>\AWSSDK\install\x86_64\windows\<BUILD_TYPE>
    ```

7. Build the SDK (~10 min depends on your workstation specs):

    ```bat
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\msbuild.exe" ALL_BUILD.vcxproj /p:Configuration=<BUILD_TYPE> -maxcpucount
    ```

8. Install the SDK to the <YOUR_DEVELOPMENT_PATH>\AWSSDK\install location

    ```bat
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\msbuild.exe" INSTALL.vcxproj /p:Configuration=<BUILD_TYPE> -maxcpucount
    ```

##### macOS

1. Clone the AWS SDK C++: `git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp` inside `~/<YOUR_DEVELOPMENT_PATH>` (make sure you're on tag 1.9.188 for macOS)
   1. Make sure to use the same tag for `aws-sdk-cpp` for all builds (macOS and iOS)
2. Install CMake: https://cmake.org/download/
3. For both macOS and iOS, you will need to pull in the tag `v0.17.13` for `crt/aws-crt-cpp` submodule of `aws-sdk-cpp`. This fixes a bug for iOS builds related to `SetKeychainPath`. Make sure to `init` and recursive `update` further submodules of `aws-crt-cpp`.

##### For macOS SHARED LIBRARIES

3. Create a build directory called AWSSDK_mac e.g., `~/development/AWSSDK_mac`

    ```bat
    mkdir AWSSDK_mac
    ```

4. Change directory into AWSSDK_mac:

    ```bat
    cd AWSSDK_mac
    ```

5. Run CMake

    ```bat
    rm -rf CMakeFiles/; rm -rf CMakeScripts/; rm CMakeCache.txt; rm -rf crt/aws-crt-cpp/CMakeFiles/; rm -rf crt/aws-crt-cpp/CMakeScripts/;

    cmake ~/development/aws-sdk-cpp -G Xcode -DCMAKE_BUILD_TYPE=<BUILD_TYPE> -DBUILD_ONLY="core;apigateway;cloudformation;cognito-idp;lambda;s3;ssm;secretsmanager;sts" -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=<YOUR_DEVELOPMENT_PATH>/AWSSDK_mac/install/x86_64/macos/<BUILD_TYPE> -DTARGET_ARCH="APPLE" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
    ```

6. Build and install

    ```bat
    xcodebuild -parallelizeTargets -configuration <BUILD_TYPE> -target ALL_BUILD
    xcodebuild -parallelizeTargets -target install
    ```

##### For iOS Static Libraries

3. Create a build directory called AWSSDK_ios e.g., `~/development/AWSSDK_ios`

    ```bat
    mkdir AWSSDK_ios
    ```

4. Change directory into AWSSDK_ios:

    ```bat
    cd AWSSDK_ios
    ```

5.  Run CMake
    Use XCode 12 to compile for iOS 14.0 which is the highest version supported by Unreal 4.27.

    - If multiple Xcode versions are installed,

    Run: `sudo xcode-select -s <YOUR_XCODE12_APP>/Contents/Developer`
    Example: `sudo xcode-select -s /Applications/Xcode12.app/Contents/Developer`

    ```bat
    cmake ~/development/aws-sdk-cpp -G Xcode -DCMAKE_TOOLCHAIN_FILE=<YOUR_DEVELOPMENT_PATH>/ios-cmake/ios.toolchain.cmake -DPLATFORM=OS64 -DCMAKE_BUILD_TYPE=<BUILD_TYPE> -DBUILD_ONLY="core;apigateway;cloudformation;cognito-idp;lambda;s3;ssm;secretsmanager;sts" -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=<YOUR_DEVELOPMENT_PATH>/AWSSDK_ios/install/arm64/ios/<BUILD_TYPE> -DENABLE_TESTING=NO -DCURL_LIBRARY=<YOUR_DEVELOPMENT_PATH>/Build-OpenSSL-cURL/curl/lib/libcurl.a -DCURL_INCLUDE_DIR=<YOUR_DEVELOPMENT_PATH>/Build-OpenSSL-cURL/curl/include -DUSE_OPENSSL=ON -DENABLE_PROXY_INTEGRATION_TESTS=OFF -DENABLE_COMMONCRYPTO_ENCRYPTION=OFF -DENABLE_OPENSSL_ENCRYPTION=ON -DOPENSSL_ROOT_DIR=<YOUR_DEVELOPMENT_PATH>/Build-OpenSSL-cURL/openssl/iOS -DCMAKE_CXX_FLAGS="-Wno-shorten-64-to-32" -DENABLE_CURL_CLIENT=ON -DENABLE_CURL_LOGGING=OFF
    ```

6. Build and install

```
xcodebuild -parallelizeTargets -configuration <BUILD_TYPE> -target ALL_BUILD
xcodebuild -parallelizeTargets -target install
```

##### For Android Libraries

1. Set these environment variables inside a Developer Command Prompt for VS 2019.

    ```bat
    set ANDROID_API_LEVEL=24
    set BUILD_TYPE=Debug
    set ARCH=arm
    set PLATFORM=android
    ```

  * For Unreal, set:
    ```bat
    set BUILD_SHARED=OFF
    set STL_TYPE=c++_static
    ```
  * For Unity, set:
    ```bat
    set BUILD_SHARED=ON
    set STL_TYPE=c++_shared 
    ```

2. Create a build directory called AWSSDK_android

    ```bat
    mkdir AWSSDK_android
    ```

3. Change directory into AWSSDK_android:

    ```bat
    cd AWSSDK_android
    ```

4. Run CMake

    ```bat
    cmake <YOUR_DEVELOPMENT_PATH>\aws-sdk-cpp -DNDK_DIR=%NDKROOT% -DBUILD_ONLY="core;apigateway;cloudformation;cognito-idp;lambda;s3;ssm;secretsmanager;sts" -DBUILD_SHARED_LIBS=%BUILD_SHARED% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCUSTOM_MEMORY_MANAGEMENT=ON -DTARGET_ARCH=ANDROID -DANDROID_NATIVE_API_LEVEL=%ANDROID_API_LEVEL% -DANDROID_BUILD_CURL=1 -DANDROID_BUILD_OPENSSL=1 -DANDROID_BUILD_ZLIB=1 -DBUILD_ZLIB=1 -DCMAKE_INSTALL_PREFIX=<YOUR_DEVELOPMENT_PATH>\AWSSDK_android\install\%ARCH%\%PLATFORM%\%BUILD_TYPE% -G "Ninja" -DANDROID_STL=%STL_TYPE% -DANDROID_PLATFORM=android-%ANDROID_API_LEVEL% -DENABLE_TESTING=NO
    ```

5. Build and install

    ```bat
    cmake --build .
    cmake --build . --target install
    ```

6. Before building `Release` after you have built `Debug`, make sure to delete all `CMakeCache` files and also the folder `external-build` in `AWSSDK_android` root path.

### yaml-cpp

AWS GameKit uses a fixed version of yaml-cpp: commit `2f899756`

#### Windows Build and Install

1. `git clone https://github.com/jbeder/yaml-cpp/`
1. `cd yaml-cpp`
1. `git checkout -b gamekit_fixed_version 2f899756`
1. `mkdir build`
1. `cd build`
1. `cmake -DYAML_BUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=<DIRECTORY_IN_STEP_TWO>\install\<BUILD_TYPE> ..`
1. `"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\msbuild.exe" INSTALL.vcxproj /p:Configuration=<BUILD_TYPE> -maxcpucount`

#### macOS Build and Install

1. `git clone https://github.com/jbeder/yaml-cpp/`
1. `cd yaml-cpp`
1. `git checkout -b gamekit_fixed_version 2f899756`
1. `mkdir build_mac`
1. `cd build_mac`
1. `cmake -G Xcode -DYAML_BUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=install/x86_64/<BUILD_TYPE> ..`
1. `xcodebuild -parallelizeTargets -configuration <BUILD_TYPE> -target ALL_BUILD`
1. `xcodebuild -parallelizeTargets -target install`

#### iOS Build and Install

1. `git clone https://github.com/jbeder/yaml-cpp/` (if not already cloned for `macos`)
1. `cd yaml-cpp`
1. `git checkout -b gamekit_fixed_version 2f899756`
1. `mkdir build_ios` (same level as `macos`)
1. `cd build_ios`
1. `cmake -G Xcode -DCMAKE_TOOLCHAIN_FILE=<YOUR_DEVELOPMENT_PATH>/ios-cmake/ios.toolchain.cmake -DPLATFORM=OS64 -DCMAKE_BUILD_TYPE=<BUILD_TYPE> -DCMAKE_INSTALL_PREFIX=install/arm64/<BUILD_TYPE> ..`
1. `xcodebuild -parallelizeTargets -configuration <BUILD_TYPE> -target ALL_BUILD`
1. `xcodebuild -parallelizeTargets -target install`

#### Android Build and Install

1. Set these environment variables inside a Developer Command Prompt for VS 2019.

    ```bat
    set ANDROID_API_LEVEL=24
    set BUILD_TYPE=Debug
    set ARCH=arm
    set PLATFORM=android
    ```

  * For Unreal, set:
    ```bat
    set BUILD_SHARED=OFF
    set STL_TYPE=c++_static
    ```
  * For Unity, set:
    ```bat
    set BUILD_SHARED=ON
    set STL_TYPE=c++_shared 
    ```

1. `git clone https://github.com/jbeder/yaml-cpp/`
1. `cd yaml-cpp`
1. `git checkout -b gamekit_fixed_version 2f899756`
1. `mkdir build_android`
1. `cd build_android`

    ```bat
    cmake .. -DCMAKE_TOOLCHAIN_FILE=%NDKROOT%\build\cmake\android.toolchain.cmake -DYAML_BUILD_SHARED_LIBS=%BUILD_SHARED% -DANDROID_ABI=armeabi-v7a -DANDROID_NATIVE_API_LEVEL=%ANDROID_API_LEVEL% -DCMAKE_INSTALL_PREFIX=install\%BUILD_TYPE% -G "Ninja" -DANDROID_STL=%STL_TYPE% -DBUILD_TESTING=OFF
    cmake --build .
    cmake --build . --target install
    ```

### GTest

#### Windows Build and Install

1. Create a directory to contain the Google test repo (example: `D:\development`), navigate into it and clone: `git clone https://github.com/google/googletest`. You should end up having a directory tree like `D:\development\googletest`.
2. Navigate into the googletest directory inside newly cloned googletest repo, your cwd should look like `D:\development\googletest\googletest`.
3. Run CMake to generate the configuration files: `cmake -DBUILD_SHARED_LIBS=ON -Dgtest_force_shared_crt=ON -DCMAKE_INSTALL_PREFIX=<path to gtest repo>\build\install -DGTEST_CREATE_SHARED_LIBRARY=1 ..`
4. Build with `"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\msbuild.exe" INSTALL.vcxproj /p:Configuration=Debug -maxcpucount`

#### macOS Build and Install

1. Clone https://github.com/google/googletest
1. `cd` into the directory where you cloned googletest
1. `mkdir build_mac`
1. `cd build_mac`
1. `cmake -G Xcode -DBUILD_SHARED_LIBS=ON -Dgtest_force_shared_crt=ON -DCMAKE_INSTALL_PREFIX=install/x86_64 -DGTEST_CREATE_SHARED_LIBRARY=1 ..`
1. `xcodebuild -parallelizeTargets -target ALL_BUILD`
1. `xcodebuild -parallelizeTargets -target install`

#### iOS Build and Install

1. Not needed.

#### Android Build and Install

1. Set these environment variables inside a Developer Command Prompt for VS 2019.

    ```bat
    set ANDROID_API_LEVEL=24
    set BUILD_TYPE=Debug
    set ARCH=arm
    set PLATFORM=android
    ```

  * For Unreal, set:
    ```bat
    set STL_TYPE=c++_static
    ```
  * For Unity, set:
    ```bat
    set STL_TYPE=c++_shared 
    ```

1. Clone https://github.com/google/googletest
2. `cd` into the directory where you cloned googletest
3. `mkdir build_android`
4. `cd build_android`

    ```bat
    cmake .. -DBUILD_SHARED_LIBS=ON -Dgtest_force_shared_crt=ON -DCMAKE_INSTALL_PREFIX=install\%BUILD_TYPE% -DGTEST_CREATE_SHARED_LIBRARY=1 -DCMAKE_TOOLCHAIN_FILE=%NDKROOT%\build\cmake\android.toolchain.cmake -DANDROID_ABI=armeabi-v7a -DANDROID_NATIVE_API_LEVEL=%ANDROID_API_LEVEL%  -G "Ninja" -DANDROID_STL=%STL_TYPE%
    cmake --build .
    cmake --build . --target install
    ```


### Boost

#### Windows Build and Install

1. Download and extract https://www.boost.org/users/history/version_1_76_0.html
2. `cd` into the directory you extracted Boost
3. Run `bootstrap.bat`
4. Run `.\b2 link=static`

#### macOS Build and Install

1. Download and extract https://www.boost.org/users/history/version_1_76_0.html
2. `cd` into the directory you extracted Boost
3. Run `./bootstrap.sh`
4. Run `./b2 link=static`

#### iOS Build and Install

1. Make the following updates to the script from https://gist.github.com/faithfracture/c629ae4c7168216a9856#file-boost-sh :
   1. Make sure it's in a separate folder from the boost folder for win64 and macos, for example: `~/development/ios-boost`.
   2. May need to give `r+w` permissions to all files in this folder with `chmod`.
   3. Update boost version in to `1.76.0` (or whichever one we are currently using for win64 and macos).
   4. Change BOOST_LIBS to `BOOST_LIBS="regex filesystem iostreams"`
2. Run the script using `./boost.sh -ios`.
3. Note: You can continue to use the other boost folder (ex: `<BOOST_PARENT_DIR>\boost_1_76_0`) when asked for the boost path in the `scripts/aws_gamekit_cpp_build.py` build script since the build needs boost source/includes. Use the `ios-boost` directory when prompted for the boost path in the `scripts/refresh_plugin.py` script that copies ios static libs.

#### Android Build and Install

1. Set these environment variables inside a Developer Command Prompt for VS 2019.

    ```bat
    set ANDROID_API_LEVEL=24
    set BUILD_TYPE=Debug
    set ARCH=arm
    set PLATFORM=android
    set CLANGPATH=%NDKROOT%\toolchains\llvm\prebuilt\windows-x86_64\bin
    set PATH=%PATH%;%CLANGPATH%
    ```

    * For Unreal, set:
    ```bat
    set STL_TYPE=c++_static
    ```
    * For Unity, set:
    ```bat
    set STL_TYPE=c++_shared 
    ```

2. Copy the `scripts\Android\sample-android-user-config.jam` to your User directory with the name `user-config.jam` e.g. `C:\users\<your username>\user-config.jam`. 
    
    **Note:** Remember to remove or rename this file after building Boost for Android, otherwise all future Boost builds will use this configuration.  

3. Download and extract https://www.boost.org/users/history/version_1_76_0.html. Make sure it's in a separate folder from the boost folder for win64 and macos, for example: `~/development/android-boost`.
4. `cd` into the directory you extracted Boost
5. Run `bootstrap.bat --with-libraries=regex,filesystem,iostreams`



6. Build boost:
 * For **Unreal**, run 
    ```bat
    .\b2 toolset=clang-armeabiv7a target-os=android architecture=arm --without-python threading=multi link=static --layout=tagged variant=debug,release
    ```
 * For **Unity**:  
    Open `boostcpp.jam` in your boost dir and add `android` to the platforms on line 210, so it ends up as
    ```jam
    ! [ $(property-set).get <target-os> ] in windows cygwin darwin aix android &&
    ```
    Then, run in the Command Prompt
    ```bat
    .\b2 toolset=clang-armeabiv7a target-os=android architecture=arm --without-python link=shared --layout=system variant=debug,release stage
    ```

### pybind11

#### Windows Build and Install

- Make sure `python3` is on your `PATH`

1. Clone https://github.com/pybind/pybind11/
2. `cd` into the directory where you cloned pybind11
3. `cmake -DBoost_INCLUDE_DIR=<BOOST_PARENT_DIR>\boost_1_76_0 -DCMAKE_INSTALL_PREFIX=<PYBIND_PARENT_DIR>\pybind11\install\<BUILD_TYPE> .`
4. `"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\msbuild.exe" INSTALL.vcxproj /p:Configuration=<BUILD_TYPE> -maxcpucount`

#### macOS Build and Install

1. Clone https://github.com/pybind/pybind11/
2. `cd` into the directory where you cloned pybind11
3. `cmake -DBOOST_ROOT=<BOOST_PARENT_DIR>/boost_1_76_0 .`
4. `make`
5. `sudo make install`

#### iOS Build and Install

Not supported.

#### Android Build and Install

Not supported.

## Build GameKit C++
Use the `scripts/aws_gamekit_cpp_build.py` utility to generate project files and compile for the specified platform.
- Usage: `python scripts/aws_gamekit_cpp_build.py <Platform> <Platform specific arguments> <BUILD_TYPE>`
- Example: `python scripts/aws_gamekit_cpp_build.py Windows --test Debug`
- Use `--help` to see all arguments and directions. If used after supplying a platform it will list and explain platform sepecific arguments.

 * For Android support in Unity, call the command as follows (replacing <BUILD_TYPE> with Release or Debug):
    ```bat
    python scripts/aws_gamekit_cpp_build.py  Android --shared <BUILD_TYPE>
    ```

The script will first search your environment variables for dependency path variables. If they are not present there, it will search in
the `.env` file at the root of this repository (created by running this script), else it will prompt you for input on where the dependencies
are located and save those locations in the `.env` file.


## Run Unit Tests

### Windows

1. `cd` into `tests\<BUILD_TYPE>` directory
2. Run `aws-gamekit-cpp-tests.exe`

### macOS

1. cd into the `tests/Debug` directory
2. Run `./aws-gamekit-cpp-tests`

## Update Plugin with new binaries and headers
Use the `scripts/refresh_plugin.py` script to update your game engine plugin with the new libraries and header files.
- Usage: `python scripts/refresh_plugin.py --platform <target_platform> <BuildType> <GameEngine> <game engine specific arguments>`
- Example: `python scripts/refresh_plugin.py --platform Windows Debug Unreal --unreal_plugin_path D:\development\MyUnrealGame\Plugins\AwsGameKit`
- Example: `python scripts/refresh_plugin.py --platform Windows Debug Unity --unity_plugin_path D:\development\MyUnityGame\Packages\com.amazonaws.gamekit`
- Use `--help` with the script to see argument options, and using `--help` after supplying a `GameEngine` argument, it will list and explain game engine specific arguments.
- **If refreshing MacOS or iOS binaries you will have to codesign them to distribute them to other developers, supply the `--certificate_name` argument and give a Developer ID Application certificate common name.**