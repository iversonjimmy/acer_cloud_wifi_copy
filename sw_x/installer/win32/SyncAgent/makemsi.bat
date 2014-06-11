candle.exe SyncAgent.wxs -out SyncAgent.wixobj
light.exe SyncAgent.wixobj -dWixUILicenseRtf=License-en-us.rtf -cultures:en-us -loc en-us.wxl -ext WixUIExtension -out SyncAgent-%VERSION%-en-us.msi
light.exe SyncAgent.wixobj -dWixUILicenseRtf=License-zh-tw.rtf -cultures:zh-tw -loc zh-tw.wxl -ext WixUIExtension -out SyncAgent-%VERSION%-zh-tw.msi
