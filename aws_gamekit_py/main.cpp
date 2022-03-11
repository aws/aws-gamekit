#include <iostream>

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <aws/gamekit/core/exports.h>
#include <aws/gamekit/core/gamekit_account.h>
#include <aws/gamekit/core/model/account_info.h>
#include <aws/gamekit/authentication/exports.h>
#include <aws/gamekit/authentication/gamekit_session_manager.h>
#include <aws/gamekit/identity/exports.h>
#include <aws/gamekit/identity/gamekit_identity.h>
#include <aws/gamekit/achievements/exports.h>
#include <aws/gamekit/user-gameplay-data/exports.h>
#include <aws/gamekit/user-gameplay-data/gamekit_user_gameplay_data.h>
#include <aws/gamekit/game-saving/exports.h>
#include <aws/gamekit/game-saving/gamekit_game_saving.h>

/*
 * You're going to see a lot of `malloc`s below, without corresponding `free`s.
 * This is because python gets handed off the reference and takes ownership,
 * and will free the pointer when it is done (at the end of a test).
 *
 * https://pybind11.readthedocs.io/en/stable/advanced/functions.html#return-value-policies
 */

namespace py = pybind11;

using namespace GameKit;

// Stolen from AwsGameKitDispatcher.h
template <typename Lambda, typename RetType, typename ... Args>
struct LambdaDispatcher
{
    static RetType Dispatch(void* func, Args... args)
    {
        return (*static_cast<Lambda*>(func)) (std::forward<Args>(args)...);
    }
    
    static RetType Dispatch(void* func, Args&&... args)
    {
        return (*static_cast<Lambda*>(func)) (std::forward<Args>(args)...);
    }
};

// This namespace contains pybind-friendly structs and function wrappers
namespace gkpy_shim
{
    namespace logging
    {
        static std::function<void(unsigned int, const char*)> pyLog = nullptr;

        // Set a global log callback here
        void set_py_log(std::function<void(unsigned int, const char*)>& logFunc)
        {
            pyLog = logFunc;
        };

        void log(unsigned int l, const char* m, int s)
        {
            if (pyLog != nullptr)
            {
                pyLog(l, m);
            }
        }
    }

    enum handle_type
    {
        // Core
        account_instance,
        feature_resources_instance,
        settings_instance,

        // Authentication
        session_manager_instance,

        // Identity
        identity_instance,

        // Achievements
        achievement_instance,

        // Game Saving
        game_saving_instance,

        // User Game Data
        user_game_data_instance
    };
    
    template <enum handle_type H>
    struct gamekit_handle
    {
        void* low_level_handle;

        gamekit_handle(void* h)
        {
            low_level_handle = h;
        }
    };

    typedef gamekit_handle<account_instance> gamekit_account_instance_handle;
    typedef gamekit_handle<feature_resources_instance> gamekit_feature_resources_instance_handle;
    typedef gamekit_handle<settings_instance> gamekit_settings_instance_handle;
    typedef gamekit_handle<session_manager_instance> gamekit_session_manager_instance_handle;
    typedef gamekit_handle<identity_instance> gamekit_identity_instance_handle;
    typedef gamekit_handle<achievement_instance> gamekit_achievement_instance_handle;
    typedef gamekit_handle<game_saving_instance> gamekit_game_saving_instance_handle;
    typedef gamekit_handle<user_game_data_instance> gamekit_user_game_data_instance_handle;

    // All structs and methods required by core's exports.h
    namespace core_exports
    {
        namespace models
        {
            struct account_info
            {
                std::string environment;
                std::string account_id;
                std::string company_name;
                std::string game_name;

                account_info()
                {}

                account_info(const std::string& environment, const std::string& accountId, const std::string& companyName, const std::string& gameName)
                    : environment(environment), account_id(accountId), company_name(companyName), game_name(gameName)
                {}

                // char*s will be valid as long as the returned struct is in scope
                GameKit::AccountInfo ToAccountInfo() const
                {
                    GameKit::AccountInfo accountInfo;
                    accountInfo.environment = environment.c_str();
                    accountInfo.accountId = account_id.c_str();
                    accountInfo.companyName = company_name.c_str();
                    accountInfo.gameName = game_name.c_str();

                    return accountInfo;
                }
            };

            struct account_credentials
            {
                std::string region;
                std::string access_key;
                std::string access_secret;
                std::string account_id;

                account_credentials()
                {}

                account_credentials(const std::string& region, const std::string& accessKey, const std::string& accessSecret, const std::string& accountId)
                    : region(region), access_key(accessKey), access_secret(accessSecret), account_id(accountId)
                {}

                // char*s will be valid as long as the returned struct is in scope
                GameKit::AccountCredentials ToAccountCredentials() const
                {
                    GameKit::AccountCredentials credentials;
                    credentials.region = region.c_str();
                    credentials.accessKey = access_key.c_str();
                    credentials.accessSecret = access_secret.c_str();
                    credentials.accountId = account_id.c_str();

                    return credentials;
                }
            };
        }

