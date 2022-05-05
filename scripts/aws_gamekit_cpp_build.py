# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import argparse

import Win64.build as windows
import Android.build as android
import Mac.build as mac
import IOS.build as ios

def build_libs():
	parser = argparse.ArgumentParser(description="Builds and tests AWS GameKit cpp sdk.")
	platform_subparser = parser.add_subparsers(help="Target platform type.", dest="platform", required=True)

	windows_parser = platform_subparser.add_parser("Windows")
	windows.add_subparser(windows_parser)

	android_parser = platform_subparser.add_parser("Android")
	android.add_subparser(android_parser)

	mac_parser = platform_subparser.add_parser("Mac")
	mac.add_subparser(mac_parser)

	ios_parser = platform_subparser.add_parser("iOS")
	ios.add_subparser(ios_parser)

	parser.add_argument("type", choices=["Debug", "Release"], help="Compile type for GKCpp, Debug or Release.")
	args = parser.parse_args()

	if args.platform == "Windows":
		windows.build(args)
	elif args.platform == "Android":
		android.build(args)
	elif args.platform == "Mac":
		mac.build(args)
	elif args.platform == "iOS":
		ios.build(args)

if __name__ == "__main__":
	build_libs()
