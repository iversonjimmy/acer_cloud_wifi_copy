ifndef BUILDROOT
$(error Please define BUILDROOT first!)
endif
CURRENT_IP=$(shell ifconfig | grep "inet " | grep -v 127.0.0.1 | cut -d\  -f2)
override IOS_LOG_RECEIVER_URL=http://$(CURRENT_IP)/LogReceiver/LogReceive.php

.PHONY: default
default: ios_setup ios_release ios_debug

.PHONY: ios_setup
ios_setup:
	printf "$(IOS_LOG_RECEIVER_URL)" > LogReceiveURL

.PHONY: ios_release
ios_release:
	xcodebuild -configuration Release -project libvpl/vpl.xcodeproj
	xcodebuild -configuration Release -project protobuf/protobuf.xcodeproj
	xcodebuild -configuration Release -project libsqlite3/sqlite3.xcodeproj
	xcodebuild -configuration Release -project protorpc/protorpc.xcodeproj
	xcodebuild -configuration Release -project libvplex/vplex.xcodeproj
	xcodebuild -configuration Release -project media_metadata_shared_impl/media_metadata_shared_impl.xcodeproj
	xcodebuild -configuration Release -project internal/internal.xcodeproj
	xcodebuild -configuration Release -project ccd/ccd.xcodeproj

.PHONY: ios_debug
ios_debug:
	xcodebuild -configuration Debug -project libvpl/vpl.xcodeproj
	xcodebuild -configuration Debug -project libvpl/vpl.xcodeproj -scheme OnDeviceVPLTest
	xcodebuild -configuration Debug -project protobuf/protobuf.xcodeproj
	xcodebuild -configuration Debug -project libsqlite3/sqlite3.xcodeproj
	xcodebuild -configuration Debug -project protorpc/protorpc.xcodeproj
	xcodebuild -configuration Debug -project libvplex/vplex.xcodeproj
	xcodebuild -configuration Debug -project libvplex/vplex.xcodeproj -scheme OnDeviceVPLEXTest
	xcodebuild -configuration Debug -project media_metadata_shared_impl/media_metadata_shared_impl.xcodeproj
	xcodebuild -configuration Debug -project internal/internal.xcodeproj
	xcodebuild -configuration Debug -project ccd/ccd.xcodeproj
	xcodebuild -configuration Debug -project ccd/ccd.xcodeproj -scheme OnDeviceTest
	xcodebuild -configuration Debug -project dx_remote_agent/dx_remote_agent.xcodeproj -scheme dx_remote_agent

.PHONY: clean
clean:
	xcodebuild -configuration Release -project libvpl/vpl.xcodeproj clean
	xcodebuild -configuration Release -project protobuf/protobuf.xcodeproj clean
	xcodebuild -configuration Release -project libsqlite3/sqlite3.xcodeproj clean
	xcodebuild -configuration Release -project protorpc/protorpc.xcodeproj clean
	xcodebuild -configuration Release -project libvplex/vplex.xcodeproj clean
	xcodebuild -configuration Release -project media_metadata_shared_impl/media_metadata_shared_impl.xcodeproj clean
	xcodebuild -configuration Release -project internal/internal.xcodeproj clean
	xcodebuild -configuration Release -project ccd/ccd.xcodeproj clean
	xcodebuild -configuration Release -project dx_remote_agent/dx_remote_agent.xcodeproj clean
	xcodebuild -configuration Debug -project libvpl/vpl.xcodeproj clean
	xcodebuild -configuration Debug -project protobuf/protobuf.xcodeproj clean
	xcodebuild -configuration Debug -project libsqlite3/sqlite3.xcodeproj clean
	xcodebuild -configuration Debug -project protorpc/protorpc.xcodeproj clean
	xcodebuild -configuration Debug -project libvplex/vplex.xcodeproj clean
	xcodebuild -configuration Debug -project media_metadata_shared_impl/media_metadata_shared_impl.xcodeproj clean
	xcodebuild -configuration Debug -project internal/internal.xcodeproj clean
	xcodebuild -configuration Debug -project ccd/ccd.xcodeproj clean
	xcodebuild -configuration Debug -project dx_remote_agent/dx_remote_agent.xcodeproj clean