        unsigned int initialize_aws_sdk()
        {
            return GameKitInitializeAwsSdk(gkpy_shim::logging::log);
        }

        gamekit_account_instance_handle account_instance_create(models::account_info accountInfo, models::account_credentials accountCredentials)
        {
            void* low_level_handle = GameKitAccountInstanceCreate(accountInfo.ToAccountInfo(), accountCredentials.ToAccountCredentials(), gkpy_shim::logging::log);
            return gamekit_account_instance_handle(low_level_handle);
        }
        
        gamekit_account_instance_handle account_instance_create_with_root_paths(models::account_info accountInfo, models::account_credentials accountCredentials, char* rootPath, char* pluginRoot)
        {
            void* low_level_handle = GameKitAccountInstanceCreateWithRootPaths(accountInfo.ToAccountInfo(), accountCredentials.ToAccountCredentials(), rootPath, pluginRoot, gkpy_shim::logging::log);
            return gamekit_account_instance_handle(low_level_handle);
        }

        void account_instance_release(gamekit_account_instance_handle handle)
        {
            GameKitAccountInstanceRelease(handle.low_level_handle);
        }

        gamekit_settings_instance_handle settings_instance_create(char* rootPath, char* gameName, char* environment)
        {
            void* low_level_handle = GameKitSettingsInstanceCreate(rootPath, "1.0.0", gameName, environment, gkpy_shim::logging::log);
            return gamekit_settings_instance_handle(low_level_handle);
        }

        void settings_instance_release(gamekit_settings_instance_handle handle)
        {
            GameKitSettingsInstanceRelease(handle.low_level_handle);
        }

        bool account_has_valid_credentials(gamekit_account_instance_handle handle)
        {
            return GameKitAccountHasValidCredentials(handle.low_level_handle);
        }

        unsigned int account_instance_bootstrap(gamekit_account_instance_handle handle)
        {
            return GameKitAccountInstanceBootstrap(handle.low_level_handle);
        }

        void settings_set_feature_variables(gamekit_settings_instance_handle handle, FeatureType featureType)
        {
            const char* varKeys[4] = { "max_save_slots_per_player", "facebook_client_id", "is_facebook_enabled", "cloudwatch_dashboard_enabled" };
            const char* varValues[4] = { "10", "", "false", "false" };
            return GameKitSettingsSetFeatureVariables(handle.low_level_handle, featureType, varKeys, varValues, 4);
        }

        unsigned int account_upload_all_dashboards(gamekit_account_instance_handle handle)
        {
            return GameKitAccountUploadAllDashboards(handle.low_level_handle);
        }

        unsigned int save_settings(gamekit_settings_instance_handle handle)
        {
            return GameKitSettingsSave(handle.low_level_handle);
        }

        unsigned int account_save_feature_instance_templates(gamekit_account_instance_handle handle)
        {
            return GameKitAccountSaveFeatureInstanceTemplates(handle.low_level_handle);
        }

        unsigned int account_upload_layers(gamekit_account_instance_handle handle)
        {
            return GameKitAccountUploadLayers(handle.low_level_handle);
        }

        unsigned int account_upload_functions(gamekit_account_instance_handle handle)
        {
            return GameKitAccountUploadFunctions(handle.low_level_handle);
        }

        unsigned int account_create_or_update_main_stack(gamekit_account_instance_handle handle)
        {
            return GameKitAccountCreateOrUpdateMainStack(handle.low_level_handle);
        }

        unsigned int account_create_or_update_stacks(gamekit_account_instance_handle handle)
        {
            return GameKitAccountCreateOrUpdateStacks(handle.low_level_handle);
        }

        unsigned int account_deploy_api_gateway_stage(gamekit_account_instance_handle handle)
        {
            return GameKitAccountDeployApiGatewayStage(handle.low_level_handle);
        }

        py::str get_aws_account_id(const std::string& accessKey, const std::string &secretKey)
        {
            py::str accountId;
            auto getAccountIdDispatcher = [&](const char* response)
            {
                accountId = py::str(response);
            };
            typedef LambdaDispatcher<decltype(getAccountIdDispatcher), void, const char*> GetAccountIdDispatcher;

            GameKitGetAwsAccountId(&getAccountIdDispatcher, GetAccountIdDispatcher::Dispatch, accessKey.c_str(), secretKey.c_str(), gkpy_shim::logging::log);
            return accountId;
        }

