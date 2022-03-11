import Debug.aws_gamekit_py as gamekit

import os
import pytest
import time
from boto3 import Session

def custom_log(level, message):
    print("[{}] {}".format(level, message))


# Note: will not currently work with credentials that require a token
current_credentials = Session().get_credentials().get_frozen_credentials()

@pytest.fixture(scope="class")
def username():
    return "test_0"
@pytest.fixture(scope="class")
def password():
    return "Password123!"
@pytest.fixture(scope="class")
def region():
    return Session().region_name
@pytest.fixture(scope="class")
def access_key():
    return os.getenv("GAMEKIT_ACCESS_KEY", current_credentials.access_key)
@pytest.fixture(scope="class")
def secret_key():
    return os.getenv("GAMEKIT_SECRET_KEY", current_credentials.secret_key)
@pytest.fixture(scope="class")
def account_id(access_key, secret_key):
    return gamekit.core.get_aws_account_id(access_key, secret_key)

@pytest.fixture(scope="class")
def session_manager():
    # Verify we have a client config, tests hang otherwise
    filename = "awsGameKitClientConfig.yml"
    if not os.path.exists(filename):
        print("Missing `awsGameKitClientConfig.yml`, tests existing")
        raise SystemExit(1)

    session_manager = gamekit.authentication.session_manager_instance_create(filename)
    yield session_manager
    gamekit.authentication.session_manager_instance_release(session_manager)

@pytest.fixture(scope="class")
def identity_instance(session_manager, username, password):
    login = gamekit.identity.model.user_login(username, password)
    assert(login.user_name == username)
    assert(login.password == password)
    identity = gamekit.identity.identity_instance_create_with_session_manager(session_manager)
    assert(gamekit.identity.identity_login(identity, login) == gamekit.GAMEKIT_SUCCESS)
    yield identity
    gamekit.identity.identity_instance_release(identity)

@pytest.fixture
def credentials(region, access_key, secret_key, account_id):
    return gamekit.core.model.account_credentials(region, access_key, secret_key, account_id)

@pytest.fixture
def game_name():
    return os.getenv("GAMEKIT_GAME_NAME", "cpptest")

@pytest.fixture
def environment():
    return os.getenv("GAMEKIT_ENVIRONMENT", "dev")

@pytest.fixture
def account_info(credentials, game_name, environment):
    return gamekit.core.model.account_info(environment, credentials.account_id, "aws", game_name)

@pytest.fixture
def plugin_path():
    # Assuming AwsGameTechGameKitCloudResources is sibling directory to parent
    default_plugin_path = os.path.abspath(os.path.join(os.path.dirname(__file__), r'..\..\AwsGameTechGameKitCloudResources'))
    return os.getenv("GAMEKIT_PLUGIN_PATH", default_plugin_path)

@pytest.fixture
def root_path():
    default_root_path = os.path.abspath(os.path.join(os.path.dirname(__file__), r'..\tests\core\test_data\sampleplugin\instance'))
    return os.getenv("GAMEKIT_ROOT_PATH", default_root_path)

@pytest.fixture
def account_instance(account_info, credentials, root_path, plugin_path):
    account_instance = gamekit.core.account_instance_create_with_root_paths(account_info, credentials, root_path, plugin_path)
    yield account_instance
    gamekit.core.account_instance_release(account_instance)

@pytest.fixture
def settings_instance(root_path, game_name, environment):
    settings_instance = gamekit.core.settings_instance_create(root_path, game_name, environment)
    yield settings_instance
    gamekit.core.settings_instance_release(settings_instance)

@pytest.fixture
def game_saving_instance(session_manager):
    game_saving_instance = gamekit.game_saving.game_saving_instance_create(session_manager)
    yield game_saving_instance
    gamekit.game_saving.game_saving_instance_release(game_saving_instance)

@pytest.fixture
def main_resource_instance(account_info, credentials, root_path, plugin_path):
    main_resource = gamekit.core.resources_instance_create(account_info, credentials, gamekit.FeatureType.Main, root_path, plugin_path)
    yield main_resource
    gamekit.core.resources_instance_release(main_resource)

