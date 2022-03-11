// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "gmock/gmock.h"

#include <aws/cloudformation/CloudFormationClient.h>

namespace GameKit
{
    namespace Mocks
    {
        // Async calls are delegated to this.
        // See: https://google.github.io/googletest/gmock_cook_book.html#DelegatingToFake
        class FakeCloudFormationClient : public Aws::CloudFormation::CloudFormationClient
        {
        public:
            Aws::CloudFormation::Model::CreateStackOutcomeCallable CreateStackCallable(const Aws::CloudFormation::Model::CreateStackRequest& request) const
            {
                Aws::CloudFormation::Model::CreateStackResult createResult;
                Aws::CloudFormation::Model::CreateStackOutcome createOutcome(createResult);
                return std::async(std::launch::async, [=] { return  createOutcome; });
            }

            Aws::CloudFormation::Model::UpdateStackOutcomeCallable UpdateStackCallable(const Aws::CloudFormation::Model::UpdateStackRequest& request) const
            {
                Aws::CloudFormation::Model::UpdateStackResult updateResult;
                Aws::CloudFormation::Model::UpdateStackOutcome updateOutcome(updateResult);
                return std::async(std::launch::async, [=] { return  updateOutcome; });
            }

            Aws::CloudFormation::Model::DescribeStacksOutcome DescribeStacks(const Aws::CloudFormation::Model::DescribeStacksRequest& request) const
            {
                Aws::CloudFormation::Model::DescribeStacksResult describeResult;
                Aws::CloudFormation::Model::Stack testStack = Aws::CloudFormation::Model::Stack();
                testStack.SetStackName("gamekit-dev-testgame-identity");
                testStack.SetStackStatus(Aws::CloudFormation::Model::StackStatus::CREATE_COMPLETE);

                Aws::Vector<Aws::CloudFormation::Model::Output> outputs = Aws::Vector<Aws::CloudFormation::Model::Output>();
                Aws::CloudFormation::Model::Output output = Aws::CloudFormation::Model::Output().WithOutputKey("GameKitUserPoolClientId").WithOutputValue("1234567890");
                outputs.push_back(output);
                testStack.SetOutputs(outputs);

                Aws::Vector<Aws::CloudFormation::Model::Parameter> parameters = Aws::Vector<Aws::CloudFormation::Model::Parameter>();
                Aws::CloudFormation::Model::Parameter param1 = Aws::CloudFormation::Model::Parameter().WithParameterKey("FacebookEnabled").WithParameterValue("true");
                Aws::CloudFormation::Model::Parameter param2 = Aws::CloudFormation::Model::Parameter().WithParameterKey("FacebookClientId").WithParameterValue("1357327404709833");
                parameters.push_back(param1);
                parameters.push_back(param2);
                testStack.SetParameters(parameters);

                Aws::Vector<Aws::CloudFormation::Model::Stack> stacks = Aws::Vector<Aws::CloudFormation::Model::Stack>();
                stacks.push_back(testStack);
                describeResult.SetStacks(stacks);
                Aws::CloudFormation::Model::DescribeStacksOutcome describeOutcome(describeResult);
                return describeOutcome;
            }

            Aws::CloudFormation::Model::DescribeStackEventsOutcomeCallable DescribeStackEventsCallable(const Aws::CloudFormation::Model::DescribeStackEventsRequest& request) const
            {
                Aws::CloudFormation::Model::DescribeStackEventsResult eventsResult;
                Aws::CloudFormation::Model::StackEvent stackEvent;
                stackEvent.SetEventId("1");
                stackEvent.SetLogicalResourceId("TestResource");
                stackEvent.SetResourceStatus(Aws::CloudFormation::Model::ResourceStatus::CREATE_COMPLETE);
                eventsResult.AddStackEvents(stackEvent);
                Aws::CloudFormation::Model::DescribeStackEventsOutcome eventsOutcome(eventsResult);
                return std::async(std::launch::async, [=] { return  eventsOutcome; });
            }

            Aws::CloudFormation::Model::DeleteStackOutcomeCallable DeleteStackCallable(const Aws::CloudFormation::Model::DeleteStackRequest& request)
            {
                Aws::NoResult result;
                Aws::CloudFormation::Model::DeleteStackOutcome deleteOutcome(result);

                return std::async(std::launch::async, [=] { return deleteOutcome; });
            }
        };

        class MockCloudFormationClient : public Aws::CloudFormation::CloudFormationClient
        {
        private:
            FakeCloudFormationClient fake_;
        public:
            MockCloudFormationClient() {}
            virtual ~MockCloudFormationClient() override {}
            MOCK_METHOD(Aws::CloudFormation::Model::DescribeStacksOutcome, DescribeStacks, (const Aws::CloudFormation::Model::DescribeStacksRequest& request), (const, override));
            MOCK_METHOD(Aws::CloudFormation::Model::DescribeStackResourcesOutcome, DescribeStackResources, (const Aws::CloudFormation::Model::DescribeStackResourcesRequest& request), (const, override));
            MOCK_METHOD(Aws::CloudFormation::Model::DescribeStackResourceOutcome, DescribeStackResource, (const Aws::CloudFormation::Model::DescribeStackResourceRequest& request), (const, override));
            MOCK_METHOD(Aws::CloudFormation::Model::CreateStackOutcomeCallable, CreateStackCallable, (const Aws::CloudFormation::Model::CreateStackRequest& request), (const, override));
            MOCK_METHOD(Aws::CloudFormation::Model::UpdateStackOutcomeCallable, UpdateStackCallable, (const Aws::CloudFormation::Model::UpdateStackRequest& request), (const, override));
            MOCK_METHOD(Aws::CloudFormation::Model::DescribeStackEventsOutcomeCallable, DescribeStackEventsCallable, (const Aws::CloudFormation::Model::DescribeStackEventsRequest& request), (const, override));
            MOCK_METHOD(Aws::CloudFormation::Model::DeleteStackOutcomeCallable, DeleteStackCallable, (const Aws::CloudFormation::Model::DeleteStackRequest& request), (const, override));
            MOCK_METHOD(Aws::CloudFormation::Model::GetTemplateOutcome, GetTemplate, (const Aws::CloudFormation::Model::GetTemplateRequest& request), (const, override));

            // Delegates the default actions of the methods to a FakeCloudFormationClient object.
            void DelegateToFake()
            {
                ON_CALL(*this, CreateStackCallable).WillByDefault([this](const Aws::CloudFormation::Model::CreateStackRequest& request)
                    {
                        return fake_.CreateStackCallable(request);
                    });

                ON_CALL(*this, UpdateStackCallable).WillByDefault([this](const Aws::CloudFormation::Model::UpdateStackRequest& request)
                    {
                        return fake_.UpdateStackCallable(request);
                    });

                ON_CALL(*this, DescribeStackEventsCallable).WillByDefault([this](const Aws::CloudFormation::Model::DescribeStackEventsRequest& request)
                    {
                        return fake_.DescribeStackEventsCallable(request);
                    });

                ON_CALL(*this, DeleteStackCallable).WillByDefault([this](const Aws::CloudFormation::Model::DeleteStackRequest& request)
                    {
                        return fake_.DeleteStackCallable(request);
                    });
            }

        };
    }
}