        gamekit_feature_resources_instance_handle resources_instance_create(models::account_info accountInfo, models::account_credentials credentials, FeatureType featureType, const std::string& root_path, const std::string& plugin_path)
        {
            return gamekit_feature_resources_instance_handle(GameKitResourcesInstanceCreateWithRootPaths(accountInfo.ToAccountInfo(), credentials.ToAccountCredentials(), featureType, root_path.c_str(), plugin_path.c_str(), gkpy_shim::logging::log));
        }

        void resources_instance_release(gamekit_feature_resources_instance_handle resources_instance_handle)
        {
            GameKitResourcesInstanceRelease(resources_instance_handle.low_level_handle);
        }

        unsigned int resources_instance_create_or_update_stack(gamekit_feature_resources_instance_handle resources_instance_handle)
        {
            return GameKitResourcesInstanceCreateOrUpdateStack(resources_instance_handle.low_level_handle);
        }

        unsigned int resources_save_cloud_formation_instance(gamekit_feature_resources_instance_handle resources_instance_handle)
        {
            return GameKitResourcesSaveCloudFormationInstance(resources_instance_handle.low_level_handle);
        }

        unsigned int resources_save_layer_instances(gamekit_feature_resources_instance_handle resources_instance_handle)
        {
            return GameKitResourcesSaveLayerInstances(resources_instance_handle.low_level_handle);
        }

        unsigned int resources_save_function_instances(gamekit_feature_resources_instance_handle resources_instance_handle)
        {
            return GameKitResourcesSaveFunctionInstances(resources_instance_handle.low_level_handle);
        }

        unsigned int resources_upload_feature_layers(gamekit_feature_resources_instance_handle resources_instance_handle)
        {
            return GameKitResourcesUploadFeatureLayers(resources_instance_handle.low_level_handle);
        }

        unsigned int resources_upload_feature_functions(gamekit_feature_resources_instance_handle resources_instance_handle)
        {
            return GameKitResourcesUploadFeatureFunctions(resources_instance_handle.low_level_handle);
        }

        unsigned int resources_instance_delete_stack(gamekit_feature_resources_instance_handle resources_instance_handle)
        {
            return GameKitResourcesInstanceDeleteStack(resources_instance_handle.low_level_handle);
        }
    };

    namespace session_exports
    {
        gamekit_session_manager_instance_handle session_manager_instance_create(const std::string &clientConfigFile)
        {
            void* low_level_handle = GameKitSessionManagerInstanceCreate(clientConfigFile.c_str(), gkpy_shim::logging::log);
            return gamekit_session_manager_instance_handle(low_level_handle);
        }

        void session_manager_instance_release(gamekit_session_manager_instance_handle session_handle)
        {
            GameKitSessionManagerInstanceRelease(session_handle.low_level_handle);
        }
    }

    namespace identity_exports
    {
        namespace models
        {
            struct user_login
            {
                std::string user_name;
                std::string password;

                user_login()
                {}

                user_login(const std::string& userName, const std::string& password)
                    :user_name(userName), password(password)
                {}

                // char*s will be valid as long as the returned struct is in scope
                GameKit::UserLogin ToUserLogin() const
                {
                    return UserLogin{ user_name.c_str(), password.c_str() };
                }
            };

            struct get_user_response
            {
                std::string user_id;
                std::string updated_at;
                std::string created_at;
                std::string facebook_external_id;
                std::string facebook_ref_id;

                get_user_response()
                {}

                get_user_response(const std::string& userId, const std::string& updatedAt, const std::string& createdAt, const std::string& facebookExternalId, const std::string& facebookRefId)
                    :user_id(userId), updated_at(updatedAt), created_at(createdAt), facebook_external_id(facebookExternalId), facebook_ref_id(facebookRefId)
                {}
            };
        }
        gamekit_identity_instance_handle identity_instance_create_with_session_manager(gamekit_session_manager_instance_handle session_handle)
        {
            void *low_level_handle = GameKitIdentityInstanceCreateWithSessionManager(session_handle.low_level_handle, gkpy_shim::logging::log);
            return gamekit_identity_instance_handle(low_level_handle);
        }

        void identity_instance_release(gamekit_identity_instance_handle identity_handle)
        {
            GameKitIdentityInstanceRelease(identity_handle.low_level_handle);
        }

        unsigned int identity_login(gamekit_identity_instance_handle identity_handle, models::user_login user_login)
        {
            return GameKitIdentityLogin(identity_handle.low_level_handle, user_login.ToUserLogin());
        }

        // Returns a tuple of Status, GetUserResponse
        py::tuple identity_get_user(gamekit_identity_instance_handle identity_handle)
        {
            models::get_user_response userResponse;
            auto getUserInfoDispatcher = [&](const GetUserResponse* response)
            {
                userResponse.user_id = py::str(response->userId);
                userResponse.updated_at = py::str(response->updatedAt);
                userResponse.created_at = py::str(response->createdAt);
                userResponse.facebook_external_id = py::str(response->facebookExternalId);
                userResponse.facebook_ref_id = py::str(response->facebookRefId);
            };
            typedef LambdaDispatcher<decltype(getUserInfoDispatcher), void, const GetUserResponse*> GetUserInfoDispatcher;
            unsigned int status = GameKitIdentityGetUser(identity_handle.low_level_handle, &getUserInfoDispatcher, GetUserInfoDispatcher::Dispatch);
            return py::make_tuple(status, userResponse);
        }
    }