@pytest.fixture
def identity_resource_instance(account_info, credentials, root_path, plugin_path):
    identity_resource = gamekit.core.resources_instance_create(account_info, credentials, gamekit.FeatureType.Identity, root_path, plugin_path)
    yield identity_resource
    gamekit.core.resources_instance_release(identity_resource)

@pytest.fixture
def gamesaving_resource_instance(account_info, credentials, root_path, plugin_path):
    gamesaving_resource = gamekit.core.resources_instance_create(account_info, credentials, gamekit.FeatureType.GameStateCloudSaving, root_path, plugin_path)
    yield gamesaving_resource
    gamekit.core.resources_instance_release(gamesaving_resource)

@pytest.fixture
def userdata_resource_instance(account_info, credentials, root_path, plugin_path):
    userdata_resource = gamekit.core.resources_instance_create(account_info, credentials, gamekit.FeatureType.UserGameplayData, root_path, plugin_path)
    yield userdata_resource
    gamekit.core.resources_instance_release(userdata_resource)

@pytest.fixture
def achievements_resource_instance(account_info, credentials, root_path, plugin_path):
    achievements_resource = gamekit.core.resources_instance_create(account_info, credentials, gamekit.FeatureType.Achievements, root_path, plugin_path)
    yield achievements_resource
    gamekit.core.resources_instance_release(achievements_resource)

## END FIXTURES ##


