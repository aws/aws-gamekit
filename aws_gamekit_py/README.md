# aws_gamekit_py
- Note: If having issues, use `python3` instead of `python` and use version `3.8.10`
## Test Setup

This test suite requires a client config file (`awsGameKitClientConfig.yml`) to be placed in this folder so that the `GameKitSessionManager` can load the proper values (region, Cognito client ID, etc) and hit the proper API endpoints. It is expected that this config file is generated from a GameKit deploy through the Unreal game engine. All features should be deployed for complete testing, otherwise the corresponding tests below will fail.

If you'd like to deploy and test a subset of features, see below.

### `test_deploy`

`test_deploy` requires a copy of `AwsGameTechGameKitCloudResources` to be placed next to `AwsGameTechGdkCppSdk` in order to load the proper CloudFormation templates.

## Test Running

After building in VisualStudio, run tests with:

```sh
$ pip install -r requirements.txt
$ python -m pytest
```

### Running a Subset of Tests

If you'd like to run a single test, you can specify it with the `-k` flag"

```sh
$ python -m pytest -k identity
```

You can find more information in pytest's documentation about [specifying tests](https://docs.pytest.org/en/6.2.x/usage.html#specifying-tests-selecting-tests).

_Note: You may have multiple python interpreters installed on your system. Make sure to use the same one that was used for building pybind, which can be found in pybind's cmakecache_

### Slow Tests

There are some slow tests (ex: `test_deploy`) that are skipped by default. If you want to run these as well, you need to pass a `--runslow` flag. This can be combined with other filters to still target specific tests.

```sh
$ python -m pytest -k test_deploy --runslow
```