    namespace achievements_exports
    {
        namespace models
        {
            struct achievement
            {
                std::string achievement_id;
                std::string title;

                achievement()
                {}

                achievement(const std::string& achievementId, const std::string& title)
                    :achievement_id(achievementId), title(title)
                {}
            };
        }
        gamekit_achievement_instance_handle achievements_instance_create(gamekit_session_manager_instance_handle session_handle, const std::string& plugin_root_path)
        {
            void* instance_handle = GameKitAchievementsInstanceCreateWithSessionManager(session_handle.low_level_handle, gkpy_shim::logging::log);
            return gamekit_achievement_instance_handle(instance_handle);
        }

        void achievements_instance_release(gamekit_achievement_instance_handle achievements_handle)
        {
            GameKitAchievementsInstanceRelease(achievements_handle.low_level_handle);
        }

        py::tuple list_achievements(gamekit_achievement_instance_handle achievements_handle, unsigned int page_size, bool wait_for_all_pages)
        {
            py::list results;
            auto listAchievementsDispatcher = [&](const char* response)
            {
                const std::string jsonData = std::string(response);
                const JsonValue value = Aws::Utils::Json::JsonValue(jsonData);
                const JsonView view = value.View();
                Aws::Utils::Array<JsonView> achievements = view.GetObject("data").GetArray("achievements");

                for (int i = 0; i < achievements.GetLength(); i++)
                {
                    JsonView achievement = achievements.GetItem(i);
                    if (achievement.IsObject())
                    {
                        models::achievement a{ py::str(achievement.GetString("achievementId")), py::str(achievement.GetString("title")) };
                        results.append(a);
                    }
                }
            };
            typedef LambdaDispatcher<decltype(listAchievementsDispatcher), void, const char*> ListAchievementsDispatcher;

            unsigned int status = GameKitListAchievements(achievements_handle.low_level_handle, page_size, wait_for_all_pages, &listAchievementsDispatcher, ListAchievementsDispatcher::Dispatch);
            return py::make_tuple(status, results);
        }

        py::tuple get_achievement(gamekit_achievement_instance_handle achievements_handle, const std::string& achievement_id)
        {
            models::achievement achievement{};
            auto getAchievementsDispatcher = [&](const char* response)
            {
                const std::string jsonData = std::string(response);
                const JsonValue value = Aws::Utils::Json::JsonValue(jsonData);
                const JsonView view = value.View();
                const JsonView data = view.GetObject("data");
                achievement.achievement_id = data.GetString("achievementId");
                achievement.title = data.GetString("title");
            };
            typedef LambdaDispatcher<decltype(getAchievementsDispatcher), void, const char*> GetAchievementsDispatcher;
            unsigned int status = GameKitGetAchievement(achievements_handle.low_level_handle, achievement_id.c_str(), &getAchievementsDispatcher, GetAchievementsDispatcher::Dispatch);
            return py::make_tuple(status, achievement);
        }
    }

    namespace user_gameplay_data_exports
    {
        namespace models
        {
            struct user_gameplay_data_bundle
            {
                std::string bundle_name;
                std::vector<std::pair<std::string, std::string>> _items;
                std::vector<const char*> keys;
                std::vector<const char*> values;

                user_gameplay_data_bundle()
                {}

                user_gameplay_data_bundle(std::string bundleName, std::vector<std::pair<std::string, std::string>> items)
                    :bundle_name(bundleName), _items(items)
                {
                    for (std::pair<std::string, std::string> &tuple : _items)
                    {
                        keys.push_back(tuple.first.c_str());
                        values.push_back(tuple.second.c_str());
                    }
                }

                // char*s will be valid as long as the returned struct is in scope
                GameKit::UserGameplayDataBundle ToUserGameplayDataBundle()
                {
                    GameKit::UserGameplayDataBundle bundle{};
                    bundle.bundleName = bundle_name.c_str();
                    bundle.numKeys = _items.size();
                    bundle.bundleItemKeys = keys.data();
                    bundle.bundleItemValues = values.data();
                    return bundle;
                }
            };
        }

        gamekit_user_game_data_instance_handle user_gameplay_data_instance_create(gamekit_session_manager_instance_handle session_handle)
        {
            void* instance_handle = GameKitUserGameplayDataInstanceCreateWithSessionManager(session_handle.low_level_handle, gkpy_shim::logging::log);
            return gamekit_user_game_data_instance_handle(instance_handle);
        }

