# AWS GameKit Testing
To run tests in Visual Studio:
1. Load the main solution in Visual Studio and build the **aws-gamekit-cpp-tests** project. This will build AWS GameKit projects as well as Gtest dependencies.
2. Right click on the **aws-gamekit-cpp-tests** project and click on 'Run tests'. This will run ALL the tests in the project.
3. To run a single test in VS open the Test Explorer window, right click on the test and click on Run.

To run all the tests suites from the command line:
1. Navigate into the **tests** directory
2. Run `ctest --verbose`

To run individual tests from the command line:
1. Navigate into the tests\Debug|Release directory
2. Run the executable that contains the test code and append a regular expression that matches the name of the test to run, for example `aws-gamekit-cpp-tests.exe --gtest_filter=*Null_TestCallback*`
