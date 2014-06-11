package com.acer.ccd.cache.data;

import android.database.Cursor;

import com.acer.ccd.cache.DBDevice;
import com.acer.ccd.debug.L;

/**
 * The object that contains the information what a Device needs.
 */
public class DlnaDevice {
    private static final String TAG = "DlnaDevice";
    private String mDeviceType;
    private int mIsAcerDev;
    private String mDeviceName;
    private String mManufacture;
    private String mManufacturerUrl;
    private String mModelName;
    private String mUuid;
    private String mIconPath;
    private long mDbId;
    private int mDeviceStatus;

    public void dump() {
        L.t(TAG, "mDeviceType = `%s`, mIsAcerDev = %d, mDeviceName = `%s`",
                mDeviceType, mIsAcerDev, mDeviceName);
        L.t(TAG, "mManufacture = `%s`, mManufacturerUrl = `%s`, mModelName = `%s`, mUuid = `%s`",
                mManufacture, mManufacturerUrl, mModelName, mUuid);
        L.t(TAG, ", mIconPath = `%s`, mDbId = %d , mDeviceStatus = %d",
                mIconPath, mDbId, mDeviceStatus);
    }

    public DlnaDevice() {
        super();
    }

    public void setDeviceType(String deviceType) {
        this.mDeviceType = deviceType;
    }

    public String getDeviceType() {
        return mDeviceType;
    }

    public void setDeviceName(String deviceName) {
        this.mDeviceName = deviceName;
    }

    public String getDeviceName() {
        return mDeviceName;
    }

    public void setManufacture(String manufacture) {
        this.mManufacture = manufacture;
    }

    public String getManufacture() {
        return mManufacture;
    }

    public void setManufacturerUrl(String manufacturerUrl) {
        this.mManufacturerUrl = manufacturerUrl;
    }

    public String getManufacturerUrl() {
        return mManufacturerUrl;
    }

    public void setModelName(String modelName) {
        this.mModelName = modelName;
    }

    public String getModelName() {
        return mModelName;
    }

    public void setUuid(String uuid) {
        this.mUuid = uuid;
    }

    public String getUuid() {
        return mUuid;
    }

    public void setIconPath(String iconPath) {
        this.mIconPath = iconPath;
    }

    public String getIconPath() {
        return mIconPath;
    }

    public void setDbId(long dbId) {
        this.mDbId = dbId;
    }

    public long getDbId() {
        return mDbId;
    }

    public void setAcerDevice(int isAcerDev) {
        this.mIsAcerDev = isAcerDev;
    }

    public int isAcerDevice() {
        return mIsAcerDev;
    }

    public int getDeviceStatus() {
        return mDeviceStatus;
    }

    public void setDeviceStatus(int deviceStatus) {
        this.mDeviceStatus = deviceStatus;
    }

    public boolean setDataFromDB(Cursor cursor)
    {
        boolean bResult = true;
        if (cursor == null)
        {
            bResult = false;
            throw new IllegalArgumentException("Invalid cursor.");
        }

        try {
            setDeviceType(cursor.getString(DBDevice.ColumnName.COL_DEVICETYPE.ordinal()));
            setAcerDevice(cursor.getInt(DBDevice.ColumnName.COL_ISACER.ordinal()));
            setDeviceName(cursor.getString(DBDevice.ColumnName.COL_DEVICENAME.ordinal()));
            setManufacture(cursor.getString(DBDevice.ColumnName.COL_MANUFACTURE.ordinal()));
            setManufacturerUrl(cursor.getString(DBDevice.ColumnName.COL_MANUFACTURERURL.ordinal()));
            setModelName(cursor.getString(DBDevice.ColumnName.COL_MODELNAME.ordinal()));
            setUuid(cursor.getString(DBDevice.ColumnName.COL_UUID.ordinal()));
            setDbId(cursor.getInt(DBDevice.ColumnName.COL_ID.ordinal()));
            setDeviceStatus(cursor.getInt(DBDevice.ColumnName.COL_DEVICESTATUS.ordinal()));
            setIconPath(cursor.getString(DBDevice.ColumnName.COL_ICON.ordinal()));
        } catch (Exception e) {
            e.printStackTrace();
            bResult = false;
        }

        return bResult;
    }
}