        void user_gameplay_data_instance_release(gamekit_user_game_data_instance_handle user_gameplay_data_handle)
        {
            GameKitUserGameplayDataInstanceRelease(user_gameplay_data_handle.low_level_handle);
        }

        unsigned int add_user_gameplay_data(gamekit_user_game_data_instance_handle user_gameplay_data_handle, std::string bundle_name, std::vector<std::pair<std::string, std::string>> items)
        {
            models::user_gameplay_data_bundle bundle = models::user_gameplay_data_bundle{ bundle_name, items };
            const GameKit::UserGameplayDataBundle userGameplayDataBundle = bundle.ToUserGameplayDataBundle();
            return GameKitAddUserGameplayData(user_gameplay_data_handle.low_level_handle, userGameplayDataBundle, nullptr, nullptr);
        }

        // Returns a tuple of (status, list of strings)
        //  as the GameKit function doesn't return fully hydrated bundles
        py::tuple list_user_gameplay_data_bundles(gamekit_user_game_data_instance_handle user_gameplay_data_handle)
        {
            py::list bundles;
            auto bundlesDispatcher = [&](const char* charPtr)
            {
                bundles.append(py::str(charPtr));
            };

            typedef LambdaDispatcher<decltype(bundlesDispatcher), void, const char*> BundlesDispatcher;
            unsigned int status = GameKitListUserGameplayDataBundles(user_gameplay_data_handle.low_level_handle, &bundlesDispatcher, BundlesDispatcher::Dispatch);
            return py::make_tuple(status, bundles);
        }

        // Returns a tuple of (status, models::user_gameplay_data_bundle)
        py::tuple get_user_gameplay_data_bundle(gamekit_user_game_data_instance_handle user_gameplay_data_handle, char* bundle_name)
        {
            std::vector<std::pair<std::string, std::string>> pairs;
            auto bundleSetter = [&pairs](const char* key, const char* value)
            {
                pairs.push_back(std::make_pair(key, value));
            };
            typedef LambdaDispatcher<decltype(bundleSetter), void, const char*, const char*> BundleSetter;

            // act
            unsigned int status = GameKitGetUserGameplayDataBundle(user_gameplay_data_handle.low_level_handle, bundle_name, &bundleSetter, BundleSetter::Dispatch);
            return py::make_tuple(status, models::user_gameplay_data_bundle{ std::string(bundle_name), pairs });
        }

        unsigned int delete_user_gameplay_data_bundle(gamekit_user_game_data_instance_handle user_gameplay_data_handle, char* bundle_name)
        {
            return GameKitDeleteUserGameplayDataBundle(user_gameplay_data_handle.low_level_handle, bundle_name);
        }

        unsigned int delete_all_user_gameplay_data(gamekit_user_game_data_instance_handle user_gameplay_data_handle)
        {
            return GameKitDeleteAllUserGameplayData(user_gameplay_data_handle.low_level_handle);
        }
    }

    namespace game_saving_exports
    {
        namespace models
        {
            struct slot
            {
                std::string slot_name;
                int64_t epoch_time;
                bool override_sync;
                std::vector<uint8_t> data_vector;
                std::string local_slot_information_file_path;

                slot()
                {}

                slot(std::string slotName, int64_t epochTime, bool overrideSync, std::vector<uint8_t> data, std::string localSlotInformationFilePath)
                    :slot_name(slotName), epoch_time(epochTime), override_sync(overrideSync), data_vector(data), local_slot_information_file_path(localSlotInformationFilePath)
                {}

                GameSavingModel ToGameSavingModel()
                {
                    GameSavingModel savingModel{};
                    savingModel.slotName = slot_name.c_str();
                    savingModel.epochTime = epoch_time;
                    savingModel.overrideSync = override_sync;
                    savingModel.data = data_vector.data();
                    savingModel.dataSize = data_vector.size();
                    savingModel.localSlotInformationFilePath = local_slot_information_file_path.c_str();
                    return savingModel;
                }
            };
        }

        gamekit_game_saving_instance_handle game_saving_instance_create(gamekit_session_manager_instance_handle session_handle)
        {
            // fake our reads and writes for faster testing without mucking up our filesystem
            auto fakeWrite = [](DISPATCH_RECEIVER_HANDLE, const char* filePath, const uint8_t* data, const unsigned int size)
            {
                return true;
            };
            auto fakeRead = [](DISPATCH_RECEIVER_HANDLE, const char* filePath, uint8_t* data, unsigned int size)
            {
                return true;
            };
            FileActions fileActions = FileActions{};
            fileActions.fileWriteCallback = fakeWrite;
            fileActions.fileReadCallback = fakeRead;
            void* instance_handle = GameKitGameSavingInstanceCreateWithSessionManager(session_handle.low_level_handle, gkpy_shim::logging::log, nullptr, 0, fileActions);
            return gamekit_game_saving_instance_handle(instance_handle);
        }