class TestGamekit:

    @pytest.mark.offline
    def test_feature_enums(self):
        assert(gamekit.FeatureType.Main is not None)
        assert(gamekit.FeatureType.Identity is not None)
        assert(gamekit.FeatureType.Authentication is not None)
        assert(gamekit.FeatureType.Achievements is not None)
        assert(gamekit.FeatureType.GameStateCloudSaving is not None)
        assert(gamekit.FeatureType.UserGameplayData is not None)

    @pytest.mark.offline
    def test_consts(self):
        assert(gamekit.GAMEKIT_SUCCESS == 0)

    @pytest.mark.identity
    def test_login(self, identity_instance):
        assert(identity_instance is not None)
        status, user_response = gamekit.identity.identity_get_user(identity_instance)
        assert(status == gamekit.GAMEKIT_SUCCESS)
        assert(isinstance(user_response, gamekit.identity.model.get_user_response))

    @pytest.mark.achievements
    def test_achievements(self, identity_instance, session_manager, plugin_path):
        achievements_instance = gamekit.achievements.achievements_instance_create(session_manager, plugin_path)
        assert(achievements_instance is not None)
        status, achievements = gamekit.achievements.list_achievements(achievements_instance, 100, True)
        assert(status == gamekit.GAMEKIT_SUCCESS)
        assert(achievements is not None)

        gamekit.achievements.achievements_instance_release(achievements_instance)

    @pytest.mark.userdata
    def test_userdata(self, identity_instance, session_manager):
        user_gameplay_data = gamekit.user_gameplay_data.user_gameplay_data_instance_create(session_manager)
        items = [("bar", "baz"), ("bar2", "baz2")]
        assert(gamekit.GAMEKIT_SUCCESS == gamekit.user_gameplay_data.add_user_gameplay_data(user_gameplay_data, "foo", items))

        status, bundles = gamekit.user_gameplay_data.list_user_gameplay_data_bundles(user_gameplay_data)
        assert(gamekit.GAMEKIT_SUCCESS == status)
        assert(len(bundles) == 1)
        assert("foo" in bundles)

        status, bundle = gamekit.user_gameplay_data.get_user_gameplay_data_bundle(user_gameplay_data, "foo")
        assert(gamekit.GAMEKIT_SUCCESS == status)
        assert(bundle.bundle_name == "foo")
        assert(len(bundle.items) == 2)

        assert(gamekit.GAMEKIT_SUCCESS == gamekit.user_gameplay_data.delete_user_gameplay_data_bundle(user_gameplay_data, "foo"))

        status, bundles = gamekit.user_gameplay_data.list_user_gameplay_data_bundles(user_gameplay_data)
        assert(gamekit.GAMEKIT_SUCCESS == status)
        assert(len(bundles) == 0)

        gamekit.user_gameplay_data.user_gameplay_data_instance_release(user_gameplay_data)

    @pytest.mark.gamesaving
    def test_gamesaving(self, identity_instance, game_saving_instance):
        slot_name = "test"
        content = open(__file__, 'rb').read() # reads the current file and turns it into a data object

        # Ensure we start with no slots
        status, slots = gamekit.game_saving.get_all_slot_sync_statuses(game_saving_instance)
        assert(gamekit.GAMEKIT_SUCCESS == status)
        assert(len(slots) == 0)

        # Create our slot object
        slot = gamekit.game_saving.model.slot(slot_name, 0, True, list(content), os.path.abspath(__file__))
        assert(slot is not None)
        assert(slot.slot_name == slot_name)

        # save slot
        result, slot_copy = gamekit.game_saving.save_slot(game_saving_instance, slot)
        assert(gamekit.GAMEKIT_SUCCESS == result)
        assert(slot_copy.slot_name == slot_name)

        # read all slots, ensure saved slot(s) is returned
        status, slots = gamekit.game_saving.get_all_slot_sync_statuses(game_saving_instance)
        assert(gamekit.GAMEKIT_SUCCESS == status)
        assert(len(slots) == 1)

        # delete slot
        result = gamekit.game_saving.delete_slot(game_saving_instance, slot_name)
        assert(gamekit.GAMEKIT_SUCCESS == result)

        # read all slots, expect zero
        status, slots = gamekit.game_saving.get_all_slot_sync_statuses(game_saving_instance)
        assert(gamekit.GAMEKIT_SUCCESS == status)
        assert(len(slots) == 0)

    @pytest.mark.slow
    def test_deploy(self, account_instance, settings_instance, main_resource_instance, identity_resource_instance, gamesaving_resource_instance, userdata_resource_instance, achievements_resource_instance):
        result = gamekit.core.account_instance_bootstrap(account_instance)
        assert(gamekit.GAMEKIT_SUCCESS == result)

        result = gamekit.core.account_save_feature_instance_templates(account_instance)
        assert(gamekit.GAMEKIT_SUCCESS == result)

        result = gamekit.core.account_upload_all_dashboards(account_instance)
        assert(gamekit.GAMEKIT_SUCCESS == result)

        # Set variables for all features
        for feature_type in [gamekit.FeatureType.Identity, gamekit.FeatureType.GameStateCloudSaving, gamekit.FeatureType.UserGameplayData, gamekit.FeatureType.Achievements]:
            gamekit.core.settings_set_feature_variables(settings_instance, feature_type)

        assert(gamekit.GAMEKIT_SUCCESS == gamekit.core.save_settings(settings_instance))

        for feature_resource in [main_resource_instance, identity_resource_instance, gamesaving_resource_instance, userdata_resource_instance, achievements_resource_instance]:
            result = gamekit.core.resources_save_cloud_formation_instance(feature_resource)
            assert(gamekit.GAMEKIT_SUCCESS == result)

            result = gamekit.core.resources_save_layer_instances(feature_resource)
            assert(gamekit.GAMEKIT_SUCCESS == result)

            result = gamekit.core.resources_save_function_instances(feature_resource)
            assert(gamekit.GAMEKIT_SUCCESS == result)

            result = gamekit.core.resources_upload_feature_layers(feature_resource)
            assert(gamekit.GAMEKIT_SUCCESS == result)

            result = gamekit.core.resources_upload_feature_functions(feature_resource)
            assert(gamekit.GAMEKIT_SUCCESS == result)

            result = gamekit.core.resources_instance_create_or_update_stack(feature_resource)
            assert(gamekit.GAMEKIT_SUCCESS == result)

    @pytest.mark.slow
    def test_delete(self, account_instance, main_resource_instance, identity_resource_instance, gamesaving_resource_instance, userdata_resource_instance, achievements_resource_instance):
        # make sure we delete identity & main stack last
        for feature_resource in [gamesaving_resource_instance, userdata_resource_instance, achievements_resource_instance, identity_resource_instance, main_resource_instance]:
            result = gamekit.core.resources_instance_delete_stack(feature_resource)
            assert(gamekit.GAMEKIT_SUCCESS == result)
