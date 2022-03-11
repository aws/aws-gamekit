// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "gmock/gmock.h"

#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateBucketRequest.h>

namespace GameKit
{
    namespace Mocks
    {
        class MockS3Client : public Aws::S3::S3Client
        {
        private:
            bool m_callDieInDestructor = false;

        public:
            MockS3Client() {}
            ~MockS3Client() override 
            { 
                if (m_callDieInDestructor)
                { 
                    Die(); 
                } 
            }
            MOCK_METHOD(void, Die, ());
            MOCK_METHOD(Aws::S3::Model::CreateBucketOutcome, CreateBucket, (const Aws::S3::Model::CreateBucketRequest& request), (const, override));
            MOCK_METHOD(Aws::S3::Model::ListBucketsOutcome, ListBuckets, (), (const, override));
            MOCK_METHOD(Aws::S3::Model::PutObjectOutcome, PutObject, (const Aws::S3::Model::PutObjectRequest& request), (const, override));
            MOCK_METHOD(Aws::S3::Model::PutBucketLifecycleConfigurationOutcome, PutBucketLifecycleConfiguration, (const Aws::S3::Model::PutBucketLifecycleConfigurationRequest& request), (const, override));

            void CallDieInDestructor(bool callDieInDestructor)
            {
                m_callDieInDestructor = callDieInDestructor;
            }
        };
    }
}