        void game_saving_instance_release(gamekit_game_saving_instance_handle game_saving_handle)
        {
            GameKitGameSavingInstanceRelease(game_saving_handle.low_level_handle);
        }

        py::tuple get_all_slot_sync_statuses(gamekit_game_saving_instance_handle game_saving_handle)
        {
            py::list slots;

            auto slotCallback = [&slots](const Slot* syncedSlots, unsigned int slotCount, bool complete, unsigned int callStatus)
            {
                for (unsigned int i = 0; i < slotCount; ++i)
                {
                    models::slot slot{};
                    // We can hydrate this more later if we need further testing
                    slot.slot_name = std::string(syncedSlots[i].slotName);
                    slots.append(slot);
                }
            };
            typedef LambdaDispatcher<decltype(slotCallback), void, const Slot*, unsigned int, bool, unsigned int> SlotsDispatcher;

            unsigned int status = GameKitGetAllSlotSyncStatuses(game_saving_handle.low_level_handle, &slotCallback, SlotsDispatcher::Dispatch, true, 100);
            return py::make_tuple(status, slots);
        }

        py::tuple save_slot(gamekit_game_saving_instance_handle game_saving_handle, models::slot slot)
        {
            models::slot returnSlot{};

            auto slotCallback = [&returnSlot](const Slot * syncedSlots, unsigned int slotCount, Slot slot, unsigned int callStatus)
            {
                returnSlot.slot_name = std::string(slot.slotName);
            };
            typedef LambdaDispatcher<decltype(slotCallback), void, const Slot*, unsigned int, Slot, unsigned int> SlotsDispatcher;

            unsigned int result = GameKitSaveSlot(game_saving_handle.low_level_handle, &slotCallback, SlotsDispatcher::Dispatch, slot.ToGameSavingModel());
            return py::make_tuple(result, returnSlot);
        }

        // Sending nullptr for callbacks as we don't care for a copy of the slot when we're done
        unsigned int delete_slot(gamekit_game_saving_instance_handle game_saving_handle, std::string slot_name)
        {
            return GameKitDeleteSlot(game_saving_handle.low_level_handle, nullptr, nullptr, slot_name.c_str());
        }
    }
};

