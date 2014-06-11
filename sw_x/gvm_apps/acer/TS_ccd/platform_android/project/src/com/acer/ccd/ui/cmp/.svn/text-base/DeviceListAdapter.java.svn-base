/*
 *  Copyright 2011 Acer Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER INC.
 */

package com.acer.ccd.ui.cmp;

import com.acer.ccd.R;

import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.ArrayList;

public class DeviceListAdapter extends BaseAdapter {

    private final static String TAG = "DeviceListAdapter";

    private ArrayList<DeviceItem> mAcerList = new ArrayList<DeviceItem>();
    private ArrayList<DeviceItem> mNonAcerList = new ArrayList<DeviceItem>();
    private Context mContext;
    private LayoutInflater mInflater;

    public DeviceListAdapter(Context context) {
        mContext = context;
        mInflater = LayoutInflater.from(mContext);
        mAcerList = new ArrayList<DeviceItem>();
        mNonAcerList = new ArrayList<DeviceItem>();
    }

    @Override
    public int getCount() {
        return mAcerList.size()+mNonAcerList.size();
    }

    @Override
    public Object getItem(int position) {
//        if (position >= mAcerList.size())
//            return null;
        return getItemFromDifferentList(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }
    
    @Override
    public boolean isEnabled(int position) {
        if (position == 0)
            return false;
        if (position == mAcerList.size())
            return false;
        return super.isEnabled(position);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        Holder holder = null;
        DeviceItem item = getItemFromDifferentList(position);
        
        if (!item.getItemTitle().equals("")) {
//            if (convertView == null || convertView.getApplicationWindowToken() == null) {
                holder = new Holder();
                convertView = mInflater.inflate(R.layout.device_list_title_item, null);
                holder.textTitle = (TextView) convertView.findViewById(R.id.device_item_text_title);
                // holder.convertView = convertView;
//            } else {
//                holder = (Holder) convertView.getTag();
//            }
            
            holder.item = item;
            
            if (holder.textTitle != null)
                holder.textTitle.setText(item.getItemTitle());
            
            return convertView;
        }

//        if (convertView == null || convertView.getApplicationWindowToken() == null) {
            holder = new Holder();
            convertView = mInflater.inflate(R.layout.device_list_item, null);
            // holder.convertView = convertView;
            holder.imageDeviceIcon = (ImageView) convertView
                    .findViewById(R.id.device_item_image_icon);
            holder.textDeviceName = (TextView) convertView
                    .findViewById(R.id.device_item_label_devicename);
            holder.textDeviceType = (TextView) convertView
                    .findViewById(R.id.device_item_label_devicetype);
            holder.checkboxLink = (CheckBox) convertView
                    .findViewById(R.id.device_item_checkbox_link);
            holder.checkboxLink.setTag(holder);
            holder.checkboxLink.setOnClickListener(mLinkClickListener);
            convertView.setTag(holder);
//        } else {
//            holder = (Holder) convertView.getTag();
//        }
        holder.item = item;

        if (item.getIsOnline())
            holder.imageDeviceIcon.setImageResource(R.drawable.device_item_icon_1);
        else
            holder.imageDeviceIcon.setImageResource(R.drawable.device_item_icon_2);

        holder.textDeviceName.setText(item.getDeviceName());
        holder.textDeviceType.setText(getTypeString(item.getDeviceType()));
        holder.checkboxLink.setChecked(true);

        return convertView;
    }

    private String getTypeString(int type) {
        switch (type) {
            case DeviceItem.DEVICE_TYPE_PHONE:
                return mContext.getText(R.string.device_label_type_phone).toString();
            case DeviceItem.DEVICE_TYPE_TABLET:
                return mContext.getText(R.string.device_label_type_tablet).toString();
            case DeviceItem.DEVICE_TYPE_PC:
                return mContext.getText(R.string.device_label_type_pc).toString();
        }
        return mContext.getText(R.string.device_label_type_unknown).toString();

    }

    public int addAcerItem(DeviceItem item) {
        
        if (mAcerList.size() == 0)
            addAcerTitleItem();
        
        Log.i(TAG, "item = " + item.toString());
        mAcerList.add(item);
        return mAcerList.size();
    }

    public int addItems(ArrayList<DeviceItem> items) {
        for (DeviceItem item : items) {
            if (item.getDeviceId() == DeviceItem.DEVICE_ID_ACER)
                addAcerItem(item);
            else
                addNonAcerItem(item);
        }
        return mAcerList.size();
    }
    
    public int addNonAcerItem(DeviceItem item) {
        
        if (mNonAcerList.size() == 0)
            addNonAcerTitleItem();

        Log.i(TAG, "item = " + item.toString());
        mNonAcerList.add(item);
        return mNonAcerList.size();
    }
    
    private void addAcerTitleItem() {
        DeviceItem item = new DeviceItem();
        item.setItemTitle(mContext.getText(R.string.device_label_item_title_acer).toString());
        mAcerList.add(item);
    }
    
    private void addNonAcerTitleItem() {
        DeviceItem item = new DeviceItem();
        item.setItemTitle(mContext.getText(R.string.device_label_item_title_non_acer).toString());
        mNonAcerList.add(item);
    }
    
    private DeviceItem getItemFromDifferentList(int position) {
        Log.i(TAG, "getItemFromDifferentList : position = " + position);
        Log.i(TAG, "mNonAcerList.size() = " + mNonAcerList.size());
        Log.i(TAG, "mAcerList.size() = " + mAcerList.size());
        if (position >= mAcerList.size()) {
            
            return mNonAcerList.get(position - mAcerList.size());
        }
        return mAcerList.get(position);
    }

    private CheckBox.OnClickListener mLinkClickListener = new CheckBox.OnClickListener() {

        @Override
        public void onClick(View arg0) {
            // TODO Auto-generated method stub
            Log.i(TAG, "click item = " + ((Holder)arg0.getTag()).item.toString());
        }

    };

    private class Holder {
        public DeviceItem item;
        // public View convertView;
        public ImageView imageDeviceIcon;
        public TextView textDeviceName;
        public TextView textDeviceType;
        public TextView textTitle;
        public CheckBox checkboxLink;
    }
}
