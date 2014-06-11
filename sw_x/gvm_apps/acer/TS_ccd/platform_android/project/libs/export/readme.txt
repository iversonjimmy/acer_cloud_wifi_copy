Ingredient:

a) src/com.acer.ccd.cache.* (all)
    1. DBContent.java
    2. DBDevice.java
    3. DBManager.java
    4. DBMediaInfo.java
    5. DBMusicPlaylist.java
    6. DBPreviewContent.java
    7. DBProvider.java
    8. DBSearchResult.java
    9. DBUtil.java

b) src/com.acer.ccd.cache.data.* (all)
    1. ContainerItem.java
    2. DlnaAudio.java
    3. DlnaContainer.java
    4. DlnaDevice.java
    5. DlnaImage.java
    6. DlnaMedia.java
    7. DlnaMusicPlaylist.java
    8. DlnaSearchResult.java
    9. DlnaVideo.java
    10. MusicInfo.java
    
c) src/com.acer.ccd.debug. (should be removed soon, after refactoring DBManager.java)
    1. L.java

d) src/com.acer.ccd.provider.
    1. CloudMediaColumns.java
    2. MediaProvider.java
    3. ThnumnailProvider.java

e) src/com.acer.ccd.service.
    1. IDlnaService.aidl
    2. IDlnaServiceCallback.aidl
    3. DlnaServiceWrapper.java

f) src/com.acer.ccd.serviceclient.* (all)
    1. AbstractCcdiClient.java
    2. AbstractMcaClient.java
    3. CcdeClient.java
    4. CcdeClientRemoteBinder.java
    5. McdClient.java
    6. McaClientRemoteBinder.java

g) src/com.acer.ccd.util.
    1. CcdSdkDefines.java

h) src/com.acer.ccd.util.igware.* (all)
    1. Constants.java
    2. Dataset.java
    3. DownloadProgress.java
    4. Filter.java
    5. Subfolder.java
    6. SyncListItem.java
    7. Utils.java

i) src/com.igware.android_services.
    1. ICcdiAidlRpc.aidl
    2. IMcaAidlRpc.aidl

j) gen/com.acer.ccd.service.* (all)
    1. IDlnaService.java
    2. IDlnaServiceCallback.java

k) gen/com.igware.android_services.* (all)
    1. ICcdiAidlRpc.java
    2. IMcaAidlRpc.java