PYBIND11_MODULE(aws_gamekit_py, m)
{
    // Core module
    py::module core = m.def_submodule("core", "Core submodule");

    // intermediate python-friendly structs
    //py::class_<gkpy_shim::gamekit_handle>(core, "gamekit_handle"); // don't expose the void* to make this truly opaque on python side
    py::class_<gkpy_shim::gamekit_account_instance_handle>(core, "gamekit_account_instance_handle");
    py::class_<gkpy_shim::gamekit_feature_resources_instance_handle>(core, "gamekit_feature_resources_instance_handle");
    py::class_<gkpy_shim::gamekit_settings_instance_handle>(core, "gamekit_settings_instance_handle");
    py::class_<gkpy_shim::gamekit_session_manager_instance_handle>(core, "gamekit_session_manager_instance_handle");
    py::class_<gkpy_shim::gamekit_identity_instance_handle>(core, "gamekit_identity_instance_handle");
    py::class_<gkpy_shim::gamekit_achievement_instance_handle>(core, "gamekit_achievement_instance_handle");
    py::class_<gkpy_shim::gamekit_game_saving_instance_handle>(core, "gamekit_game_saving_instance_handle");
    py::class_<gkpy_shim::gamekit_user_game_data_instance_handle>(core, "gamekit_user_game_data_instance_handle");

    // Structs used in Core exports
    py::module core_models = core.def_submodule("model", "Core Models submodule");
    py::class_<gkpy_shim::core_exports::models::account_info>(core_models, "account_info")
        .def(py::init())
        .def(py::init<const std::string&, const std::string&, const std::string&, const std::string&>())
        .def_readwrite("environment", &gkpy_shim::core_exports::models::account_info::environment)
        .def_readwrite("account_id", &gkpy_shim::core_exports::models::account_info::account_id)
        .def_readwrite("company_name", &gkpy_shim::core_exports::models::account_info::company_name)
        .def_readwrite("game_name", &gkpy_shim::core_exports::models::account_info::game_name);

    py::class_<gkpy_shim::core_exports::models::account_credentials>(core_models, "account_credentials")
        .def(py::init())
        .def(py::init<const std::string&, const std::string&, const std::string&, const std::string&>())
        .def_readwrite("region", &gkpy_shim::core_exports::models::account_credentials::region)
        .def_readwrite("access_key", &gkpy_shim::core_exports::models::account_credentials::access_key)
        .def_readwrite("access_secret", &gkpy_shim::core_exports::models::account_credentials::access_secret)
        .def_readwrite("account_id", &gkpy_shim::core_exports::models::account_credentials::account_id);

    // Global logging
    py::module logging = m.def_submodule("logging", "Logging module");
    logging.def("set_py_log", &gkpy_shim::logging::set_py_log);

    // Core exports functions
    core.def("initialize_aws_sdk", &gkpy_shim::core_exports::initialize_aws_sdk);

    core.def("account_instance_create", &gkpy_shim::core_exports::account_instance_create);

    core.def("account_instance_create_with_root_paths", &gkpy_shim::core_exports::account_instance_create_with_root_paths);

    core.def("account_instance_release", &gkpy_shim::core_exports::account_instance_release);

    core.def("settings_instance_create", &gkpy_shim::core_exports::settings_instance_create);

    core.def("settings_instance_release", &gkpy_shim::core_exports::settings_instance_release);

    core.def("account_has_valid_credentials", &gkpy_shim::core_exports::account_has_valid_credentials);

    core.def("account_instance_bootstrap", &gkpy_shim::core_exports::account_instance_bootstrap);

    core.def("settings_set_feature_variables", &gkpy_shim::core_exports::settings_set_feature_variables);

    core.def("save_settings", &gkpy_shim::core_exports::save_settings);

    core.def("account_save_feature_instance_templates", &gkpy_shim::core_exports::account_save_feature_instance_templates);

    core.def("account_upload_all_dashboards", &gkpy_shim::core_exports::account_upload_all_dashboards);

    core.def("account_upload_layers", &gkpy_shim::core_exports::account_upload_layers);

    core.def("account_upload_functions", &gkpy_shim::core_exports::account_upload_functions);

    core.def("account_create_or_update_main_stack", &gkpy_shim::core_exports::account_create_or_update_main_stack);

    core.def("account_create_or_update_stacks", &gkpy_shim::core_exports::account_create_or_update_stacks);

    core.def("account_deploy_api_gateway_stage", &gkpy_shim::core_exports::account_deploy_api_gateway_stage);

    core.def("get_aws_account_id", &gkpy_shim::core_exports::get_aws_account_id);

    core.def("resources_instance_create", &gkpy_shim::core_exports::resources_instance_create);

    core.def("resources_instance_create_or_update_stack", &gkpy_shim::core_exports::resources_instance_create_or_update_stack);

    core.def("resources_save_cloud_formation_instance", &gkpy_shim::core_exports::resources_save_cloud_formation_instance);

    core.def("resources_save_layer_instances", &gkpy_shim::core_exports::resources_save_layer_instances);

    core.def("resources_save_function_instances", &gkpy_shim::core_exports::resources_save_function_instances);

    core.def("resources_upload_feature_layers", &gkpy_shim::core_exports::resources_upload_feature_layers);

    core.def("resources_upload_feature_functions", &gkpy_shim::core_exports::resources_upload_feature_functions);

    core.def("resources_instance_delete_stack", &gkpy_shim::core_exports::resources_instance_delete_stack);

    core.def("resources_instance_release", &gkpy_shim::core_exports::resources_instance_release);

    // Authentication module
    py::module authentication = m.def_submodule("authentication", "Authentication submodule");

    authentication.def("session_manager_instance_create", &gkpy_shim::session_exports::session_manager_instance_create);

    authentication.def("session_manager_instance_release", &gkpy_shim::session_exports::session_manager_instance_release);

    // Identity module
    py::module identity = m.def_submodule("identity", "Identity submodule");

    identity.def("identity_instance_create_with_session_manager", &gkpy_shim::identity_exports::identity_instance_create_with_session_manager);

    identity.def("identity_instance_release", &gkpy_shim::identity_exports::identity_instance_release);

    identity.def("identity_login", &gkpy_shim::identity_exports::identity_login);

    identity.def("identity_get_user", &gkpy_shim::identity_exports::identity_get_user);

    py::module identity_models = identity.def_submodule("model", "Identity Models submodule");
    py::class_<gkpy_shim::identity_exports::models::user_login>(identity_models, "user_login")
        .def(py::init())
        .def(py::init<const std::string&, const std::string&>())
        .def_readwrite("user_name", &gkpy_shim::identity_exports::models::user_login::user_name)
        .def_readwrite("password", &gkpy_shim::identity_exports::models::user_login::password);
    py::class_<gkpy_shim::identity_exports::models::get_user_response>(identity_models, "get_user_response")
        .def(py::init())
        .def(py::init<const std::string&, const std::string&, const std::string&, const std::string&, const std::string&>())
        .def_readwrite("user_id", &gkpy_shim::identity_exports::models::get_user_response::user_id)
        .def_readwrite("updated_at", &gkpy_shim::identity_exports::models::get_user_response::updated_at)
        .def_readwrite("created_at", &gkpy_shim::identity_exports::models::get_user_response::created_at)
        .def_readwrite("facebook_external_id", &gkpy_shim::identity_exports::models::get_user_response::facebook_external_id)
        .def_readwrite("facebook_ref_id", &gkpy_shim::identity_exports::models::get_user_response::facebook_ref_id);

    // Achievements module
    py::module achievements = m.def_submodule("achievements", "Achievements submodule");

    achievements.def("achievements_instance_create", &gkpy_shim::achievements_exports::achievements_instance_create);

    achievements.def("achievements_instance_release", &gkpy_shim::achievements_exports::achievements_instance_release);

    achievements.def("list_achievements", &gkpy_shim::achievements_exports::list_achievements);

    achievements.def("get_achievement", &gkpy_shim::achievements_exports::get_achievement);

    py::module achievement_models = achievements.def_submodule("model", "Achievements Models submodule");
    py::class_<gkpy_shim::achievements_exports::models::achievement>(achievement_models, "achievement")
        .def(py::init())
        .def(py::init<const std::string&, const std::string&>())
        .def_readwrite("achievement_id", &gkpy_shim::achievements_exports::models::achievement::achievement_id)
        .def_readwrite("title", &gkpy_shim::achievements_exports::models::achievement::title);

    // UserGameplayData module
    py::module user_gameplay_data = m.def_submodule("user_gameplay_data", "UserGameplayData submodule");

    user_gameplay_data.def("user_gameplay_data_instance_create", &gkpy_shim::user_gameplay_data_exports::user_gameplay_data_instance_create);

    user_gameplay_data.def("user_gameplay_data_instance_release", &gkpy_shim::user_gameplay_data_exports::user_gameplay_data_instance_release);

    user_gameplay_data.def("add_user_gameplay_data", &gkpy_shim::user_gameplay_data_exports::add_user_gameplay_data);

    user_gameplay_data.def("list_user_gameplay_data_bundles", &gkpy_shim::user_gameplay_data_exports::list_user_gameplay_data_bundles);

    user_gameplay_data.def("get_user_gameplay_data_bundle", &gkpy_shim::user_gameplay_data_exports::get_user_gameplay_data_bundle);

    user_gameplay_data.def("delete_user_gameplay_data_bundle", &gkpy_shim::user_gameplay_data_exports::delete_user_gameplay_data_bundle);

    user_gameplay_data.def("delete_all_user_gameplay_data", &gkpy_shim::user_gameplay_data_exports::delete_all_user_gameplay_data);

    py::module user_gameplay_data_models = user_gameplay_data.def_submodule("model", "User Gameplay Data Models submodule");
    py::class_<gkpy_shim::user_gameplay_data_exports::models::user_gameplay_data_bundle>(user_gameplay_data_models, "user_gameplay_data_bundle")
        .def(py::init())
        .def(py::init<const std::string&, const std::vector<std::pair<std::string, std::string>>&>())
        .def_readwrite("bundle_name", &gkpy_shim::user_gameplay_data_exports::models::user_gameplay_data_bundle::bundle_name)
        .def_readwrite("items", &gkpy_shim::user_gameplay_data_exports::models::user_gameplay_data_bundle::_items);

    // GameSaving module
    py::module game_saving = m.def_submodule("game_saving", "GameSaving submodule");

    game_saving.def("game_saving_instance_create", &gkpy_shim::game_saving_exports::game_saving_instance_create);

    game_saving.def("game_saving_instance_release", &gkpy_shim::game_saving_exports::game_saving_instance_release);

    game_saving.def("get_all_slot_sync_statuses", &gkpy_shim::game_saving_exports::get_all_slot_sync_statuses);

    game_saving.def("save_slot", &gkpy_shim::game_saving_exports::save_slot);

    game_saving.def("delete_slot", &gkpy_shim::game_saving_exports::delete_slot);

    py::module game_saving_models = game_saving.def_submodule("model", "Game Saving Data Models submodule");
    py::class_<gkpy_shim::game_saving_exports::models::slot>(game_saving_models, "slot")
        .def(py::init())
        .def(py::init<std::string, int64_t, bool, std::vector<uint8_t>, std::string>())
        .def_readwrite("slot_name", &gkpy_shim::game_saving_exports::models::slot::slot_name);

    // Un-used for now, can be used in the future for deployment tests
    py::enum_<FeatureType>(m, "FeatureType")
        .value("Main", FeatureType::Main)
        .value("Identity", FeatureType::Identity)
        .value("Authentication", FeatureType::Authentication)
        .value("Achievements", FeatureType::Achievements)
        .value("GameStateCloudSaving", FeatureType::GameStateCloudSaving)
        .value("UserGameplayData", FeatureType::UserGameplayData);

    m.attr("GAMEKIT_SUCCESS") = py::int_(GAMEKIT_SUCCESS);
    m.attr("__version__") = "dev";
}
